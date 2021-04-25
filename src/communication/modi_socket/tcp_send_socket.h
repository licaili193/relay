#ifndef __TCP_SEND_SOCKET__
#define __TCP_SEND_SOCKET__

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

class TCPSendSocket {
 public:
  TCPSendSocket(TCPSocket* sock, size_t max_buffer_size = 10);

  void stop();
  void push(size_t payload_size, const char* payload);
  void push(std::string payload);
  bool running();

  void join();
  void detach();
  
 protected:
  std::atomic_bool running_;
  std::mutex mutex_;
  size_t max_buffer_size_ = 10;

  std::thread thread_;

  std::deque<std::string> buffer_;

  void worker(TCPSocket* sock);
};

}
}

#endif
