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

namespace relay {
namespace communication {

constexpr size_t buffer_size = 1024;

class TCPReceiveSocket {
 public:
  TCPReceiveSocket(TCPSocket* sock, size_t max_buffer_size = 100);

  void stop();
  void comsume(std::function<void(std::deque<std::string>&)> fun);
  bool running();

  void join();
  void detach();
  
 protected:
  std::atomic_bool running_;
  std::mutex mutex_;
  size_t max_buffer_size_ = 100;

  uint32_t index_ = 0;
  std::thread thread_;

  std::deque<std::string> buffer_;

  void worker(TCPSocket* sock);
};

}
}

#endif
