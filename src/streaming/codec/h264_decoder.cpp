#include "h264_decoder.h"

namespace relay {
namespace codec {

H264Decoder::H264Decoder() {
  running_.store(true);
  thread_ = std::thread(&H264Decoder::worker, this);
}

void H264Decoder::push(size_t payload_size, const char* payload) {
  std::lock_guard<std::mutex> guard(send_mutex_);
  if (recv_buffer_.size() >= max_buffer_size_) {
    recv_buffer_.pop_front();
  }
  recv_buffer_.emplace_back(
      payload, payload_size + AV_INPUT_BUFFER_PADDING_SIZE);
  memset(const_cast<char*>(recv_buffer_.back().c_str()) + payload_size, 
         0, 
         AV_INPUT_BUFFER_PADDING_SIZE);
}

void H264Decoder::push(std::string payload) {
  auto original_size = payload.size();
  payload.resize(payload.size() + AV_INPUT_BUFFER_PADDING_SIZE);
  memset(const_cast<char*>(payload.c_str()) + original_size, 
         0, 
         AV_INPUT_BUFFER_PADDING_SIZE);
  std::lock_guard<std::mutex> guard(send_mutex_);
  if (recv_buffer_.size() >= max_buffer_size_) {
    recv_buffer_.pop_front();
  }
  recv_buffer_.push_back(std::move(payload));
}

void H264Decoder::worker() {
  AVFrame* frame;
  
  avcodec_register_all();

  AVCodec* codec = avcodec_find_decoder(codec_id_);
  if (!codec) {
    LOG(FATAL) << "Failed to find decoder";
  }
  AVCodecContext* c = avcodec_alloc_context3(codec);
  if (!c) {
    LOG(FATAL) << "Failed to allocate decoder codec context";
  }

  static SwsContext *img_c = NULL; 
  
  AVPacket* pkt = av_packet_alloc();
  if (!pkt) {
    LOG(FATAL) << "Failed to allocate packet for decoder";
  }

  if (avcodec_open2(c, codec, NULL) < 0) {
    LOG(FATAL) << "Failed to open decoder";
  }

  frame = av_frame_alloc();
  if (!frame) {
    LOG(FATAL) << "Failed to allocate image frame for decoder";
  }

  while (running_.load()) {
    std::string payload;
    bool has_payload = false;
    {
      std::lock_guard<std::mutex> guard(recv_mutex_);
      if (!recv_buffer_.empty()) {
        payload = std::move(recv_buffer_.front());
        recv_buffer_.pop_front();
        has_payload = true;
      }
    }
    if (has_payload) {
      pkt->size = payload.size() - AV_INPUT_BUFFER_PADDING_SIZE;
      pkt->data = const_cast<uint8_t*>(
          reinterpret_cast<const uint8_t*>(payload.c_str()));
      
      int ret = avcodec_send_packet(c, pkt);
      if (ret < 0) {
        LOG(ERROR) << "Error occurred when sending frame " << index_;
      }
      
      while (ret >= 0) {
        ret = avcodec_receive_frame(c, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        } else if (ret < 0) {
          LOG(ERROR) << "Error occurred when decoding frame " << index_;
          break;
        }
        std::string temp(c->width * c->height * 3 /2, 0);
        memcpy(const_cast<char*>(temp.c_str()), 
               frame->data[0], 
               c->width * c->height);
        memcpy(const_cast<char*>(temp.c_str()) + c->width * c->height, 
               frame->data[1], 
               c->width * c->height / 4);
        memcpy(const_cast<char*>(temp.c_str()) + c->width * c->height * 5 / 4, 
               frame->data[2], 
               c->width * c->height / 4);
        std::lock_guard<std::mutex> guard(send_mutex_);
        if (send_buffer_.size() >= max_buffer_size_) {
          send_buffer_.pop_front();
        }
        send_buffer_.push_back(std::move(temp));
      }
    }
  }

  avcodec_free_context(&c);
  av_frame_free(&frame);
  av_packet_free(&pkt);
}

}
}
