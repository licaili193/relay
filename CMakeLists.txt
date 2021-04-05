cmake_minimum_required (VERSION 3.10.2)
project (Relay)

set(CMAKE_CXX_STANDARD 14)
message("CXX Standard: ${CMAKE_CXX_STANDARD}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_library(AVCODEC_LIBRARY avcodec)
find_library(SWSCALE_LIBRARY swscale)
find_package(OpenCV REQUIRED)
find_package(gflags REQUIRED)
find_package(Glog REQUIRED)
find_package(websocketpp REQUIRED)
find_package(CUDA REQUIRED)
find_package(OpenGL REQUIRED)
find_package(NANOGUI REQUIRED)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

set (CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
enable_language(CUDA)

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)

add_subdirectory (src)