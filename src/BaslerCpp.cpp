#include "BaslerCpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "basler_config.h"

using namespace Basler_UsbCameraParams;
using namespace Pylon;
using namespace std;

//https://austingwalters.com/multithreading-semaphores/
std::mutex mtx;           // mutex for critical section
std::condition_variable cv; // condition variable for critical section
int current = 0; // current count
int thread_counter = 0;

MyCamera::MyCamera(int cam_n)
{

  attached = false;
  acquisitionActive = false;
  saveData=false;
  saveFilePath = "./";
  saveFileName = "output.mp4";
  ffmpegPath = "ffmpeg";
  ffmpegInputOptions = "-f rawvideo -pix_fmt gray16 -s";
  ffmpegOutputOptions = "-i - -y -pix_fmt gray16 -compression_algo raw";
  _h = 480;
  _w = 640;
  bytes_per_pixel = 1;
  num_cam = cam_n;
  buf_id[0]=0;
  buf_id[1]=0;

  offset = 0;

  img_to_display = (char*)calloc(_h * _w * num_cam * bytes_per_pixel, sizeof(char));
  leftover = (char*)calloc(_h * _w * num_cam * bytes_per_pixel, sizeof(char));
  mydata = (char*)calloc(_h * _w * num_cam * frame_buf_size * bytes_per_pixel, sizeof(char));

  read_frame[0] = false;
  read_frame[1] = false;
  left_over_flag = false;
  string file_path = __FILE__;
  string dir_path = file_path.substr(0, file_path.rfind("/"));

  configFileName = "default.pfs";
  std::cout << configFileName << std::endl;
  framesGrabbed = false;
  totalFramesSaved = 0;

  trialNum=0;
  trial_structure = 1;

  #ifdef _WIN32
  AllocConsole();
  ShowWindow(GetConsoleWindow(), SW_HIDE);
  #endif
}

MyCamera::~MyCamera()
{
  free(img_to_display);
  free(leftover);
  free(mydata);
}

void MyCamera::Connect()
{
  if (!attached) {

    // Get the transport layer factory.
    CTlFactory& tlFactory = CTlFactory::GetInstance();

    // Get all attached devices and exit application if no device is found.
    DeviceInfoList_t devices;
    if ( tlFactory.EnumerateDevices(devices) == 0 )
    {
      throw RUNTIME_EXCEPTION( "Not enough cameras present.");
    }

    for (int i = 0; i < num_cam; i++) {

      camera[i].Attach( tlFactory.CreateDevice( devices[ i ]));

      if (camera[i].IsPylonDeviceAttached())
      {
        std::cout << "Using device " << camera[i].GetDeviceInfo().GetModelName() << std::endl;
        attached = true;

        camera[i].MaxNumBuffer = 50;
        camera[i].Open(); // Need to access parameters

        //Load values from configuration file
        CFeaturePersistence::Load(configFileName.c_str(), &camera[i].GetNodeMap(), true);

        std::cout << "Frame Rate " << camera[i].AcquisitionFrameRate.GetValue() << std::endl;
        std::cout << "Exposure Time: " << camera[i].ExposureTime.GetValue() << std::endl;

        //Resulting Frame Rate gives the real frame rate accounting for all of the camera
        //configuration parameters such as the desired sampling rate and exposure time
        std::cout << "Resulting Frame Rate " << camera[i].ResultingFrameRate.GetValue() << std::endl;
      } else {
        std::cout << "Camera was not able to be initialized. Is one connected?" << std::endl;
      }
    }
  }
}

void MyCamera::StartAcquisition()
{
  acquisitionActive=true;
  for (int i = 0; i < num_cam; i++) {
    camera[i].StartGrabbing();
  }
}

void MyCamera::StopAcquisition()
{
  acquisitionActive=false;
  for (int i = 0; i < num_cam; i++) {
    camera[i].StopGrabbing();
  }
}

