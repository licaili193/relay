#include <atomic>
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

DEFINE_int32(port, 6000, "Local port");

std::atomic_bool running;

void myHandler(int s){
  running.store(false);
}

std::string input = "";
bool has_new_input = false;
std::mutex mu;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = myHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  running.store(true);

  std::thread worker([&]() {
    while (running.load()) {
      std::string temp;
      std::cout << ">> ";
      std::cin >> temp;

      if (temp == "exit") {
        LOG(INFO) << "Exiting...";
        running.store(false);
      } else {
        std::lock_guard<std::mutex> guard(mu);
        input = std::move(temp);
        has_new_input = true;
      }
    }
  });

  try {
    TCPServerSocket serv_sock(FLAGS_port);
  
    while (running.load()) {
      LOG(INFO) << "Accepting new connection";
      auto sock = serv_sock.accept();
      try {
        LOG(INFO) << "Connected accepted";
        LOG(INFO) << "Connection from: " 
                  << sock->getForeignAddress() 
                  << ":" 
                  << sock->getForeignPort();
      } catch (SocketException e) {
        LOG(ERROR) << "Error occurred when getting connection info: " 
                   << e.what();
      }

      relay::communication::BiDirectionalTCPSocket bidi_sock(sock);
      while (bidi_sock.running()) {
        bidi_sock.comsume([](std::deque<std::string>& buffer){
          while (!buffer.empty()) {
            LOG(INFO) << "Message received: " << buffer.front();
            buffer.pop_front();
          }
        });
        if (running.load()) {
          std::lock_guard<std::mutex> guard(mu);
          if (has_new_input) {
            bidi_sock.push(input.size(), input.c_str());
            LOG(INFO) << "Message sent: " << input;
            has_new_input = false;
          }
        } else {
          bidi_sock.stop();
        }
      }
    }
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}