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