void MyCamera::GrabFrames()
{
  if (acquisitionActive != false && attached) {

    // This smart pointer will receive the grab result data.
    CGrabResultPtr ptrGrabResult[MAX_CAMERA];

    int nBuffersInQueue = 0;
    framesGrabbed = false;

    if (saveData) {

      bool keep_going = true;
      int new_frame_id = 0; //Keeps track of how many frames are acquired this time
      int true_pos = offset; //Keeps track of where you actually are in the ring buffer

      //Before the first frame from the(each) camera is acquired in a producer, we should check and see
      //If there is a left over flag from the previous iteration.
      if (left_over_flag) {
        for (int i = 0; i < num_cam; i++) {
          if (!read_frame[i]) {
            camera[i].RetrieveResult( 0, ptrGrabResult[i], TimeoutHandling_Return);
            memcpy(leftover + _w*_h*i*bytes_per_pixel, static_cast<char*> (ptrGrabResult[i]->GetBuffer()),_w*_h*bytes_per_pixel);
          } else {
            read_frame[i] = false;
          }
          buf_id[i] = 1;
        }
        memcpy(mydata + _w*_h*num_cam*true_pos*bytes_per_pixel, leftover,_w*_h*num_cam*bytes_per_pixel);
        left_over_flag = false;
        new_frame_id += 1;
        true_pos = (true_pos + 1) % frame_buf_size;
      }

      while (keep_going) {

        //Should there be a producer thread for each camera? Not sure if thread safe on the basler end to do that
        //One single producer thread is probably okay to start, regardless of how many cameras
        //I don't think that this is the bottleneck
        for (int i = 0; i < num_cam; i++) {

          if (camera[i].RetrieveResult( 0, ptrGrabResult[i], TimeoutHandling_Return)) {

            //Fill buffer at next position in ring buffer

            //If we fall sufficiently behind, we will start to overwrite the ring buffer while a thread is waiting to send
            //it to ffmpeg. A better implementation would have another semaphore to protect against this, and make the
            //Producer wait to collect more camera frames

            //My impression is that shouldn't happen intermittently if the buffer is sufficiently large,
            //and therefore if it does happen, ffmpeg won't be able to keep up saving anyway and there will
            //be other problems to deal with
            memcpy(mydata + _w*_h*i*bytes_per_pixel + _w*_h*num_cam*true_pos*bytes_per_pixel, static_cast<char*> (ptrGrabResult[i]->GetBuffer()),_w*_h*bytes_per_pixel);
            read_frame[i] = true;
            nBuffersInQueue++;
            totalFramesSaved+=1;
            buf_id[i] += 1;

          } else {

            keep_going = false;
            read_frame[i] = false;
          }
        }

        if (keep_going) {
          new_frame_id += 1; // How many frames have been written
          true_pos = (true_pos + 1) % frame_buf_size; // Position of next frame to acquire
        }
      }

      if (nBuffersInQueue) {
        memcpy(img_to_display, mydata + offset * _w * _h * num_cam * bytes_per_pixel,_w*_h*num_cam *bytes_per_pixel);

        //std::cout << "C1 " << buf_id[0] << "C2: " << buf_id[1] << std::endl;

        for (int i = 0; i < num_cam; i++) {
          if (read_frame[i]) {
            memcpy(leftover + _w*_h*i*bytes_per_pixel, mydata + _w*_h*i *bytes_per_pixel + _w*_h*num_cam*(true_pos)*bytes_per_pixel, _w*_h*bytes_per_pixel);
            left_over_flag = true;
          }
          buf_id[i] = 0;
        }

        //Only God knows if this is okay to do.
        //This should be a consumer thread
        std::thread t1(&MyCamera::mywrite,this,new_frame_id,offset,thread_counter);
        t1.detach();
        thread_counter++;
      }

      offset = true_pos;

    } else {

      bool keep_going = true;

      while (keep_going) {

        for (int i = 0; i < num_cam; i++) {

          if (camera[i].RetrieveResult( 0, ptrGrabResult[i], TimeoutHandling_Return)) {

            memcpy(mydata + _w*_h*i*bytes_per_pixel, static_cast<char*> (ptrGrabResult[i]->GetBuffer()),_w*_h*bytes_per_pixel);
            read_frame[i] = true;
            nBuffersInQueue++;

          } else {

            keep_going = false;
            read_frame[i] = false;
          }
        }
      }
      if (nBuffersInQueue) {
        memcpy(img_to_display, mydata,_w*_h*num_cam*bytes_per_pixel);
      }
    }

    if (nBuffersInQueue)
    {
      framesGrabbed = true;
    }
  }
}


