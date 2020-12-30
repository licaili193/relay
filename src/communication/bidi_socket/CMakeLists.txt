add_library (bidi_socket SHARED 
  bidi_tcp_socket.cpp
)

target_include_directories (bidi_socket PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries (bidi_socket PUBLIC 
    practical_socket
    ${GLOG_LIBRARIES}
)