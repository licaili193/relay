project(spmc_queue)

add_library (${PROJECT_NAME} INTERFACE)

target_link_libraries (${PROJECT_NAME} INTERFACE 
  rt
)

target_include_directories (${PROJECT_NAME} INTERFACE 
  ${CMAKE_CURRENT_SOURCE_DIR}
)