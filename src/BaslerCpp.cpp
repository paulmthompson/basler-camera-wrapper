#include "BaslerCpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
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

  offset = 0;

  img_to_display = (char*)calloc(_h * _w * bytes_per_pixel, sizeof(char));
  mydata = (char*)calloc(_h * _w * frame_buf_size * bytes_per_pixel, sizeof(char));

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

    camera.Attach( tlFactory.CreateDevice( devices[ 0 ]));

    if (camera.IsPylonDeviceAttached())
    {
      std::cout << "Using device " << camera.GetDeviceInfo().GetModelName() << std::endl;
      attached = true;

      camera.MaxNumBuffer = 50;
      camera.Open(); // Need to access parameters

      //Load values from configuration file
      CFeaturePersistence::Load(configFileName.c_str(), &camera.GetNodeMap(), true);

      std::cout << "Frame Rate " << camera.AcquisitionFrameRate.GetValue() << std::endl;
      std::cout << "Exposure Time: " << camera.ExposureTime.GetValue() << std::endl;

      //Resulting Frame Rate gives the real frame rate accounting for all of the camera
      //configuration parameters such as the desired sampling rate and exposure time
      std::cout << "Resulting Frame Rate " << camera.ResultingFrameRate.GetValue() << std::endl;
    } else {
      std::cout << "Camera was not able to be initialized. Is one connected?" << std::endl;
    }
  }
}

void MyCamera::ConnectBySerial(const char *myserial)
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

    for (int i = 0; i < devices.size(); i++) {

      if (devices[i].GetSerialNumber() == myserial) {
        std::cout << "Matched serial number for " << devices[i].GetSerialNumber() << std::endl;

        camera.Attach( tlFactory.CreateDevice( devices[ i ]));

        if (camera.IsPylonDeviceAttached())
        {
          std::cout << "Using device " << camera.GetDeviceInfo().GetModelName() << std::endl;
          attached = true;

          camera.MaxNumBuffer = 50;
          camera.Open(); // Need to access parameters

          //Load values from configuration file
          CFeaturePersistence::Load(configFileName.c_str(), &camera.GetNodeMap(), true);
        } else {
          std::cout << "Camera was not able to be initialized. Is one connected?" << std::endl;
        }
      } else {
        std::cout << "Not matched serial number for " << devices[i].GetSerialNumber() << std::endl;
      }
    }
  }
}

void MyCamera::StartAcquisition()
{
  acquisitionActive=true;
  camera.StartGrabbing();
}

void MyCamera::StopAcquisition()
{
  acquisitionActive=false;
  camera.StopGrabbing();
}

void MyCamera::GrabFrames()
{
  if (acquisitionActive != false && attached) {

    // This smart pointer will receive the grab result data.
    CGrabResultPtr ptrGrabResult;

    int nBuffersInQueue = 0;
    framesGrabbed = false;

    //Here we will send data to FFMPEG to save, otherwise we will
    if (saveData) {

      bool keep_going = true;
      int new_frame_id = 0; //Keeps track of how many frames are acquired this time
      int true_pos = offset; //Keeps track of where you actually are in the ring buffer

      while (keep_going) {
        if (camera.RetrieveResult( 0, ptrGrabResult, TimeoutHandling_Return)) {

          //Fill buffer at next position in ring buffer

          //If we fall sufficiently behind, we will start to overwrite the ring buffer while a thread is waiting to send
          //it to ffmpeg. A better implementation would have another semaphore to protect against this, and make the
          //Producer wait to collect more camera frames

          //My impression is that shouldn't happen intermittently if the buffer is sufficiently large,
          //and therefore if it does happen, ffmpeg won't be able to keep up saving anyway and there will
          //be other problems to deal with
          memcpy(mydata + _w*_h*bytes_per_pixel + _w*_h*true_pos*bytes_per_pixel, static_cast<char*> (ptrGrabResult->GetBuffer()),_w*_h*bytes_per_pixel);
          nBuffersInQueue++;
          totalFramesSaved+=1;
        } else {
          keep_going = false;
        }
        if (keep_going) {
          new_frame_id += 1; // How many frames have been written
          true_pos = (true_pos + 1) % frame_buf_size; // Position of next frame to acquire
        }
      }
      // After we have acquired from the camera, we write the last one to the
      //
      if (nBuffersInQueue) {
        memcpy(img_to_display, mydata + offset * _w * _h * bytes_per_pixel,_w*_h*bytes_per_pixel);

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

        if (camera.RetrieveResult( 0, ptrGrabResult, TimeoutHandling_Return)) {

          memcpy(mydata + _w*_h*bytes_per_pixel, static_cast<char*> (ptrGrabResult->GetBuffer()),_w*_h*bytes_per_pixel);
          nBuffersInQueue++;
        } else {
          keep_going = false;
        }
      }
      if (nBuffersInQueue) {
        memcpy(img_to_display, mydata,_w*_h*bytes_per_pixel);
      }
    }
    if (nBuffersInQueue) {
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
    fwrite(mydata + _w*_h*start_pos*bytes_per_pixel, (sizeof(char)*_w*_h*bytes_per_pixel)*(write_dist), 1, ffmpeg);
  } else {
    int first_write = frame_buf_size - start_pos;
    fwrite(mydata + _w*_h*start_pos*bytes_per_pixel, (sizeof(char)*_w*_h*bytes_per_pixel)*(first_write), 1, ffmpeg);
    int second_write = write_dist - first_write;
    fwrite(mydata, (sizeof(char)*_w*_h*bytes_per_pixel)*(second_write), 1, ffmpeg);
  }

  cv.notify_all();
}

void MyCamera::resizeImage() {

  free(img_to_display);
  free(mydata);

  img_to_display = (char*)calloc(_h * _w * bytes_per_pixel, sizeof(char));
  mydata = (char*)calloc(_h * _w * frame_buf_size * bytes_per_pixel, sizeof(char));
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
    mkdir(dir_name.c_str(),S_IRWXU);
    #endif
  }
  char full_cmd[1000];
  //Should have a configuration file or something to state location of ffmpeg along with
  //other default parameters, and probably the location of the basler camera features

  int len;

  if (trial_structure) {
    len = std::snprintf(full_cmd, sizeof(full_cmd), "%s %s %dx%d %s %s%s%d%s", ffmpegPath.c_str(), ffmpegInputOptions.c_str(), _w, _h, ffmpegOutputOptions.c_str(), saveFilePath.c_str(), "/", trialNum, "/%d.tif");
  }
  else {
    len = std::snprintf(full_cmd, sizeof(full_cmd), "%s %s %dx%d %s %s%s", ffmpegPath.c_str(), ffmpegInputOptions.c_str(), _h, _w, ffmpegOutputOptions.c_str(), saveFilePath.c_str(), saveFileName.c_str());
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
