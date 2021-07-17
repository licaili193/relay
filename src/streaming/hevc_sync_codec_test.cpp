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
#include "nv_hevc_sync_decoder.h"
#include "nv_hevc_sync_encoder.h"

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

  relay::codec::NvHEVCSyncEncoder encoder(cuContext);
  relay::codec::NvHEVCSyncDecoder decoder(cuContext);

  char* encoded_buffer = new char[640*480*3];
  char* decoded_buffer = new char[640*480*3];

  size_t index = 0;
  while (running.load()) {
    auto start = std::chrono::system_clock::now();
    
    cv::Mat frame;
    cap >> frame;

    if (frame.rows > 0 && frame.cols > 0) {
      if (frame.rows != 480 || frame.cols != 640) {
        double aspect_ratio = 
            static_cast<double>(frame.cols) / static_cast<double>(frame.rows);
        if (aspect_ratio > 640.0 / 480.0) {
            double scale = 480.0 / static_cast<double>(frame.rows);  
          cv::resize(
              frame, frame, cv::Size(0, 0), scale, scale, cv::INTER_LINEAR);
          frame = frame(cv::Rect((frame.cols - 640) / 2, 0, 640, 480));
        } else {
          double scale = 640.0 / static_cast<double>(frame.cols);  
          cv::resize(
              frame, frame, cv::Size(0, 0), scale, scale, cv::INTER_LINEAR);
          frame = frame(cv::Rect(0, (frame.rows - 480) / 2, 640, 480));
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

    cv::resize(frame, frame, cv::Size(640, 480), 0, 0, cv::INTER_LINEAR);
    cv::cvtColor(frame, frame, CV_BGR2YUV_I420);

    LOG(INFO) << "Encoder push";
    encoder.push(640 * 480 * 3 / 2, reinterpret_cast<char*>(frame.data));

    size_t result_size;
    while (encoder.get(result_size, encoded_buffer)) {
      LOG(INFO) << "Encoder get: " << result_size;
      size_t decoded_result_size;
      LOG(INFO) << "Decoder push";
      decoder.push(result_size, encoded_buffer);
      while (decoder.get(decoded_result_size, decoded_buffer)) {
        LOG(INFO) << "Decoder get: " << decoded_result_size;
        cv::Mat decoded_frame = 
            cv::Mat(480 * 3 / 2, 640, CV_8UC1, const_cast<void*>(
                reinterpret_cast<const void*>(decoded_buffer)));
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
    }

    index++;
    std::this_thread::sleep_until(start + std::chrono::milliseconds(33));
    if (FLAGS_frame_cap > 0 && index > FLAGS_frame_cap) {
      running.store(false);
    }
  }
  delete[] encoded_buffer;
  delete[] decoded_buffer;
}
