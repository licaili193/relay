#include <memory>
#include <string.h>

#include "tcp_receive_socket.h"

namespace relay {
namespace communication {

TCPReceiveSocket::TCPReceiveSocket(
    TCPSocket* sock, size_t max_buffer_size) {
  max_buffer_size_ = max_buffer_size;

  running_.store(true);
  thread_ = std::thread(&TCPReceiveSocket::worker, this, sock);
}

void TCPReceiveSocket::worker(TCPSocket* sock) {
  char buffer[buffer_size];

  std::unique_ptr<TCPSocket>socket(sock);
  socket->setBlocking(false);
  try {
    while (running_.load()) {
      socket->check();

      int recv_sz = 0;
      while (true) {
        recv_sz = socket->recv(buffer, buffer_size);
        if (recv_sz <= 0) {
          break;
        }

        std::lock_guard<std::mutex> guard(mutex_);
        if (buffer_.size() >= max_buffer_size_) {
          buffer_.pop_front();
        }
        buffer_.emplace_back(buffer, recv_sz);
      }
    }
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when receiving or sending messages: " 
               << e.what();  
  }
}

void TCPReceiveSocket::stop() {
  running_.store(false);
}

void TCPReceiveSocket::comsume(
    function<void(std::deque<std::string>&)> fun) {
  std::lock_guard<std::mutex> guard(mutex_);
  fun(buffer_);
}

bool TCPReceiveSocket::running() {
  return running_.load();
}

void TCPReceiveSocket::join() {
  thread_.join();
}

void TCPReceiveSocket::detach() {
  thread_.detach();
}

}
}