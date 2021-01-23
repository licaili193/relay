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
#include "h264_decoder.h"
#include "h264_encoder.h"

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");

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

    relay::codec::H264Decoder decoder;

    while (bidi_sock.running()) {
      if (running.load()) {
        bidi_sock.comsume([&](std::deque<std::string>& buffer){
          while (!buffer.empty()) {
            decoder.push(std::move(buffer.front()));
            buffer.pop_front();
          }
        });

        cv::Mat decoded_frame;
        bool got_decoded_frame = false;
        decoder.comsume([&](std::deque<std::string>& buffer) {
          if (!buffer.empty()) {
            decoded_frame = cv::Mat(
                360 * 3 / 2, 640, CV_8UC1, const_cast<void*>(
                    reinterpret_cast<const void*>(buffer.front().c_str()))).
                        clone();
            buffer.pop_front();
            got_decoded_frame = true;
          }
        });
        if (got_decoded_frame) {
          cv::cvtColor(decoded_frame, decoded_frame, CV_YUV2BGR_I420);
          if (decoded_frame.rows > 0 && decoded_frame.cols > 0) {
            cv::imshow("decoded image", decoded_frame);
            cv::waitKey(1);
          } 
        }
      } else {
        bidi_sock.stop();
      }
    }
    bidi_sock.join();
    decoder.stop();
    decoder.join();
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}