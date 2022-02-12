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
#include "udp_send_socket.h"
#include "udp_receive_socket.h"
#include "shm_socket.h"
#include "messages.h"

DEFINE_int32(port, 7000, "Server port");
DEFINE_int32(foreign_to_port, 7001, "Foreign port to send to");
DEFINE_int32(foreign_from_port, 7002, "Foreign port to receive from");

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
    LOG(INFO) << "Accepting connection...";
    std::string foreign_addr = "";

    TCPServerSocket serv_sock(FLAGS_port);
    LOG(INFO) << "Accepting new connection";
    auto sock = serv_sock.accept();
    try {
      foreign_addr = sock->getForeignAddress();
      LOG(INFO) << "Connected accepted";
      LOG(INFO) << "Connection from: " 
                << foreign_addr; 
    } catch (SocketException e) {
      LOG(ERROR) << "Error occurred when getting connection info: " 
                  << e.what();
    }
    // TODO: to send foreign port via TCP
    // TODO: to accept connection again

    relay::communication::UDPSendSocket modi_to_sock(8000); // TODO: change this
    modi_to_sock.setForeign(foreign_addr, FLAGS_foreign_to_port);
    relay::communication::UDPReceiveSocket modi_from_sock(FLAGS_foreign_from_port);

    while (running.load() && modi_to_sock.running() && modi_from_sock.running()) {
      modi_from_sock.consume([&](std::deque<std::string>& buffer){
        if (!buffer.empty()) {
          if (buffer.back().size() == VehicleState::size) {
            VehicleState state = 
                VehicleState::parseVehicleState(buffer.back().c_str());
            shms.send(state);
          }
          buffer.clear();
        }
      });

      char buffer[ControlCommand::size];
      ControlCommand* command = shms.receive();
      if (command) {
        command->makeControlCommand(buffer);
        modi_to_sock.push(ControlCommand::size, buffer);
      }
    }

    modi_to_sock.stop();
    modi_from_sock.stop();
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}