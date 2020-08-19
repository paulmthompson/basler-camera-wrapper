#ifndef BASLERCONFIG_H
#define BASLERCONFIG_H

#include <string>

static constexpr int MAX_CAMERA = 1;
static constexpr int frame_buf_size = 100;

static constexpr float default_frame_rate = 500.0;

static std::string default_camera_config_name = "default.pfs";

//static constexpr char ffmpeg_filepath[] = "ffmpeg";
static constexpr char ffmpeg_filepath[] = "c:/Users/wanglab/Downloads/ffmpeg/bin/ffmpeg"; //intrinsic

//static constexpr char ffmpeg_options[] = "-threads 4 -f rawvideo -pix_fmt gray -s";
static constexpr char ffmpeg_options[] = "-f rawvideo -pix_fmt gray16 -s"; //intrinsic

//static constexpr char ffmpeg_cmd[] = "-i - -y -pix_fmt nv12 -vcodec h264_nvenc";
static constexpr char ffmpeg_cmd[] = "-i - -y -pix_fmt gray16 -compression_algo raw"; //intrinsic

//static constexpr int trial_structure = 0;
static constexpr int trial_structure = 1;

//static constexpr int bytes_per_pixel = 1;
static constexpr int bytes_per_pixel = 2;

#endif // BASLERCPP_H
