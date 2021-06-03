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
#include "shm_socket.h"
#include "messages.h"

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 7000, "Foreign port");

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

  relay::communication::SHMSocket<VehicleState, ControlCommand> 
      shms("vehicle_state", "control_command");

  running.store(true);

  try {
    LOG(INFO) << "Connecting to server: "
              << FLAGS_foreign_addr 
              << ":" 
              << FLAGS_foreign_port;

    relay::communication::BiDirectionalTCPSocket bidi_sock(
        new TCPSocket(FLAGS_foreign_addr, FLAGS_foreign_port));
    while (bidi_sock.running()) {
      bidi_sock.consume([&](std::deque<std::string>& buffer){
        if (!buffer.empty()) {
          if (buffer.back().size() == VehicleState::size) {
            VehicleState state = 
                VehicleState::parseVehicleState(buffer.back().c_str());
            shms.send(state);
          }
          buffer.clear();
        }
      });
      if (running.load()) {
        char buffer[ControlCommand::size];
        ControlCommand* command = shms.receive();
        if (command) {
          command->makeControlCommand(buffer);
          bidi_sock.push(ControlCommand::size, buffer);
        }
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