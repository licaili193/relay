#include <memory>
#include <string.h>

#include "tcp_send_socket.h"

namespace relay {
namespace communication {

TCPSendSocket::TCPSendSocket(TCPSocket* sock, size_t max_buffer_size) {
  max_buffer_size_ = max_buffer_size;

  running_.store(true);
  thread_ = std::thread(&TCPSendSocket::worker, this, sock);
}

void TCPSendSocket::worker(TCPSocket* sock) {
  std::unique_ptr<TCPSocket>socket(sock);
  socket->setBlocking(false);
  try {
    while (running_.load()) {
      socket->check();

      std::lock_guard<std::mutex> guard(mutex_);
      while (!buffer_.empty()) {
        const auto& payload = buffer_.front();
        size_t index = 0;
        while (index < payload.size()) {
          size_t send_size = std::min(payload.size() - index, buffer_size);
          socket->send(payload.c_str() + index, send_size);
          index += send_size;
        }
        buffer_.pop_front();
      }
    }
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when sending messages: " << e.what();  
  }
}

void TCPSendSocket::stop() {
  running_.store(false);
}

void TCPSendSocket::push(size_t payload_size, const char* payload) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (buffer_.size() >= max_buffer_size_) {
    buffer_.pop_front();
  }
  buffer_.emplace_back(payload, payload_size);
}

void TCPSendSocket::push(std::string payload) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (buffer_.size() >= max_buffer_size_) {
    buffer_.pop_front();
  }
  buffer_.push_back(std::move(payload));
}

bool TCPSendSocket::running() {
  return running_.load();
}

void TCPSendSocket::join() {
  thread_.join();
}

void TCPSendSocket::detach() {
  thread_.detach();
}

}
}