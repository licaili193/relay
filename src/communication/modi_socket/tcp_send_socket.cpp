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
        uint8_t packet_num = (payload.size() + packet_size - 1) / packet_size;
        size_t index = 0;
        uint8_t packet_index = 1;
        while (index < payload.size()) {
          size_t send_size = std::min(payload.size() - index, packet_size);
          PacketHeader header = 
              {frame_index_, packet_num, packet_index, 0, send_size, 0};
          header.makePacketHeader(send_buffer_);
          memcpy(send_buffer_ + header_size, 
                 payload.c_str() + index, 
                 send_size);
          socket->send(send_buffer_, header_size + packet_size);
          index += send_size;
          packet_index++;
        }

        frame_index_++;

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