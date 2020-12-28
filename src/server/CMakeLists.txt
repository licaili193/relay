project(server)

# include (FindGlog)
include (FindGFlags)

find_library(AVCODEC_LIBRARY avcodec)
find_package(OpenCV REQUIRED)
find_package(gflags REQUIRED)
find_package(Glog REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${GLOG_INCLUDE_DIRS})
include_directories(${GFLAGS_INCLUDE_DIRS})

add_executable (server 
    hello_world.cpp
)

target_link_libraries (server PUBLIC 
    practical_socket
    ${AVCODEC_LIBRARY}
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)