project(communication)

add_subdirectory (bidi_socket)
add_subdirectory (modi_socket)

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

add_executable (shm_test 
    shm_test.cpp
)

target_link_libraries (shm_test PUBLIC 
    spmc_queue
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

target_include_directories(shm_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/communication/messages
)
