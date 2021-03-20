#include <cmath>

#include "nv_hevc_encoder.h"

namespace relay {
namespace codec {

NvHEVCEncoder::NvHEVCEncoder(CUcontext cu_context, 
                             uint16_t width, 
                             uint16_t height, 
                             float framerate, 
                             uint32_t bitrate) {
  cu_context_ = cu_context;

  width_ = width;
  height_ = height;
  framerate_ = framerate;
  bitrate_ = bitrate;

  running_.store(true);
  thread_ = std::thread(&NvHEVCEncoder::worker, this);
}

void NvHEVCEncoder::worker() {
  std::unique_ptr<NvEncoderCuda> pEnc(
      new NvEncoderCuda(cu_context_, width_, height_, buffer_format_));

  NV_ENC_INITIALIZE_PARAMS initializeParams = 
      { NV_ENC_INITIALIZE_PARAMS_VER };
  NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

  initializeParams.frameRateDen = 1;
  initializeParams.frameRateNum = static_cast<size_t>(framerate_);

  initializeParams.encodeConfig = &encodeConfig;
  pEnc->CreateDefaultEncoderParams(
      &initializeParams, 
      NV_ENC_CODEC_HEVC_GUID, 
      NV_ENC_PRESET_P1_GUID, 
      NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY );
    
  encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
  encodeConfig.frameIntervalP = 1;
  encodeConfig.encodeCodecConfig.hevcConfig.idrPeriod = 
      NVENC_INFINITE_GOPLENGTH;

  encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
  encodeConfig.rcParams.averageBitRate = bitrate_;
  encodeConfig.rcParams.vbvBufferSize = 
      (encodeConfig.rcParams.averageBitRate * 
       initializeParams.frameRateDen / 
       initializeParams.frameRateNum) * 5;
  encodeConfig.rcParams.maxBitRate = encodeConfig.rcParams.averageBitRate;
  encodeConfig.rcParams.vbvInitialDelay = encodeConfig.rcParams.vbvBufferSize;

  pEnc->CreateEncoder(&initializeParams);
  
  int nFrameSize = pEnc->GetFrameSize();
    
  std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nFrameSize]);
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
      memcpy(reinterpret_cast<char*>(pHostFrame.get()), 
             payload.c_str(), 
             std::min(static_cast<int>(payload.size()), nFrameSize));
      std::vector<std::vector<uint8_t>> vPacket;

      const NvEncInputFrame* encoderInputFrame = pEnc->GetNextInputFrame();
      NvEncoderCuda::CopyToDeviceFrame(
          cu_context_, 
          pHostFrame.get(), 
          0, 
          (CUdeviceptr)encoderInputFrame->inputPtr,
          (int)encoderInputFrame->pitch,
          pEnc->GetEncodeWidth(),
          pEnc->GetEncodeHeight(),
          CU_MEMORYTYPE_HOST, 
          encoderInputFrame->bufferFormat,
          encoderInputFrame->chromaOffsets,
          encoderInputFrame->numChromaPlanes);

      pEnc->EncodeFrame(vPacket);

      index_ += (int)vPacket.size();
    
      for (std::vector<uint8_t> &packet : vPacket) {
        std::lock_guard<std::mutex> guard(send_mutex_);
        if (send_buffer_.size() >= max_buffer_size_) {
          send_buffer_.pop_front();
        }
        send_buffer_.emplace_back(
            reinterpret_cast<char*>(packet.data()), packet.size());
      }
    }
  }

  pEnc->DestroyEncoder();
}

}
}
