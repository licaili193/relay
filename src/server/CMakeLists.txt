project(server)

add_executable (server 
    hello_world.cpp
)

target_link_libraries (server PUBLIC 
    practical_socket
)