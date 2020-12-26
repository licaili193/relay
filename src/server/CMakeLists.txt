include_directories(${CMAKE_SOURCE_DIR})

add_executable (hellow_world 
    hello_world.cpp
    src/third_party/practical_socket/practical_socket.h
)

target_link_libraries (hellow_world PUBLIC 
    practial_socket
)