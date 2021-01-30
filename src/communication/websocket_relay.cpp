#include <atomic>
#include <iostream>

#include <signal.h>
#include <unistd.h>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <websocketpp/version.hpp>

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

  LOG(INFO) << "Websocket++ version: " << websocketpp::user_agent;

  return 0;
}