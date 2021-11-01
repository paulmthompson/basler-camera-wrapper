
#include "BaslerCpp.h"
#include "BaslerCWrapper.h"
#include <stdint.h>

extern "C" {

  MyCamera* newMyCamera(int num_cam) {
    return new MyCamera(num_cam);
  }

  void MyCamera_Connect(MyCamera* cam) {
    cam->Connect();
  }
  void MyCamera_ConnectBySerial(MyCamera* cam, const char *myserial) {
    cam->ConnectBySerial(myserial);
  }
  void MyCamera_StartAcquisition(MyCamera* cam) {
    cam->StartAcquisition();
  }
  void MyCamera_StopAcquisition(MyCamera* cam) {
    cam->StopAcquisition();
  }
  void MyCamera_GrabFrames(MyCamera* cam) {
    cam->GrabFrames();
  }
  void MyCamera_StartFFMPEG(MyCamera* cam) {
    cam->StartFFMPEG();
  }
  void MyCamera_EndFFMPEG(MyCamera* cam) {
    cam->EndFFMPEG();
  }
  void MyCamera_ChangeFolder(MyCamera* cam, const char *folder) {
    cam->ChangeFolder(folder);
  }

  void MyCamera_UpdateSaveName(MyCamera* cam, const char *name) {
    cam->UpdateSaveName(name);
  }

  bool MyCamera_GetFramesGrabbed(MyCamera* cam) {
    return cam->framesGrabbed;
  }

  char* MyCamera_GetData(MyCamera* cam) {
    return cam->img_to_display;
  }

  void MyCamera_GetDataBuffer(MyCamera* cam, uint16_t* data) {
	  memcpy(data, cam->img_to_display, 2 * cam->_w * cam->_h);
  }

  void MyCamera_changeResolution(MyCamera* cam, int w, int h) {
    cam->_h = h;
    cam->_w = w;

	cam->resizeImage();
  }

  void MyCamera_SetTrialStructure(MyCamera* cam, int _structure) {
    cam->SetTrialStructure(_structure);
  }

  void MyCamera_ChangeCameraConfig(MyCamera* cam, const char *path) {
    cam->ChangeCameraConfig(path);
  }

  void MyCamera_ChangeFFMPEG(MyCamera* cam, const char *path) {
    cam->ChangeFFMPEG(path);
  }

  void MyCamera_ChangeFFMPEGInputOptions(MyCamera* cam, const char *cmd) {
    cam->ChangeFFMPEGInputOptions(cmd);
  }

  void MyCamera_ChangeFFMPEGOutputOptions(MyCamera* cam, const char *cmd) {
    cam->ChangeFFMPEGOutputOptions(cmd);
  }

  void MyCamera_ChangeBytes(MyCamera* cam, int _bytes) {
    cam->ChangeBytes(_bytes);
  }

  void deleteMyCamera(MyCamera* cam) {
    delete cam;
  }

  void initPylon() {
    init_pylon();
  }

}
