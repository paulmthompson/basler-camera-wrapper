#ifndef __BASLERCWRAPPER_H
#define __BASLERCWRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MyCamera MyCamera;

#ifdef _WIN32
	#define DLLOPT __declspec(dllexport)
#else
	#define DLLOPT __attribute__((visibility("default")))
#endif

DLLOPT MyCamera* newMyCamera(int num_cam);

DLLOPT  void MyCamera_Connect(MyCamera* cam);
DLLOPT	void MyCamera_ConnectBySerial(MyCamera* cam, const char *myserial);
DLLOPT  void MyCamera_StartAcquisition(MyCamera* cam);
DLLOPT  void MyCamera_StopAcquisition(MyCamera* cam);
DLLOPT  void MyCamera_GrabFrames(MyCamera* cam);
DLLOPT  void MyCamera_StartFFMPEG(MyCamera* cam);
DLLOPT  void MyCamera_EndFFMPEG(MyCamera* cam);
DLLOPT  void MyCamera_ChangeFolder(MyCamera* cam, const char *folder);
DLLOPT  void MyCamera_UpdateSaveName(MyCamera* cam, const char *name);
DLLOPT  bool MyCamera_GetFramesGrabbed(MyCamera* cam);
DLLOPT  char* MyCamera_GetData(MyCamera* cam);
DLLOPT  void MyCamera_GetDataBuffer(MyCamera* cam, uint16_t* data);
DLLOPT  void MyCamera_changeResolution(MyCamera* cam, int w, int h);
DLLOPT  void MyCamera_SetTrialStructure(MyCamera* cam, int _structure);
DLLOPT	void MyCamera_ChangeCameraConfig(MyCamera* cam, const char *path);
DLLOPT	void MyCamera_ChangeFFMPEG(MyCamera* cam, const char *path);
DLLOPT	void MyCamera_ChangeFFMPEGInputOptions(MyCamera* cam, const char *cmd);
DLLOPT 	void MyCamera_ChangeFFMPEGOutputOptions(MyCamera* cam, const char *cmd);
DLLOPT 	void MyCamera_ChangeBytes(MyCamera* cam, int _bytes);

DLLOPT  void deleteMyCamera(MyCamera* cam);

DLLOPT  void initPylon();

#ifdef __cplusplus
}
#endif
#endif
