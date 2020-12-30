#include <memory>
#include <thread>
#include <string.h>

#include "bidi_tcp_socket.h"

namespace relay {
namespace communication {

void FrameHeader::makeFrameHeader(char buffer[9]) {
  memcpy(buffer, &id, sizeof(uint32_t));
  memcpy(buffer + sizeof(uint32_t), &length, sizeof(uint32_t));
  memcpy(buffer + 2 * sizeof(uint32_t), &command, sizeof(uint8_t));
}

FrameHeader FrameHeader::parseFrameHeader(const char buffer[9]) {
  FrameHeader header;
  memcpy(&(header.id), buffer, sizeof(uint32_t));
  memcpy(&(header.length), buffer + sizeof(uint32_t), sizeof(uint32_t));
  memcpy(&(header.command), buffer + 2 * sizeof(uint32_t), sizeof(uint8_t));
  return header;
}

const size_t FrameHeader::header_size = 9;

BiDirectionalTCPSocket::BiDirectionalTCPSocket(
    TCPSocket* sock, size_t max_buffer_size) {
  max_buffer_size_ = max_buffer_size;

  running_.store(true);
  std::thread work_thread(&BiDirectionalTCPSocket::worker, this, sock);
  work_thread.detach();
}

void BiDirectionalTCPSocket::worker(TCPSocket* sock) {
  std::unique_ptr<TCPSocket>socket(sock);
  try {
    while (running_.load()) {
      // Receiving
      while (true) {
        char header_buffer[FrameHeader::header_size];
        size_t h_recv_sz_accum = 0;
        size_t h_recv_sz = 0;
        bool got_header = false;

        while (
            h_recv_sz_accum < FrameHeader::header_size && 
            (h_recv_sz = 
                socket->recv(header_buffer + h_recv_sz_accum, 
                    FrameHeader::header_size - h_recv_sz_accum)) > 0) {
          h_recv_sz_accum += h_recv_sz;
          got_header = true;
        }
        if (got_header) {
          FrameHeader header = FrameHeader::parseFrameHeader(header_buffer);
          if (header.command == COMMAND_DISCONNECT) {
            running_.store(false);
          }

          constexpr size_t buffer_size = 87380;
          char buffer[buffer_size];
          size_t recv_sz_accum = 0;
          size_t recv_sz = 0;

          while (
            recv_sz_accum < header.length && 
            (recv_sz = 
                socket->recv(buffer + recv_sz_accum, 
                    header.length - recv_sz_accum)) > 0) {
            recv_sz_accum += recv_sz;
          }

          std::lock_guard<std::mutex> guard(recv_mutex_);
          if (recv_buffer_.size() >= max_buffer_size_) {
            recv_buffer_.pop_front();
          }
          recv_buffer_.emplace_back(buffer, recv_sz_accum);
        } else {
          break;
        }
      }

      // Sending
      {
        std::lock_guard<std::mutex> guard(send_mutex_);
        while (!send_buffer_.empty()) {
          const auto& payload = send_buffer_.front();
          FrameHeader header = {index_, payload.size(), 0};
          char header_buffer[FrameHeader::header_size];
          header.makeFrameHeader(header_buffer);
          socket->send(header_buffer, FrameHeader::header_size);
          socket->send(payload.c_str(), payload.size());

          send_buffer_.pop_front();
          index_++;
        }
      }
    }
    FrameHeader header = {index_, 0, COMMAND_DISCONNECT};
    char header_buffer[FrameHeader::header_size];
    header.makeFrameHeader(header_buffer);
    socket->send(header_buffer, FrameHeader::header_size);
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when receiving or sending messages: " 
               << e.what();  
  }
}

void BiDirectionalTCPSocket::stop() {
  running_.store(false);
}

void BiDirectionalTCPSocket::push(size_t payload_size, const char* payload) {
  std::lock_guard<std::mutex> guard(send_mutex_);
  if (send_buffer_.size() >= max_buffer_size_) {
    send_buffer_.pop_front();
  }
  send_buffer_.emplace_back(payload, payload_size);
}

void BiDirectionalTCPSocket::comsume(
    function<void(std::deque<std::string>&)> fun) {
  std::lock_guard<std::mutex> guard(recv_mutex_);
  fun(recv_buffer_);
}

bool BiDirectionalTCPSocket::running() {
  return running_.load();
}

}
}