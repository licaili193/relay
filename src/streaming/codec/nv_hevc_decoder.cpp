#include <cmath>

#include "nv_hevc_decoder.h"

namespace relay {
namespace codec {

NvHEVCDecoder::NvHEVCDecoder(CUcontext cu_context) {
  cu_context_ = cu_context;

  running_.store(true);
  thread_ = std::thread(&NvHEVCDecoder::worker, this);
}

void NvHEVCDecoder::worker() {
  NvDecoder dec(cu_context_, false, cudaVideoCodec_HEVC, NULL, true);

  int n = 0;
  while (running_.load()) {
    std::string payload;
    bool has_payload = false;
    {
      std::lock_guard<std::mutex> guard(recv_mutex_);
      if (!recv_buffer_.empty()) {
        payload = std::move(recv_buffer_.front());
        recv_buffer_.pop_front();
        has_payload = true;
      }
    }
    if (has_payload) {
      int nVideoBytes = 0, nFrameReturned = 0;
      uint8_t **ppFrame;
      int64_t *pTimestamp;
      try {
        const uint32_t tries = 3;
        for (uint32_t i = 0; (i < tries) && (nFrameReturned <= 0); ++i) {
          dec.Decode(
          reinterpret_cast<const uint8_t*>(payload.c_str()), 
          payload.size(), 
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
      
      for (int i = 0; i < nFrameReturned; i++) {
        uint8_t *pFrame = ppFrame[i];
        int frame_size = dec.GetFrameSize();
        std::string temp(frame_size, 0);
        memcpy(const_cast<char*>(temp.c_str()), 
               pFrame, 
               std::min(dec.GetFrameSize(), frame_size));
        std::lock_guard<std::mutex> guard(send_mutex_);
        if (send_buffer_.size() >= max_buffer_size_) {
          send_buffer_.pop_front();
        }
        send_buffer_.push_back(std::move(temp));
      }
      index_ += nFrameReturned;
    }
  }
}

}
}
