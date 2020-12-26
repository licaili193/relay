project(server)

find_library(AVCODEC_LIBRARY avcodec)

add_executable (server 
    hello_world.cpp
)

target_link_libraries (server PUBLIC 
    practical_socket
    ${AVCODEC_LIBRARY}
)