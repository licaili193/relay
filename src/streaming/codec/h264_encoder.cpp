#include "h264_encoder.h"

namespace relay {
namespace codec {

H264Encoder::H264Encoder(
    uint16_t width, uint16_t height, float framerate, uint32_t bitrate) {
  width_ = width;
  height_ = height;
  framerate_ = framerate;
  bitrate_ = bitrate;

  running_.store(true);
  thread_ = std::thread(&H264Encoder::worker, this);
}

void H264Encoder::worker() {
  int ret;
  AVFrame *frame;

  AVCodec* codec = avcodec_find_encoder(codec_id_);
  if (!codec) {
    LOG(FATAL) << "Failed to find encoder";
  }
  AVCodecContext* c = avcodec_alloc_context3(codec);
  if (!c) {
    LOG(FATAL) << "Failed to allocate encoder codec context";
  }
  c->bit_rate = bitrate_;
  c->width = width_;
  c->height = height_;
  c->time_base = (AVRational){1, static_cast<int>(framerate_)};
  c->gop_size = gop_size_;
  c->max_b_frames = max_b_frames_;
  c->pix_fmt = input_format_;
  av_opt_set(c->priv_data, "tune", "zerolatency", 0);
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
  ret = av_image_alloc(
      frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
  if (ret < 0) {
    LOG(FATAL) << "Failed to allocate raw picture buffer for encoder";
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
      int got_output;
      AVPacket pkt;
      pkt.data = NULL;
      pkt.size = 0;
      memcpy(frame->data[0], payload.c_str(), width_ * height_);
      memcpy(frame->data[1], 
             payload.c_str() + width_ * height_, 
             width_ * height_ / 4);
      memcpy(frame->data[2], 
             payload.c_str() + width_ * height_ * 5 / 4, 
             width_ * height_ / 4);      
      frame->pts = index_;
      ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
      if (ret < 0) {
        LOG(ERROR) << "Error occurred when encoding frame " << index_;
      }
      if (got_output) {
        std::lock_guard<std::mutex> guard(send_mutex_);
        if (send_buffer_.size() >= max_buffer_size_) {
          send_buffer_.pop_front();
        }
        send_buffer_.emplace_back(reinterpret_cast<char*>(pkt.data), pkt.size);
        av_free_packet(&pkt);
        index_++;
      } else {
        LOG(WARNING) << "Skipped one payload when encoding frame " << index_;
      }
      do {
        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
          LOG(ERROR) << "Error occurred when encoding frame " << index_;
        }
        if (got_output) {
          std::lock_guard<std::mutex> guard(send_mutex_);
          if (send_buffer_.size() >= max_buffer_size_) {
            send_buffer_.pop_front();
          }
          send_buffer_.emplace_back(reinterpret_cast<char*>(pkt.data), pkt.size);
          av_free_packet(&pkt);
          index_++;
        }
      } while (got_output);
    }
  }
  avcodec_close(c);
  av_free(c);
  av_freep(&frame->data[0]);
  av_frame_free(&frame);
}

}
}
