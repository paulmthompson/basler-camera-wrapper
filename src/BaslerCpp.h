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
  void StartAcquisition();
  void StopAcquisition();
  void GrabFrames();
  void StartFFMPEG();
  void EndFFMPEG();
  void ChangeFolder(const char *folder);
  void UpdateSaveName();
  void mywrite(int write_dist, int start_pos,int thread_id);
  void resizeImage();
  void SetTrialStructure(int _structure);

  bool attached;
  bool acquisitionActive;
  float frameRate; //Estimate of frame rate currently being used
  std::string saveFilePath;
  std::string saveFileName;
  std::string configFileName;
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
  int num_cam;

  int trial_structure; //Flag if trial structure is being used
  int trialNum; //If a trial structure is being used

private:
  Pylon::CBaslerUsbInstantCamera camera[MAX_CAMERA];

  //It is possible during multi-camera acquisition that, although frames are acquired synchronously,
  //they are not received by the PC synchronously
  //If this happens that there is an unequal frame count, that extra frame is placed in the leftholder
  //structure and they are
  char* leftover;
  bool left_over_flag;

  //Indicates the starting index in the ring buffer
  int offset

  bool read_frame[MAX_CAMERA];
  int buf_id[MAX_CAMERA];

  FILE* ffmpeg;
};

void init_pylon();


#endif // BASLERCPP_H
