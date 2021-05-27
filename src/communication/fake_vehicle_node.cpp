#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <signal.h>
#include <unistd.h>

#include "practical_socket.h"
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "bidi_tcp_socket.h"
#include "messages.h"

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 7001, "Foreign port");

std::atomic_bool running;

void myHandler(int s){
  running.store(false);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = myHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  running.store(true);

  try {
    LOG(INFO) << "Connecting to server: "
              << FLAGS_foreign_addr 
              << ":" 
              << FLAGS_foreign_port;
    int32_t speed = 0;

    relay::communication::BiDirectionalTCPSocket bidi_sock(
        new TCPSocket(FLAGS_foreign_addr, FLAGS_foreign_port));
    while (bidi_sock.running()) {
      bidi_sock.consume([&](std::deque<std::string>& buffer){
        if (!buffer.empty()) {
          if (buffer.back().size() == ControlCommand::size) {
            ControlCommand command = 
                ControlCommand::parseControlCommand(buffer.back().c_str());
            LOG(INFO) << "Receive <<< ";
            LOG(INFO) << "TR: " << static_cast<int>(command.takeover_request);
            LOG(INFO) << "YC: " << static_cast<int>(command.yaw_control);
            LOG(INFO) << "TC: " << static_cast<int>(command.throttle_control);
            LOG(INFO) << "GC: " << static_cast<int>(command.gear);
          }
          buffer.clear();
        }
      });
      if (running.load()) {
        VehicleState state = {0, 0, speed};
        char buffer[VehicleState::size];
        state.makeVehicleState(buffer);
        bidi_sock.push(VehicleState::size, buffer);
        LOG(INFO) << "Send <<< ";
        LOG(INFO) << "CM: " << static_cast<int>(state.control_mode);
        LOG(INFO) << "GR: " << static_cast<int>(state.gear);
        LOG(INFO) << "SP: " << static_cast<int>(state.speed);
      } else {
        bidi_sock.stop();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    bidi_sock.join();
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}