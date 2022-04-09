#ifndef __CAMERA_GRABBER__
#define __CAMERA_GRABBER__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#include <fstream>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>

#include <cuda.h>
#include <cuda_runtime.h>

class CameraGrabber {
 public:
  CameraGrabber(CUcontext* cuda_ctx, uint8_t* d_img_yuyv, std::string camera_name, int width, int height) {
    cuda_ctx_ = cuda_ctx;
    camera_name_ = camera_name;
    // TODO: unify the setting with camera name
    width_ = width;
    height_ = height;
    d_img_yuyv_ = d_img_yuyv;
    running_.store(false);
    new_image_.store(false);

    out_buffer_size_ = width_ * height_ * 2;
    out_buffer_ = new char[out_buffer_size_];
  }

  ~CameraGrabber() {
    if (t_.joinable()) {
      t_.join();
    }

    delete[] out_buffer_;
  }

  void startStream() {
    std::thread t(&CameraGrabber::worker, this);
    std::swap(t, t_);
  }

  void stopStream() {
    running_.store(false);
  }

  bool newImage() {
    bool res = new_image_.load();
    if (res) {
      new_image_.store(false);
    }
    return res;
  }

 private:
  void worker();

  void cleanup();

  CUcontext* cuda_ctx_ = nullptr;

  uint8_t* d_img_yuyv_ = nullptr;
  std::string camera_name_ = "/dev/video0";

  size_t width_ = 640;
  size_t height_ = 480;

  std::atomic_bool running_;
  std::thread t_;
  std::atomic_bool new_image_;

  char* out_buffer_;
  size_t out_buffer_size_ = 0;
};

#endif
