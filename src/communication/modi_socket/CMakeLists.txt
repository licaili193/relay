add_subdirectory(fec)

add_library (modi_socket SHARED 
  tcp_send_socket.cpp
  tcp_receive_socket.cpp
  udp_send_socket.cpp
  udp_receive_socket.cpp
)

target_include_directories (modi_socket PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries (modi_socket PUBLIC 
    practical_socket
    fec_codec
    fecpp
    ${GLOG_LIBRARIES}
)