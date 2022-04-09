add_executable (camera_test 
    camera_test.cpp
)

target_link_libraries (camera_test PUBLIC 
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

cuda_add_library (camera_grabber SHARED 
  camera_grabber.cu
)

target_include_directories (camera_grabber PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries (camera_grabber ${CUDA_LINK_LIBRARIES_KEYWORD} 
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${CUDA_LIBRARY}
    cuda
    image_processing
)


