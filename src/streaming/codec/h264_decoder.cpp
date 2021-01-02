#include "h264_decoder.h"

namespace relay {
namespace codec {

H264Decoder::H264Decoder() {

}

void H264Decoder::push(size_t payload_size, const char* payload) {
  std::lock_guard<std::mutex> guard(send_mutex_);
  if (send_buffer_.size() >= max_buffer_size_) {
    send_buffer_.pop_front();
  }
  send_buffer_.emplace_back(
      payload, payload_size + AV_INPUT_BUFFER_PADDING_SIZE);
  memset(const_cast<char*>(send_buffer_.back().c_str()) + payload_size, 
         0, 
         AV_INPUT_BUFFER_PADDING_SIZE);
}

void H264Decoder::worker() {
  AVFrame* frame;
  AVFrame* scaled_frame;
  
  AVCodec* codec = avcodec_find_decoder(codec_id_);
  if (!codec) {
    LOG(FATAL) << "Failed to find decoder";
  }
  AVCodecContext* c = avcodec_alloc_context3(codec);
  if (!c) {
    LOG(FATAL) << "Failed to allocate decoder codec context";
  }

  static SwsContext *img_c = NULL; 
  
  AVPacket pkt;
  av_init_packet(&pkt);

  if (avcodec_open2(c, codec, NULL) < 0) {
    LOG(FATAL) << "Failed to open decoder";
  }

  frame = av_frame_alloc();
  if (!frame) {
    LOG(FATAL) << "Failed to allocate image frame for decoder";
  }

  scaled_frame = av_frame_alloc();
  if (!scaled_frame) {
    LOG(FATAL) << "Failed to allocate scaled frame for decoder";
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
      pkt.size = payload.size() - AV_INPUT_BUFFER_PADDING_SIZE;
      pkt.data = const_cast<uint8_t*>(
          reinterpret_cast<const uint8_t*>(payload.c_str()));
      
      int len, got_frame;
      len = avcodec_decode_video2(c, frame, &got_frame, &pkt);
      if (len < 0) {
        LOG(ERROR) << "Error occurred when decoding frame " << index_;
      }
      if (got_frame) {
        if (c->pix_fmt != output_format_) {
          if(img_c == NULL) {
            img_c = sws_getContext(c->width, 
                                   c->height, 
                                   c->pix_fmt, 
                                   c->width, 
                                   c->height, 
                                   output_format_, 
                                   SWS_BILINEAR, 
                                   NULL, 
                                   NULL, 
                                   NULL);
            if(!img_c) {
              LOG(FATAL) << "Failed to allocate decoder image convert context";
            }
          }         
          sws_scale(img_c, 
                    frame->data, 
                    frame->linesize, 
                    0, 
                    c->height, 
                    scaled_frame->data, 
                    scaled_frame->linesize);     
          std::lock_guard<std::mutex> guard(send_mutex_);
          if (send_buffer_.size() >= max_buffer_size_) {
            send_buffer_.pop_front();
          }
          send_buffer_.emplace_front(
              reinterpret_cast<char*>(scaled_frame->data[0]), 
              scaled_frame->linesize[0] * c->height);
        } else {
          std::lock_guard<std::mutex> guard(send_mutex_);
          if (send_buffer_.size() >= max_buffer_size_) {
            send_buffer_.pop_front();
          }
          send_buffer_.emplace_front(reinterpret_cast<char*>(frame->data[0]), 
                                    frame->linesize[0] * c->height);
        }
      }
    }
  }
}

}
}
