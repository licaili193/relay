#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "practical_socket.h"

int main() {
    std::cout << "AVCODEC Version: " << avcodec_version() << std::endl;
    return 0;
}
