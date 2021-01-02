#include <atomic>
#include <chrono>
#include <thread>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include <signal.h>
#include <unistd.h>

#include "h264_decoder.h"
#include "h264_encoder.h"

DEFINE_int32(camera, 0, "Camera number");

std::atomic_bool running;

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

  running.store(true);

  cv::VideoCapture cap;
  if(!cap.open(FLAGS_camera)) {
    LOG(FATAL) << "Cannot open camera";
  }

  relay::codec::H264Encoder encoder;
  relay::codec::H264Encoder decoder;

  while (running.load()) {
    auto start = std::chrono::system_clock::now();
    
    encoder.comsume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        decoder.push(std::move(buffer.front()));
        buffer.pop_front();
      }
    });
    
    cv::Mat frame;
    cap >> frame;        
    cv::imshow("camera", frame); 

    cv::resize(frame, frame, cv::Size(640, 360), 0, 0, cv::INTER_LINEAR);
    cv::cvtColor(frame, frame, CV_BGR2YUV_I420);

    encoder.push(640 * 360 * 3 / 2, reinterpret_cast<char*>(frame.data));

    cv::Mat decoded_frame;
    decoder.comsume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        decoded_frame = cv::Mat(360, 480, CV_8UC3, const_cast<void*>(
            reinterpret_cast<const void*>(buffer.front().c_str())));
        buffer.pop_front();
      }
    });
    cv::imshow("decoded image", decoded_frame); 

    std::this_thread::sleep_until(start + std::chrono::milliseconds(100));
  }
  encoder.stop();
  decoder.stop();
  encoder.join();
  decoder.join();
}
