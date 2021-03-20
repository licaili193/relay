#ifndef __NV_HEVC_DECODER__
#define __NV_HEVC_DECODER__

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <cuda.h>
#include <string.h>
#include <glog/logging.h>

#include "NvDecoder.h"
#include "async_payload_framework.h"

namespace relay {
namespace codec {

class NvHEVCDecoder : public AsyncPayloadFramework {
 public:
  NvHEVCDecoder(CUcontext cu_context);

 protected:
  uint32_t index_ = 0;

  CUcontext cu_context_ = NULL;

  virtual void worker() override;
};

}
}

#endif
