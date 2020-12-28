#include <iostream>

#include <opencv2/opencv.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "practical_socket.h"

int main() {
    std::cout << "AVCODEC Version: " << avcodec_version() << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    return 0;
}
