add_subdirectory (nvencoder)
add_subdirectory (nvdecoder)

add_library (codecs SHARED 
  async_payload_framework.cpp
  h264_decoder.cpp
  h264_encoder.cpp
  jsmpeg_encoder.cpp
)

target_include_directories (codecs PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries (codecs PUBLIC 
    ${GLOG_LIBRARIES}
    ${AVCODEC_LIBRARY}
    ${SWSCALE_LIBRARY}
)

cuda_add_library (nv_codecs SHARED 
  async_payload_framework.cpp
  nv_hevc_encoder.cpp
  nv_hevc_decoder.cpp
)

target_include_directories (nv_codecs PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries (nv_codecs ${CUDA_LINK_LIBRARIES_KEYWORD} 
    ${GLOG_LIBRARIES}
    nvencoder
    nvdecoder
)