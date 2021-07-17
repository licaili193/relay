#include <cmath>

#include "nv_hevc_sync_decoder.h"

namespace relay {
namespace codec {

NvHEVCSyncDecoder::NvHEVCSyncDecoder(CUcontext cu_context) {
  cu_context_ = cu_context;

  pDec = std::unique_ptr<NvDecoder>(
      new NvDecoder(cu_context_, false, cudaVideoCodec_HEVC, NULL, true));
  n = 0;
}

void NvHEVCSyncDecoder::push(size_t payload_size, const char* payload) {
    if (payload_size) {
      int nVideoBytes = 0;
      nFrameReturned = 0;
      i = 0;
      int64_t *pTimestamp;
      try {
        const uint32_t tries = 3;
        for (uint32_t i = 0; (i < tries) && (nFrameReturned <= 0); ++i) {
          pDec->Decode(
            reinterpret_cast<const uint8_t*>(payload), 
          payload_size, 
          &ppFrame, 
          &nFrameReturned, 
          CUVID_PKT_ENDOFPICTURE, 
          &pTimestamp, 
          n++);
        }
      } catch (NVDECException& e) {
        std::string err = "Decode failed (" + 
                          e.getErrorString() + 
                          ", error " + 
                          std::to_string(e.getErrorCode()) + 
                          ")";
        LOG(ERROR) << err;
      }
    }
}

bool NvHEVCSyncDecoder::get(size_t& result_size, char* result) {
    if (i < nFrameReturned) {
        int frame_size = pDec->GetFrameSize();
        result_size = pDec->GetFrameSize();
        memcpy(result, ppFrame[i], result_size);
        i++;
        return true;
    }
    return false;
}

}
}
