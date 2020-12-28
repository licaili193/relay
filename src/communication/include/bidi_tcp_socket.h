#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "practical_socket.h"
#include <glog/logging.h>

namespace relay {
namespace communication {

struct FrameHeader {

};

class BiDirectionalTCPSocket {
 public:
  BiDirectionalTCPSocket(TCPSocket* sock);

  void stop();
  
 protected:
  std::atomic_bool running_;

  std::mutex mutex_;

  void worker(TCPSocket* sock);
};

}
}