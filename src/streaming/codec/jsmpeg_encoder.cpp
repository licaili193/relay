#include "jsmpeg_encoder.h"

namespace relay {
namespace codec {

JsmpegEncoder::JsmpegEncoder(
    uint16_t width, uint16_t height, float framerate, uint32_t bitrate) {
  width_ = width;
  height_ = height;
  framerate_ = framerate;
  bitrate_ = bitrate;

  running_.store(true);
  thread_ = std::thread(&JsmpegEncoder::worker, this);
}

void JsmpegEncoder::worker() {
  int ret;
  AVFrame* frame;
  AVPacket* pkt;

  avcodec_register_all();

  AVCodec* codec = avcodec_find_encoder(codec_id_);
  if (!codec) {
    LOG(FATAL) << "Failed to find encoder";
  }
  AVCodecContext* c = avcodec_alloc_context3(codec);
  if (!c) {
    LOG(FATAL) << "Failed to allocate encoder codec context";
  }
  c->dct_algo = FF_DCT_FASTINT;
  c->bit_rate = bitrate_;
  c->width = width_;
  c->height = height_;
  c->time_base = (AVRational){1, static_cast<int>(framerate_)};
  c->framerate = (AVRational){static_cast<int>(framerate_), 1};
  c->gop_size = gop_size_;
  c->max_b_frames = max_b_frames_;
  c->pix_fmt = input_format_;
  if (avcodec_open2(c, codec, NULL) < 0) {
    LOG(FATAL) << "Failed to open encoder";
  }
  frame = av_frame_alloc();
  if (!frame) {
    LOG(FATAL) << "Failed to allocate image frame for encoder";
  }
  frame->format = c->pix_fmt;
  frame->width  = c->width;
  frame->height = c->height;
  frame->pts = 0;
  ret = av_frame_get_buffer(frame, 0);
  if (ret < 0) {
    LOG(FATAL) << "Failed to allocate raw picture buffer for encoder";
  }

  pkt = av_packet_alloc();
  if (!pkt) {
    LOG(FATAL) << "Failed to allocate packet for encoder";
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
      ret = av_frame_make_writable(frame);
      if (ret < 0) {
        LOG(FATAL) << "Encoder frame is not writable";
      }
      memcpy(frame->data[0], payload.c_str(), width_ * height_);
      memcpy(frame->data[1], 
             payload.c_str() + width_ * height_, 
             width_ * height_ / 4);
      memcpy(frame->data[2], 
             payload.c_str() + width_ * height_ * 5 / 4, 
             width_ * height_ / 4);      
      frame->pts++;
      ret = avcodec_send_frame(c, frame);
      if (ret < 0) {
        LOG(ERROR) << "Error occurred when sending frame " << index_;
      }
      while (ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        } else if (ret < 0) {
          LOG(ERROR) << "Error occurred when encoding frame " << index_;
        }
        std::lock_guard<std::mutex> guard(send_mutex_);
        if (send_buffer_.size() >= max_buffer_size_) {
          send_buffer_.pop_front();
        }
        send_buffer_.emplace_back(
            reinterpret_cast<char*>(pkt->data), pkt->size);
        av_packet_unref(pkt);
        index_++;
      }
    }
  }
  avcodec_free_context(&c);
  av_frame_free(&frame);
  av_packet_free(&pkt);
}

}
}
