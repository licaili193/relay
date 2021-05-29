project(visualization)

cuda_add_executable (hevc_visualizer 
    hevc_visualizer.cpp
)

target_link_libraries (hevc_visualizer ${CUDA_LINK_LIBRARIES_KEYWORD} 
    Eigen3::Eigen
    nanovg   
    cuda
    nv_codecs
    practical_socket
    modi_socket
    shm_socket
    ${NANOGUI_LIBRARY} 
    ${OPENGL_LIBRARIES}
    ${CUDA_LIBRARY}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

target_include_directories(hevc_visualizer PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/communication/messages
)

cuda_add_executable (hevc_udp_visualizer 
    hevc_udp_visualizer.cpp
)

target_link_libraries (hevc_udp_visualizer ${CUDA_LINK_LIBRARIES_KEYWORD} 
    Eigen3::Eigen
    nanovg   
    cuda
    nv_codecs
    practical_socket
    modi_socket
    shm_socket
    ${NANOGUI_LIBRARY} 
    ${OPENGL_LIBRARIES}
    ${CUDA_LIBRARY}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)

target_include_directories(hevc_udp_visualizer PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/communication/messages
)
