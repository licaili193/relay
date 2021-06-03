#ifndef __SHM_SOCKET__
#define __SHM_SOCKET__

#include <string>

#include <glog/logging.h>

#include "shm.h"

namespace relay {
namespace communication {

template<class SendT, class ReceiveT>
class SHMSocket {
 public:
  SHMSocket(std::string send_channel, std::string receive_channel) {
    receive_q_ = shmmap<SPMCQueue<ReceiveT, 1>>(receive_channel.c_str());
    if (receive_q_) {
      reader_ = receive_q_->getReader();
    }
    send_q_ = shmmap<SPMCQueue<SendT, 1>>(send_channel.c_str());
  }

  void send(const SendT& to_send) {
    if (!send_q_) {
      LOG(ERROR) << "Cannot initiate send queue";
    }
    send_q_->write([to_send](SendT& msg) {
      msg = to_send;
    });
  }

  ReceiveT* receive() {
    if (!receive_q_) {
       LOG(ERROR) << "Cannot initiate receive queue";
       return nullptr;
    }
    return reader_.read();
  }

 private:
  SPMCQueue<SendT, 1>* send_q_ = nullptr;
  SPMCQueue<ReceiveT, 1>* receive_q_ = nullptr;
  typename SPMCQueue<ReceiveT, 1>::Reader reader_;
};

}    
}

#endif
