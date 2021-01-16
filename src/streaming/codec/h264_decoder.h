#ifndef __H264_DECODER__
#define __H264_DECODER__

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
#include "libswscale/swscale.h"
}
#include <glog/logging.h>

#include "async_payload_framework.h"

namespace relay {
namespace codec {

class H264Decoder : public AsyncPayloadFramework {
 public:
  H264Decoder();

  virtual void push(size_t payload_size, const char* payload) override;
  virtual void push(std::string payload) override;

 protected:
  uint32_t index_ = 0;

  const AVPixelFormat output_format_ = AV_PIX_FMT_YUV420P;
  const AVCodecID codec_id_ = AV_CODEC_ID_H264;

  virtual void worker() override;
};

}
}

#endif
