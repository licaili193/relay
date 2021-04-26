#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <signal.h>
#include <unistd.h>

#include "practical_socket.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "bidi_tcp_socket.h"
#include "jsmpeg_encoder.h"

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");
DEFINE_string(video, "", "Video path file (if set, camera won't be used)");
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

  try {
    LOG(INFO) << "Connecting to server: "
              << FLAGS_foreign_addr 
              << ":" 
              << FLAGS_foreign_port;

    relay::communication::BiDirectionalTCPSocket bidi_sock(
        new TCPSocket(FLAGS_foreign_addr, FLAGS_foreign_port));

    cv::VideoCapture cap;
    if (FLAGS_video != "") {
        if (!cap.open(FLAGS_video)) {
        LOG(FATAL) << "Cannot open video file";  
        }
    } else {
        if (!cap.open(FLAGS_camera)) {
        LOG(FATAL) << "Cannot open camera";
        }
    }

    relay::codec::JsmpegEncoder encoder;

    while (bidi_sock.running()) {
      if (running.load()) {
        auto start = std::chrono::system_clock::now();

        cv::Mat frame;
        cap >> frame;        
        if (frame.rows <= 0 || frame.cols <= 0) {
          running.store(false);
          bidi_sock.stop();
          break;
        }

        double aspect_ratio = 
            static_cast<double>(frame.cols) / static_cast<double>(frame.rows);
        if (aspect_ratio > 640.0 / 360.0) {
          double scale = 360.0 / static_cast<double>(frame.rows);  
          cv::resize(
              frame, frame, cv::Size(0, 0), scale, scale, cv::INTER_LINEAR);
          frame = frame(cv::Rect((frame.cols - 640) / 2, 0, 640, 360));
        } else {
          double scale = 640.0 / static_cast<double>(frame.cols);  
          cv::resize(
              frame, frame, cv::Size(0, 0), scale, scale, cv::INTER_LINEAR);
          frame = frame(cv::Rect(0, (frame.rows - 360) / 2, 640, 360));
        }
        cv::imshow("camera", frame);
        cv::waitKey(1);
        cv::cvtColor(frame, frame, CV_BGR2YUV_I420);

        encoder.push(640 * 360 * 3 / 2, reinterpret_cast<char*>(frame.data));
        
        encoder.consume([&](std::deque<std::string>& buffer) {
          if (!buffer.empty()) {
            bidi_sock.push(buffer.front().size(), buffer.front().c_str());
            buffer.pop_front();
          }
        });

        std::this_thread::sleep_until(start + std::chrono::milliseconds(33));
      } else {
        bidi_sock.stop();
      }
    }
    bidi_sock.join();
    encoder.stop();
    encoder.join();
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}