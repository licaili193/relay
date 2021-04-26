#include "async_payload_framework.h"

namespace relay {
namespace codec {

void AsyncPayloadFramework::stop() {
  running_.store(false);
}

void AsyncPayloadFramework::push(size_t payload_size, const char* payload) {
  std::lock_guard<std::mutex> guard(recv_mutex_);
  if (recv_buffer_.size() >= max_buffer_size_) {
    recv_buffer_.pop_front();
  }
  recv_buffer_.emplace_back(payload, payload_size);
}

void AsyncPayloadFramework::push(std::string payload) {
  std::lock_guard<std::mutex> guard(recv_mutex_);
  if (recv_buffer_.size() >= max_buffer_size_) {
    recv_buffer_.pop_front();
  }
  recv_buffer_.push_back(std::move(payload));
}


void AsyncPayloadFramework::consume(
    std::function<void(std::deque<std::string>&)> fun) {
  std::lock_guard<std::mutex> guard(send_mutex_);
  fun(send_buffer_);
}

bool AsyncPayloadFramework::running() {
  return running_.load();
}

void AsyncPayloadFramework::join() {
  thread_.join();
}

void AsyncPayloadFramework::detach() {
  thread_.detach();
}

void AsyncPayloadFramework::worker() {}

}
}