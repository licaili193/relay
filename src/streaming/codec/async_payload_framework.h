#ifndef __ASYNC_PAYLOAD_FRAMEWORK__
#define __ASYNC_PAYLOAD_FRAMEWORK__

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <glog/logging.h>

namespace relay {
namespace codec {

class AsyncPayloadFramework {
 public:
  void stop();
  virtual void push(size_t payload_size, const char* payload);
  virtual void push(std::string payload);
  void consume(std::function<void(std::deque<std::string>&)> fun);
  bool running();

  void join();
  void detach();

 protected:
  std::atomic_bool running_;
  std::mutex recv_mutex_;
  std::mutex send_mutex_;
  size_t max_buffer_size_ = 30;

  std::thread thread_;

  std::deque<std::string> recv_buffer_;
  std::deque<std::string> send_buffer_;

  virtual void worker();
};

}
}

#endif
