project(practical_socket)

add_library (${PROJECT_NAME} SHARED 
    src/practical_socket.cpp
)

target_include_directories (${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)