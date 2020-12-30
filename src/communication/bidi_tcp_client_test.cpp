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

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");

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
  worker.detach();

  try {
    LOG(INFO) << "Connecting to server: "
              << FLAGS_foreign_addr 
              << ":" 
              << FLAGS_foreign_port;

    relay::communication::BiDirectionalTCPSocket bidi_sock(
        new TCPSocket(FLAGS_foreign_addr, FLAGS_foreign_port));
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
    bidi_sock.join();
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}