
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "udp_send_socket.h"
#include "image_processing.h"

#include "camera_grabber.h"
#include "nv_hevc_sync_encoder.h"

// TODO: clean the include files

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");
DEFINE_string(camera_1, "/dev/video0", "Camera name");
// DEFINE_string(camera_2, "/dev/video1", "Camera name");
// DEFINE_string(camera_3, "/dev/video2", "Camera name");

constexpr int width = 640;
constexpr int height = 480;
constexpr size_t bitrate = 800000;
constexpr size_t idr_interval = 10;
constexpr float framerate = 15.f;
char* encoded_buffer = new char[640*480*3];

std::atomic_bool running;

CUcontext cuda_context;

void myHandler(int s){
  running.store(false);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = myHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  cudaSetDevice(0);
  auto d_img_yuyv = image_processing::allocateImageYUYV(width, height); // stream one for now
  CHECK(d_img_yuyv);
  auto d_img_yuv420 = image_processing::allocateImageYUV(width, height);
  CHECK(d_img_yuv420);
  unsigned char* img_yuv420 = new unsigned char[width * height * 3 / 2];
  CHECK(img_yuv420);

  CHECK_EQ(CUDA_SUCCESS, cuCtxGetCurrent(&cuda_context));
  CameraGrabber grabber_1(&cuda_context, d_img_yuyv, FLAGS_camera_1, width, height);

  // Create codec
  relay::codec::NvHEVCSyncEncoder encoder(cuda_context);

  // Create socket
  std::unique_ptr<relay::communication::UDPSendSocket> modi_sock =  
      std::unique_ptr<relay::communication::UDPSendSocket>(
          new relay::communication::UDPSendSocket(0, 10));
  modi_sock->setForeign(FLAGS_foreign_addr, FLAGS_foreign_port);

  running.store(true);
  grabber_1.startStream();

  /* Wait for camera event with timeout = 5000 ms */
  while (running.load()) {
    if (!grabber_1.newImage()) {
      // Only use grabber 1 to keep the frame rate
      continue;
    }

    image_processing::yuyv2YUV(width, height, d_img_yuyv, d_img_yuv420);
    image_processing::downloadImageYUV(width, height, d_img_yuv420, img_yuv420);

    // Encode frame
    encoder.push(width * height * 3 / 2, reinterpret_cast<char*>(img_yuv420));

    size_t result_size;
    while (encoder.get(result_size, encoded_buffer)) {
      modi_sock->push(result_size, encoded_buffer);
    }
  }

  grabber_1.stopStream();
  cudaFree(d_img_yuyv);
  cudaFree(d_img_yuv420);
  delete[] img_yuv420;

  LOG(INFO) << "Finished cleanly";

  return 0;
}