//Put this in a counting semaphore for thread safety
void MyCamera::mywrite(int write_dist, int start_pos,int thread_id)
{

  //If we fall behind, we can't have multiple threads writing to ffmpeg at the same time
  //So a possible subsequent thread waits to start writing until the most recent finishes.

  //The writing method checks to see where it is in the ring buffer for the amount of writing it needs to do
  //If the ring buffer wraps arounds, it will have to do two writes

  std::unique_lock<std::mutex> lck(mtx);
  while(current != thread_id){ cv.wait(lck); }
  current++;
  if ((start_pos + write_dist) < frame_buf_size) {
    fwrite(mydata + _w*_h*num_cam*start_pos*bytes_per_pixel, (sizeof(char)*_w*_h*num_cam*bytes_per_pixel)*(write_dist), 1, ffmpeg);
  } else {
    int first_write = frame_buf_size - start_pos;
    fwrite(mydata + _w*_h*num_cam*start_pos*bytes_per_pixel, (sizeof(char)*_w*_h*num_cam*bytes_per_pixel)*(first_write), 1, ffmpeg);
    int second_write = write_dist - first_write;
    fwrite(mydata, (sizeof(char)*_w*_h*num_cam*bytes_per_pixel)*(second_write), 1, ffmpeg);
  }

  cv.notify_all();
}

void MyCamera::resizeImage() {

  free(img_to_display);
  free(leftover);
  free(mydata);

  img_to_display = (char*)calloc(_h * _w * num_cam * bytes_per_pixel, sizeof(char));
  leftover = (char*)calloc(_h * _w * num_cam * bytes_per_pixel, sizeof(char));
  mydata = (char*)calloc(_h * _w * num_cam * frame_buf_size * bytes_per_pixel, sizeof(char));

}

void MyCamera::StartFFMPEG()
{
  //If we are using a "trial structure" we should create a new directory with
  //The trial number specified
  if (trial_structure) {
    std::string dir_name = saveFilePath + "/" + std::to_string(trialNum);

    #ifdef _WIN32
    CreateDirectoryA(dir_name.c_str(), NULL);
    #else
    mkdir(dir_name.c_str());
    #endif
  }
  char full_cmd[1000];
  //Should have a configuration file or something to state location of ffmpeg along with
  //other default parameters, and probably the location of the basler camera features

  int len;

  if (trial_structure) {
    len = std::snprintf(full_cmd, sizeof(full_cmd), "%s %s %dx%d %s %s%s%d%s", ffmpegPath.c_str(), ffmpegInputOptions.c_str(), _w, _h*num_cam, ffmpegOutputOptions.c_str(), saveFilePath.c_str(), "/", trialNum, "/%d.tif");
  }
  else {
    len = std::snprintf(full_cmd, sizeof(full_cmd), "%s %s %dx%d %s %s%s", ffmpegPath.c_str(), ffmpegInputOptions.c_str(), _h, _w*num_cam, ffmpegOutputOptions.c_str(), saveFilePath.c_str(), saveFileName.c_str());
  }

  #ifdef _WIN32
  ffmpeg = _popen(full_cmd, "wb");
  #else
  ffmpeg = popen(full_cmd, "w");
  #endif
  if (ffmpeg == NULL) {
    printf("Error opening file unexistent: %s\n", strerror(errno));
  }
  saveData = 1;
}

void MyCamera::EndFFMPEG()
{
  #ifdef _WIN32
  _pclose(ffmpeg);
  #else
  pclose(ffmpeg);
  #endif

  //reset the number of total frames saved
  totalFramesSaved = 0;
  saveData = 0;
  if (trial_structure) {
    trialNum += 1;
  }

}

void MyCamera::ChangeFolder(const char *folder)
{
  saveFilePath = folder;
}

void MyCamera::UpdateSaveName(const char *name)
{
  saveFileName = name;
}

void MyCamera::SetTrialStructure(int _structure)
{
  trial_structure = _structure;
}

void MyCamera::ChangeCameraConfig(const char *path) {
  configFileName = path;
}

void MyCamera::ChangeFFMPEG(const char *path) {
  ffmpegPath = path;
}

void MyCamera::ChangeFFMPEGInputOptions(const char *cmd) {
  ffmpegInputOptions = cmd;
}

void MyCamera::ChangeFFMPEGOutputOptions(const char *cmd) {
  ffmpegOutputOptions = cmd;
}

void MyCamera::ChangeBytes(int _bytes) {
  bytes_per_pixel = _bytes;
}

void init_pylon()
{
  // Before using any pylon methods, the pylon runtime must be initialized.
  PylonInitialize();
  //Pylon::PylonAutoInitTerm autoInitTerm;
}

int main()
{

  return 0;
}
