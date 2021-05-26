#ifndef __JSMEPG_DECODER__
#define __JSMEPG_DECODER__

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}
#include <glog/logging.h>

#include "async_payload_framework.h"

namespace relay {
namespace codec {

class JsmpegEncoder : public AsyncPayloadFramework {
 public:
  JsmpegEncoder(uint16_t width = 640, 
              uint16_t height = 480, 
              float framerate = 30, 
              uint32_t bitrate = 400000);

 protected:
  uint16_t width_ = 640;
  uint16_t height_ = 480;
  float framerate_ = 30;
  uint32_t bitrate_ = 400000;

  uint16_t max_b_frames_ = 3;
  uint16_t gop_size_ = 30;

  uint32_t index_ = 0;

  const AVPixelFormat input_format_ = AV_PIX_FMT_YUV420P;
  const AVCodecID codec_id_ = AV_CODEC_ID_MPEG1VIDEO;

  virtual void worker() override;
};

}
}

#endif
