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

class H264Decoder : public AsyncPayloadFramework {
 public:
  H264Decoder();

  virtual void push(size_t payload_size, const char* payload) override;
  AVPixelFormat outputFormat() const;

 protected:
  uint32_t index_ = 0;

  AVPixelFormat output_format_;
  const AVCodecID codec_id_ = AV_CODEC_ID_H264;

  std::mutex format_mutex_;

  virtual void worker() override;
};

}
}
