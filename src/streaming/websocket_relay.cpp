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

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/version.hpp>

#include "bidi_tcp_socket.h"
#include "h264_decoder.h"
#include "jsmpeg_encoder.h"


DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");

typedef websocketpp::server<websocketpp::config::asio> Server;

std::atomic_bool running;

void myHandler(int s){
  running.store(false);
}

void onOpen(websocketpp::connection_hdl hdl) {
{
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
  
  Server relay_server;
  relay_server.set_access_channels(websocketpp::log::alevel::all);
  relay_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
  relay_server.init_asio();

  m_server.set_open_handler(bind(&broadcast_server::on_open,this,::_1));

  running.store(true);

  return 0;
}