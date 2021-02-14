#ifndef __BIDI_TCP_SOCKET__
#define __BIDI_TCP_SOCKET__

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "practical_socket.h"
#include <glog/logging.h>

#define COMMAND_NULL 0
#define COMMAND_DISCONNECT 1
#define COMMAND_PING 2

#define FIN_INDEX_KEY 12245
#define FIN_SIZE_KEY 5132466

#define PING_INDEX_KEY 23414
#define PING_SIZE_KEY 102304

namespace relay {
namespace communication {

struct FrameHeader {
  uint32_t id;
  uint32_t length;
  uint8_t command;

  void makeFrameHeader(char buffer[9]);
  static FrameHeader parseFrameHeader(const char buffer[9]);

  static const size_t header_size;
};

class BiDirectionalTCPSocket {
 public:
  BiDirectionalTCPSocket(
      TCPSocket* sock, size_t max_buffer_size = 10, double ping_period_sec = 1);

  void stop();
  void push(size_t payload_size, const char* payload);
  void push(std::string payload);
  void comsume(std::function<void(std::deque<std::string>&)> fun);
  bool running();

  void join();
  void detach();
  
 protected:
  std::atomic_bool running_;
  std::mutex recv_mutex_;
  std::mutex send_mutex_;
  size_t max_buffer_size_ = 10;

  uint32_t index_ = 0;
  std::thread thread_;

  std::deque<std::string> recv_buffer_;
  std::deque<std::string> send_buffer_;

  std::chrono::duration<double> ping_period_;
  std::chrono::time_point<std::chrono::system_clock> ping_time_;

  void worker(TCPSocket* sock);
  virtual bool isCorrectHeader(char* buffer, int size, FrameHeader& header);

  static constexpr size_t buffer_size = 87380;
};

}
}

#endif
