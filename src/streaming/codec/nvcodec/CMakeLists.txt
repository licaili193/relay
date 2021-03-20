cuda_add_library (nvencoder SHARED 
  NvEncoder.cpp
  NvEncoderCuda.cpp
)

target_include_directories (nvencoder PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries (nvencoder ${CUDA_LINK_LIBRARIES_KEYWORD} 
  ${CUDA_LIBRARY}    
  nvcodec
  ${GLOG_LIBRARIES}
)

cuda_add_library (nvdecoder SHARED 
  NvDecoder.cpp
)

target_include_directories (nvdecoder PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries (nvdecoder ${CUDA_LINK_LIBRARIES_KEYWORD} 
  ${CUDA_LIBRARY}    
  nvcodec
  ${GLOG_LIBRARIES}
)