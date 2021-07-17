#ifndef __NV_HEVC_SYNC_DECODER__
#define __NV_HEVC_SYNC_DECODER__

#include <memory>

#include <cuda.h>
#include <string.h>
#include <glog/logging.h>

#include "NvDecoder.h"

namespace relay {
namespace codec {

class NvHEVCSyncDecoder {
 public:
  NvHEVCSyncDecoder(CUcontext cu_context);

  void push(size_t payload_size, const char* payload);
  bool get(size_t& result_size, char* result);

 private:
  CUcontext cu_context_ = NULL;

  int n = 0;
  std::unique_ptr<NvDecoder> pDec;
  int nFrameReturned = 0;
  int i = 0;
  uint8_t **ppFrame;
};

}
}

#endif
