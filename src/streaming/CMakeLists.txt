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