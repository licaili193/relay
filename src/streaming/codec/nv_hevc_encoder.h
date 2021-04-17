#ifndef __NV_HEVC_ENCODER__
#define __NV_HEVC_ENCODER__

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <cuda.h>
#include <string.h>
#include <glog/logging.h>

#include "NvEncoderCuda.h"
#include "async_payload_framework.h"

namespace relay {
namespace codec {

class NvHEVCEncoder : public AsyncPayloadFramework {
 public:
  NvHEVCEncoder(CUcontext cu_context,
                uint16_t width = 640, 
                uint16_t height = 360, 
                float framerate = 10, 
                uint32_t bitrate = 100000);

 protected:
  uint16_t width_ = 640;
  uint16_t height_ = 360;
  float framerate_ = 10;
  uint32_t bitrate_ = 100000;

  uint32_t index_ = 0;

  NV_ENC_BUFFER_FORMAT buffer_format_ = NV_ENC_BUFFER_FORMAT_NV12;

  CUcontext cu_context_ = NULL;

  virtual void worker() override;
};

}
}

#endif
