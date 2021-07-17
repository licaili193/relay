#include <cmath>
#include <algorithm>

#include "nv_hevc_sync_encoder.h"

namespace relay {
namespace codec {

NvHEVCSyncEncoder::NvHEVCSyncEncoder(CUcontext cu_context, 
                                     uint16_t width, 
                                     uint16_t height, 
                                     float framerate, 
                                     uint32_t bitrate) {
  cu_context_ = cu_context;

  width_ = width;
  height_ = height;
  framerate_ = framerate;
  bitrate_ = bitrate;

  pEnc = std::unique_ptr<NvEncoderCuda>(
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
      NV_ENC_PRESET_LOW_LATENCY_HQ_GUID);
    
  encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
  encodeConfig.frameIntervalP = 1;
  encodeConfig.encodeCodecConfig.hevcConfig.idrPeriod = 
      NVENC_INFINITE_GOPLENGTH;
  initializeParams.enablePTD = 1;

  encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
  encodeConfig.rcParams.averageBitRate = bitrate_;
  encodeConfig.rcParams.vbvBufferSize = 0;
  encodeConfig.rcParams.maxBitRate = encodeConfig.rcParams.averageBitRate;
  encodeConfig.rcParams.vbvInitialDelay = 0;

  pEnc->CreateEncoder(&initializeParams);
  
  nFrameSize = pEnc->GetFrameSize();
}

void NvHEVCSyncEncoder::push(size_t payload_size, const char* payload) {
    std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nFrameSize]);

    if (payload_size) {
        memcpy(reinterpret_cast<char*>(pHostFrame.get()), 
                payload, 
                std::min(static_cast<int>(payload_size), nFrameSize));

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

        NV_ENC_PIC_PARAMS picParams = {NV_ENC_PIC_PARAMS_VER};
        if (index_ % idr_interval_ == 0) {
            picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEINTRA | 
                                        NV_ENC_PIC_FLAG_OUTPUT_SPSPPS |
                                        NV_ENC_PIC_FLAG_FORCEIDR;
            vPacket.clear();
            pEnc->EncodeFrame(vPacket, &picParams);
            LOG(INFO) << "Sent one intra frame";
        } else {
            pEnc->EncodeFrame(vPacket);
        }
        // Reverse the packets
        std::reverse(vPacket.begin(), vPacket.end());

        index_++;
    }
}

bool NvHEVCSyncEncoder::get(size_t& result_size, char* result) {
    if (!vPacket.empty()) {
        auto& packet = vPacket.back();
        result_size = packet.size();
        memcpy(result, packet.data(), result_size);
        vPacket.pop_back();
        return true;
    }
    return false;
}

NvHEVCSyncEncoder::~NvHEVCSyncEncoder() {
  pEnc->DestroyEncoder();
}

}
}
