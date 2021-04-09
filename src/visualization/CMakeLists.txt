project(visualization)

add_executable (nanovg_example 
    nanovg_example.cpp
)

target_link_libraries (nanovg_example PUBLIC 
    Eigen3::Eigen
    nanovg
    ${NANOGUI_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${OpenCV_LIBS}
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
)