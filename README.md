# Basler Camera Wrapper

I use Basler Cameras for high-speed imaging of animal behavior. I use this library to communicate
with cameras and send the data to FFMPEG for compression using the GPU. <br>
<br>
You can download Pylon, the Basler Camera software, from here: <br>
https://www.baslerweb.com/en/sales-support/downloads/software-downloads/#type=pylonsoftware;version=all <br>
<br>
You can use the included docker file to compile the shared library.

Run with
```
sudo docker run --name camera_sh --rm -i -t basler-camera-wrapper:5.2 sh
```
```
sudo docker cp camera_sh:BaslerCpp.so /home/wanglab/BaslerCamera.so
```
