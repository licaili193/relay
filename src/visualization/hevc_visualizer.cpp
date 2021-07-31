#include "nanogui_includes.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <cstdint>
#include <memory>
#include <utility>

#include <cuda.h>
#include <signal.h>
#include <unistd.h>

#include "practical_socket.h"
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "NvCodecUtils.h"
#include "tcp_receive_socket.h"

#include "visualizer.h"

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  try {
    LOG(INFO) << "Connecting to server: "
              << FLAGS_foreign_addr 
              << ":" 
              << FLAGS_foreign_port;

    relay::communication::TCPReceiveSocket modi_sock(
        new TCPSocket(FLAGS_foreign_addr, FLAGS_foreign_port));

    relay::communication::SHMSocket<ControlCommand, VehicleState> 
        shms("control_command", "vehicle_state");

    int iGpu = 0;
    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    if (iGpu < 0 || iGpu >= nGpu) {
        LOG(FATAL) << "GPU ordinal out of range. Should be within"
                    " [" << 0 << ", " << nGpu - 1 << "]";
    }

    CUcontext cuContext = NULL;
    createCudaContext(&cuContext, iGpu, 0);
    
    relay::codec::NvHEVCSyncDecoder decoder(cuContext);

    nanogui::init();

    /* scoped variables */ 
    {
      nanogui::ref<relay::visualization::Visualizer> app = 
          new relay::visualization::Visualizer(640 * 3, 480);
      app->setDecoder(&decoder);
      app->setVideoSocket(
          dynamic_cast<relay::communication::ReceiveInterface*>(&modi_sock));
      app->setSHMSocket(&shms);
      app->drawAll();
      app->setVisible(true);
      nanogui::mainloop();
    }
    
    modi_sock.stop();

    nanogui::shutdown();
  } catch (const std::runtime_error &e) {
    std::string error_msg = 
        std::string("Caught a fatal error: ") + std::string(e.what());
    LOG(ERROR) << error_msg;
    return -1;
  }

  return 0;
}