project(server)

find_library(AVCODEC_LIBRARY avcodec)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable (server 
    hello_world.cpp
)

target_link_libraries (server PUBLIC 
    practical_socket
    ${AVCODEC_LIBRARY}
    ${OpenCV_LIBS}
)