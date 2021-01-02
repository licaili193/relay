project(streaming)

add_subdirectory(codec)

add_executable (codec_test 
    codec_test.cpp
)

target_link_libraries (codec_test PUBLIC 
    codecs
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)