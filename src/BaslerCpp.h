#ifndef BASLERCPP_H
#define BASLERCPP_H

#include <stdio.h>
#include <fstream>
#include <string>

#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>

#include "basler_config.h"

class MyCamera
{
public:

  MyCamera(int cam_n = 1);
  ~MyCamera();

  void Connect();
  void ConnectBySerial(const char *myserial);
  void StartAcquisition();
  void StopAcquisition();
  void GrabFrames();
  void StartFFMPEG();
  void EndFFMPEG();
  void ChangeFolder(const char *folder);
  void UpdateSaveName(const char *name);
  void mywrite(int write_dist, int start_pos,int thread_id);
  void resizeImage();
  void SetTrialStructure(int _structure);
  void ChangeCameraConfig(const char *path);
  void ChangeFFMPEG(const char *path);
  void ChangeFFMPEGInputOptions(const char *cmd);
  void ChangeFFMPEGOutputOptions(const char *cmd);
  void ChangeBytes(int _bytes);

  bool attached;
  bool acquisitionActive;
  std::string saveFilePath;
  std::string saveFileName;
  std::string configFileName;
  std::string ffmpegPath;
  std::string ffmpegInputOptions;
  std::string ffmpegOutputOptions;
  bool saveData;

  //This holds data coming in from the camera
  //I am using it as a ring buffer with a maximum capcity of frame_buf_size
  //frames for each camera.
  char* mydata;

  //This is the picture that is meant to be interacted with (such as sended to a 3rd party viewer)
  //It is the first frame acquired during an acquisition round.
  char* img_to_display;

  bool framesGrabbed;
  int totalFramesSaved;
  int _h;
  int _w;
  int bytes_per_pixel;

  int trial_structure; //Flag if trial structure is being used
  int trialNum; //If a trial structure is being used

private:
  Pylon::CBaslerUsbInstantCamera camera;
  int offset; //Indicates the starting index in the ring buffer
  FILE* ffmpeg;
};

void init_pylon();


#endif // BASLERCPP_H
