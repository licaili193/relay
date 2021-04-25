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

add_executable (sender 
    sender.cpp
)

target_link_libraries (sender PUBLIC 
    codecs
    practical_socket
    bidi_socket
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

add_executable (sample_sender 
    sample_sender.cpp
)

target_link_libraries (sample_sender PUBLIC 
    codecs
    practical_socket
    bidi_socket
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

add_executable (receiver 
    receiver.cpp
)

target_link_libraries (receiver PUBLIC 
    codecs
    practical_socket
    bidi_socket
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

add_executable (jsmpeg_sender 
    jsmpeg_sender.cpp
)

target_link_libraries (jsmpeg_sender PUBLIC 
    codecs
    practical_socket
    bidi_socket
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

add_executable (websocket_relay 
    websocket_relay.cpp
)

target_link_libraries (websocket_relay PUBLIC 
    codecs
    practical_socket
    bidi_socket
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${websocketpp_LIBRARIES}
)

cuda_add_executable (nv_hevc_sender 
    nv_hevc_sender.cpp
)

target_link_libraries (nv_hevc_sender ${CUDA_LINK_LIBRARIES_KEYWORD} 
    cuda
    nv_codecs
    practical_socket
    bidi_socket
    ${CUDA_LIBRARY}
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

cuda_add_executable (sample_nv_hevc_sender 
    sample_nv_hevc_sender.cpp
)

target_link_libraries (sample_nv_hevc_sender ${CUDA_LINK_LIBRARIES_KEYWORD} 
    cuda
    nv_codecs
    practical_socket
    modi_socket
    ${CUDA_LIBRARY}
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

cuda_add_executable (nv_hevc_receiver 
    nv_hevc_receiver.cpp
)

target_link_libraries (nv_hevc_receiver ${CUDA_LINK_LIBRARIES_KEYWORD} 
    cuda
    nv_codecs
    practical_socket
    bidi_socket
    ${CUDA_LIBRARY}
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)