#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <cuda.h>
#include <signal.h>
#include <unistd.h>

#include "practical_socket.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "NvCodecUtils.h"
#include "udp_send_socket.h"
#include "nv_hevc_encoder.h"

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");
DEFINE_int32(port, 9000, "Local port");
DEFINE_string(video, "sample.mp4", "Video path file");

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

    relay::communication::UDPSendSocket modi_sock(FLAGS_port);
    modi_sock.setForeign(FLAGS_foreign_addr, FLAGS_foreign_port);

    cv::VideoCapture cap;
    if (!cap.open(FLAGS_video)) {
      LOG(FATAL) << "Cannot open video file";  
    }
    int frame_num = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    int frame_counter = 0;

    int iGpu = 0;
    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    if (iGpu < 0 || iGpu >= nGpu) {
        LOG(FATAL) << "GPU ordinal out of range. Should be within"
                    " [" << 0 << ", " << nGpu - 1 << "]";
    }

    CUcontext cuContext = NULL;
    createCudaContext(&cuContext, iGpu, 0);

    relay::codec::NvHEVCEncoder encoder(cuContext);

    while (modi_sock.running()) {
      if (running.load()) {
        auto start = std::chrono::system_clock::now();

        cv::Mat frame;
        cap >> frame; 
        frame_counter++;
        if (frame_counter == frame_num) {
            frame_counter = 0;
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        }

        if (frame.rows <= 0 || frame.cols <= 0) {
          running.store(false);
          modi_sock.stop();
          break;
        }

        if (frame.rows != 360 || frame.cols != 640) {
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
        }

        std::chrono::milliseconds ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch());
        std::string msg = std::to_string(ms.count());
        cv::putText(frame, 
                    msg.c_str(), 
                    cvPoint(30,30), 
                    cv::FONT_HERSHEY_COMPLEX_SMALL, 
                    0.8, 
                    cvScalar(200, 200, 250), 
                    1, 
                    cv::LINE_AA);

        cv::imshow("camera", frame);
        cv::waitKey(1);
        cv::cvtColor(frame, frame, CV_BGR2YUV_I420);

        encoder.push(640 * 360 * 3 / 2, reinterpret_cast<char*>(frame.data));
        
        encoder.consume([&](std::deque<std::string>& buffer) {
          if (!buffer.empty()) {
            modi_sock.push(buffer.front().size(), buffer.front().c_str());
            buffer.pop_front();
          }
        });

        std::this_thread::sleep_until(start + std::chrono::milliseconds(100));
      } else {
        modi_sock.stop();
      }
    }
    modi_sock.join();
    encoder.stop();
    encoder.join();
  } catch (const std::exception& e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}