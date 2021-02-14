project(communication)

add_subdirectory (bidi_socket)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${GLOG_INCLUDE_DIRS})
include_directories(${GFLAGS_INCLUDE_DIRS})

add_executable (bidi_tcp_server_test 
    bidi_tcp_server_test.cpp
)

target_link_libraries (bidi_tcp_server_test PUBLIC 
    practical_socket
    bidi_socket
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

add_executable (bidi_tcp_client_test 
    bidi_tcp_client_test.cpp
)

target_link_libraries (bidi_tcp_client_test PUBLIC 
    practical_socket
    bidi_socket
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

add_executable (websocket_relay 
    websocket_relay.cpp
)

target_link_libraries (websocket_relay PUBLIC 
    ${websocketpp_LIBRARIES}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)