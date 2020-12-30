project(communication)

add_subdirectory (bidi_socket)

include (FindGlog)
include (FindGFlags)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${GLOG_INCLUDE_DIRS})
include_directories(${GFLAGS_INCLUDE_DIRS})

add_executable (server 
    hello_world.cpp
)

target_link_libraries (server PUBLIC 
    practical_socket
    bidi_socket
    ${AVCODEC_LIBRARY}
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)