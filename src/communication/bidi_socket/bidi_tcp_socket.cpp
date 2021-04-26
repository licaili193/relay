#include <memory>
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
    TCPSocket* sock, size_t max_buffer_size, double ping_period_sec) 
    : ping_period_(ping_period_sec) {
  max_buffer_size_ = max_buffer_size;
  ping_time_ = std::chrono::system_clock::now();

  running_.store(true);
  thread_ = std::thread(&BiDirectionalTCPSocket::worker, this, sock);
}

bool BiDirectionalTCPSocket::isCorrectHeader(
    char* buffer, int size, FrameHeader& header) {
  if (size == FrameHeader::header_size) {
    header = FrameHeader::parseFrameHeader(buffer);
    if (header.length <= buffer_size) {
      if ((header.command == COMMAND_DISCONNECT) && 
          (header.id == FIN_INDEX_KEY) &&
          (header.length == FIN_SIZE_KEY)) {
        running_.store(false);
        return false;
      }
      if ((header.command == COMMAND_PING) && 
          (header.id == PING_INDEX_KEY) &&
          (header.length == PING_SIZE_KEY)) {
        return false;
      }
      return true;
    }
    LOG(ERROR) << "Bad header detected (buffer size check failed)...";
  }
  if (size > 0) {
    LOG(ERROR) << "Bad header detected (header size check failed)...";
  }
  return false;
}

void BiDirectionalTCPSocket::worker(TCPSocket* sock) {
  std::unique_ptr<TCPSocket>socket(sock);
  socket->setBlocking(false);
  try {
    while (running_.load()) {
      socket->check();

      // Receiving
      char buffer[buffer_size];
      int recv_sz = 0;
      FrameHeader header;

      recv_sz = socket->recv(buffer, FrameHeader::header_size);
      if (isCorrectHeader(buffer, recv_sz, header)) {
        int recv_sz_accum = 0;
        
        while (recv_sz_accum < header.length) {
          recv_sz = socket->recv(buffer + recv_sz_accum, 
              header.length - recv_sz_accum);
          if (recv_sz > 0) {
            recv_sz_accum += recv_sz;
          }
        }

        std::lock_guard<std::mutex> guard(recv_mutex_);
        if (recv_buffer_.size() >= max_buffer_size_) {
          recv_buffer_.pop_front();
        }
        recv_buffer_.emplace_back(buffer, recv_sz_accum);
      }

      {
        std::lock_guard<std::mutex> guard(send_mutex_);
        while (!send_buffer_.empty()) {
          const auto& payload = send_buffer_.front();
          FrameHeader header = {index_, payload.size(), 0};
          char header_buffer[FrameHeader::header_size];
          header.makeFrameHeader(header_buffer);
          socket->send(header_buffer, FrameHeader::header_size);
          socket->send(payload.c_str(), payload.size());

          ping_time_ = std::chrono::system_clock::now();
          send_buffer_.pop_front();
          index_++;
        }
      }

      auto cur_time = std::chrono::system_clock::now();
      if (cur_time - ping_time_ >= ping_period_) {
        FrameHeader header = 
            {PING_INDEX_KEY, PING_SIZE_KEY, COMMAND_PING};
        char header_buffer[FrameHeader::header_size];
        header.makeFrameHeader(header_buffer);
        socket->send(header_buffer, FrameHeader::header_size);
        ping_time_ = std::move(cur_time);
      }
    }
    LOG(WARNING) << "Socket sending fin";
    FrameHeader header = {FIN_INDEX_KEY, FIN_SIZE_KEY, COMMAND_DISCONNECT};
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

void BiDirectionalTCPSocket::push(std::string payload) {
  std::lock_guard<std::mutex> guard(send_mutex_);
  if (send_buffer_.size() >= max_buffer_size_) {
    send_buffer_.pop_front();
  }
  send_buffer_.push_back(std::move(payload));
}

void BiDirectionalTCPSocket::consume(
    function<void(std::deque<std::string>&)> fun) {
  std::lock_guard<std::mutex> guard(recv_mutex_);
  fun(recv_buffer_);
}

bool BiDirectionalTCPSocket::running() {
  return running_.load();
}

void BiDirectionalTCPSocket::join() {
  thread_.join();
}

void BiDirectionalTCPSocket::detach() {
  thread_.detach();
}

}
}