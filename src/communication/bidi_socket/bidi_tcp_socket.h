#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "practical_socket.h"
#include <glog/logging.h>

#define COMMAND_NULL 0
#define COMMAND_DISCONNECT 1

#define FIN_INDEX_KEY 12245
#define FIN_SIZE_KEY 5132466

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
  BiDirectionalTCPSocket(TCPSocket* sock, size_t max_buffer_size = 10);

  void stop();
  void push(size_t payload_size, const char* payload);
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

  void worker(TCPSocket* sock);
};

}
}