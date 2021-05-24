#ifndef __TCP_RECEIVE_SOCKET__
#define __TCP_RECEIVE_SOCKET__

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "practical_socket.h"
#include <glog/logging.h>

#include "packet_header.h"
#include "receive_interface.h"

namespace relay {
namespace communication {

class TCPReceiveSocket : public ReceiveInterface {
 public:
  TCPReceiveSocket(TCPSocket* sock, size_t max_buffer_size = 10);

  void stop();
  void consume(std::function<void(std::deque<std::string>&)> fun) override;
  bool running();

  void join();
  void detach();
  
 protected:
  std::atomic_bool running_;
  std::mutex mutex_;
  size_t max_buffer_size_ = 10;

  uint32_t index_ = 0;
  std::thread thread_;

  std::chrono::system_clock::time_point received_time_;
  const int timeout_ms = 1000;

  std::deque<std::string> buffer_;

  static constexpr size_t receive_buffer_size_ = 87380;
  char receive_buffer_[receive_buffer_size_];

  void worker(TCPSocket* sock);
};

}
}

#endif
