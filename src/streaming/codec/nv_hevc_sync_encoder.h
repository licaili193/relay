#ifndef __NV_HEVC_SYNC_ENCODER__
#define __NV_HEVC_SYNC_ENCODER__

#include <memory>
#include <vector>

#include <cuda.h>
#include <string.h>
#include <glog/logging.h>

#include "NvEncoderCuda.h"

namespace relay {
namespace codec {

class NvHEVCSyncEncoder {
 public:
  NvHEVCSyncEncoder(CUcontext cu_context,
                    uint16_t width = 640, 
                    uint16_t height = 480, 
                    float framerate = 30, 
                    uint32_t bitrate = 800000);
  ~NvHEVCSyncEncoder();

  void push(size_t payload_size, const char* payload);
  bool get(size_t& result_size, char* result);

 private:
  uint16_t width_ = 640;
  uint16_t height_ = 480;
  float framerate_ = 30;
  uint32_t bitrate_ = 800000;

  uint32_t idr_interval_ = 50;
  uint32_t index_ = 0;

  NV_ENC_BUFFER_FORMAT buffer_format_ = NV_ENC_BUFFER_FORMAT_NV12;

  CUcontext cu_context_ = NULL;

  std::unique_ptr<NvEncoderCuda> pEnc;
  int nFrameSize = 0;
  std::vector<std::vector<uint8_t>> vPacket;
};

}
}

#endif