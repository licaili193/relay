#ifndef __H264_ENCODER__
#define __H264_ENCODER__

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <string.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}
#include <glog/logging.h>

#include "async_payload_framework.h"

namespace relay {
namespace codec {

class H264Encoder : public AsyncPayloadFramework {
 public:
  H264Encoder(uint16_t width = 480, 
              uint16_t height = 360, 
              float framerate = 10, 
              uint32_t bitrate = 400000);

 protected:
  uint16_t width_ = 480;
  uint16_t height_ = 360;
  float framerate_ = 10;
  uint32_t bitrate_ = 400000;

  uint16_t max_b_frames_ = 3;
  uint16_t gop_size_ = 30;

  uint32_t index_ = 0;

  const AVPixelFormat input_format_ = AV_PIX_FMT_YUV420P;
  const AVCodecID codec_id_ = AV_CODEC_ID_H264;

  virtual void worker() override;
};

}
}

#endif
