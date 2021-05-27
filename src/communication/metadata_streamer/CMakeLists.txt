add_library (shm_socket INTERFACE)

target_link_libraries (shm_socket INTERFACE 
  spmc_queue
  ${GLOG_LIBRARIES}
)

target_include_directories (shm_socket INTERFACE 
  ${CMAKE_CURRENT_SOURCE_DIR}
)
