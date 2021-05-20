#include <atomic>
#include <chrono>
#include <thread>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include <signal.h>
#include <unistd.h>

#include "NvCodecUtils.h"
#include "nv_hevc_decoder.h"
#include "nv_hevc_encoder.h"

DEFINE_string(video, "", "Video path file (if set, camera won't be used)");
DEFINE_int32(camera, 0, "Camera number");
DEFINE_int32(frame_cap, 0, "Stop running after the number of frames");
DEFINE_bool(save_image, false, "Save decoded frames to file");

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

  relay::codec::NvHEVCEncoder encoder(cuContext);
  relay::codec::NvHEVCDecoder decoder(cuContext);

  size_t index = 0;
  while (running.load()) {
    auto start = std::chrono::system_clock::now();
    
    encoder.consume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        LOG(INFO) << "Decoder push";
        decoder.push(std::move(buffer.back()));
        buffer.clear();
      }
    });
    
    cv::Mat frame;
    cap >> frame;

    if (frame.rows > 0 && frame.cols > 0) {
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
                  cvScalar(50, 50, 250), 
                  1, 
                  cv::LINE_AA);

      cv::imshow("camera", frame);
      cv::waitKey(1);
    } else {
      running.store(false);
      break;
    }

    cv::resize(frame, frame, cv::Size(640, 360), 0, 0, cv::INTER_LINEAR);
    cv::cvtColor(frame, frame, CV_BGR2YUV_I420);

    LOG(INFO) << "Encoder push";
    encoder.push(640 * 360 * 3 / 2, reinterpret_cast<char*>(frame.data));

    cv::Mat decoded_frame;
    bool got_decoded_frame = false;
    decoder.consume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        LOG(INFO) << "Decoder get";
        decoded_frame = cv::Mat(360 * 3 / 2, 640, CV_8UC1, const_cast<void*>(
            reinterpret_cast<const void*>(buffer.back().c_str())));
        buffer.clear();
        got_decoded_frame = true;
      }
    });
    if (got_decoded_frame) {
      cv::cvtColor(decoded_frame, decoded_frame, CV_YUV2BGR_I420);
      if (decoded_frame.rows > 0 && decoded_frame.cols > 0) {
        if (FLAGS_save_image) {
          std::string path = "decoded_" + std::to_string(index) + ".jpg";
          cv::imwrite(path, decoded_frame);
        }
        cv::imshow("decoded image", decoded_frame);
        cv::waitKey(1);
      } 
    }

    index++;
    std::this_thread::sleep_until(start + std::chrono::milliseconds(100));
    if (FLAGS_frame_cap > 0 && index > FLAGS_frame_cap) {
      running.store(false);
    }
  }
  encoder.stop();
  decoder.stop();
  encoder.join();
  decoder.join();
}
