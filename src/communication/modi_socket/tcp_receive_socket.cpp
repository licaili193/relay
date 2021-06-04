#include <string.h>
#include <memory>

#include "tcp_receive_socket.h"

namespace relay {
namespace communication {

TCPReceiveSocket::TCPReceiveSocket(TCPSocket* sock, size_t max_buffer_size) {
  max_buffer_size_ = max_buffer_size;

  received_time_ = std::chrono::system_clock::now();

  running_.store(true);
  thread_ = std::thread(&TCPReceiveSocket::worker, this, sock);
}

void TCPReceiveSocket::worker(TCPSocket* sock) {
  char buffer[packet_size + header_size];
  size_t received_size = 0;

  std::unique_ptr<TCPSocket> socket(sock);
  socket->setBlocking(false);
  try {
    while (running_.load()) {
      socket->check();

      int recv_sz = 0;
      int accum_recv_sz = 0;
      do {
        recv_sz = socket->recv(buffer + accum_recv_sz,
                               packet_size + header_size - accum_recv_sz);
        auto now = std::chrono::system_clock::now();
        if (recv_sz > 0) {
          received_time_ = now;
          accum_recv_sz += recv_sz;
        } else {
          if (now - received_time_ > std::chrono::milliseconds(timeout_ms)) {
            break;
          }
        }
      } while (accum_recv_sz < packet_size + header_size);

      if (accum_recv_sz != packet_size + header_size) {
        continue;
      }

      auto header = PacketHeader::parsePacketHeader(buffer);
      bool sanity_check_failed = false;
      if ((received_size + header.size > receive_buffer_size_) ||
          (header.size > packet_size)) {
        sanity_check_failed = true;
      }
      if (header.packet_num < header.packet_index || 
          header.packet_index == 0 ||
          header.packet_num == 0 ||
          header.packet_needed != 0 ||
          header.timestamp != 0) {
        sanity_check_failed = true;
      }

      if (sanity_check_failed) {
        LOG(WARNING) << "Received one frame with invalid header";
        char temp_buffer[packet_size * 3];
        socket->recv(temp_buffer, packet_size * 3);
        continue;
      }

      memcpy(receive_buffer_ + received_size, buffer + header_size,
             header.size);
      received_size += header.size;
      if (header.packet_num == header.packet_index) {
        std::lock_guard<std::mutex> guard(mutex_);
        if (buffer_.size() >= max_buffer_size_) {
          buffer_.pop_front();
        }
        buffer_.emplace_back(receive_buffer_, received_size);
        received_size = 0;
      }
    }
  } catch (SocketException& e) {
    LOG(FATAL) << "Error occurred when receiving or sending messages: "
               << e.what();
  }
}

void TCPReceiveSocket::stop() { running_.store(false); }

void TCPReceiveSocket::consume(function<void(std::deque<std::string>&)> fun) {
  std::lock_guard<std::mutex> guard(mutex_);
  fun(buffer_);
}

bool TCPReceiveSocket::running() { return running_.load(); }

void TCPReceiveSocket::join() { thread_.join(); }

void TCPReceiveSocket::detach() { thread_.detach(); }

}  // namespace communication
}  // namespace relay