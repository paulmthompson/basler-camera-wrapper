# Basler Camera Wrapper

## OBSELETE

This is old code that was used to control high speed cameras and save with FFMPEG. I now use this library:  
https://github.com/paulmthompson/CameraManager


### Compiling

You can use the included docker file to compile the shared library.

Run with
```
sudo docker run --name camera_sh --rm -i -t basler-camera-wrapper:5.2 sh
```
```
sudo docker cp camera_sh:BaslerCpp.so /home/wanglab/BaslerCamera.so
```
