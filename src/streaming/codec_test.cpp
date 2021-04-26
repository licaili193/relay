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

  cv::namedWindow("camera", cv::WINDOW_AUTOSIZE);
  cv::namedWindow("decoded image", cv::WINDOW_AUTOSIZE);

  relay::codec::H264Encoder encoder;
  relay::codec::H264Decoder decoder;

  while (running.load()) {
    auto start = std::chrono::system_clock::now();
    
    encoder.consume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        decoder.push(std::move(buffer.front()));
        buffer.pop_front();
      }
    });
    
    cv::Mat frame;
    cap >> frame;        
    if (frame.rows > 0 && frame.cols > 0) {
      cv::imshow("camera", frame);
      cv::waitKey(1);
    } else {
      running.store(false);
      break;
    }

    cv::resize(frame, frame, cv::Size(640, 360), 0, 0, cv::INTER_LINEAR);
    cv::cvtColor(frame, frame, CV_BGR2YUV_I420);

    encoder.push(640 * 360 * 3 / 2, reinterpret_cast<char*>(frame.data));

    cv::Mat decoded_frame;
    bool got_decoded_frame = false;
    decoder.consume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        decoded_frame = cv::Mat(360 * 3 / 2, 640, CV_8UC1, const_cast<void*>(
            reinterpret_cast<const void*>(buffer.front().c_str())));
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

    std::this_thread::sleep_until(start + std::chrono::milliseconds(100));
  }
  encoder.stop();
  decoder.stop();
  encoder.join();
  decoder.join();
}
