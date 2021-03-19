project(nvcodec)

add_library (${PROJECT_NAME} INTERFACE)

target_include_directories (${PROJECT_NAME} INTERFACE 
  ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

file (GLOB lib 
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.so"
)

target_link_libraries (${PROJECT_NAME} INTERFACE 
    ${lib}
)
