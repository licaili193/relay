#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <thread>

#include <signal.h>
#include <unistd.h>

#include "practical_socket.h"
#include <glog/logging.h>
#include <gflags/gflags.h>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/version.hpp>

#include "bidi_tcp_socket.h"
#include "jsmpeg_encoder.h"


DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6001, "Foreign port");
DEFINE_int32(local_port, 8080, "Local port");

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::server<websocketpp::config::asio> Server;

enum JsmpegTrameType {
    jsmpeg_frame_type_video = 0xFA010000,
    jsmpeg_frame_type_audio = 0xFB010000
};

struct JsmpegFrame {
    JsmpegTrameType type;
    int size;
};

struct JsmpegHeader {
    unsigned char magic[4];
    unsigned short width;
    unsigned short height;
};

std::set<websocketpp::connection_hdl, 
         std::owner_less<websocketpp::connection_hdl>> connection_list;
websocketpp::lib::shared_ptr<websocketpp::lib::thread> ws_thread;

std::atomic_bool running;

void myHandler(int s){
  running.store(false);
}

inline int swap_int16(int in) {
  return ((in>>8)&0xff) | ((in<<8)&0xff00);
}

inline int swap_int32(int in) {
    return ((in>>24)&0xff) |
            ((in<<8)&0xff0000) |
            ((in>>8)&0xff00) |
            ((in<<24)&0xff000000);
}

void onOpen(Server* s, websocketpp::connection_hdl hdl) {
  JsmpegHeader header = 
      {{'j','s','m','p'}, swap_int16(640),  swap_int16(480)};

  try {
    s->send(hdl, &header, sizeof(header), websocketpp::frame::opcode::binary);
  } catch (websocketpp::exception const & e) {
    LOG(ERROR) << "Error sending message: " << e.what();
  }
  connection_list.insert(hdl);
  LOG(INFO) << "Client connected";
}

void onClose(Server* s, websocketpp::connection_hdl hdl) {
  connection_list.erase(hdl);
  LOG(INFO) << "Client disconnected";
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

  relay_server.set_open_handler(bind(&onOpen, &relay_server, ::_1));
  relay_server.set_close_handler(bind(&onClose, &relay_server, ::_1));

  char* send_buffer = new char[1024*1024];

  running.store(true);
  try {
    LOG(INFO) << "Connecting to server: "
              << FLAGS_foreign_addr 
              << ":" 
              << FLAGS_foreign_port;

    relay::communication::BiDirectionalTCPSocket bidi_sock(
        new TCPSocket(FLAGS_foreign_addr, FLAGS_foreign_port));

    relay_server.listen(FLAGS_local_port);
    relay_server.start_accept();
    ws_thread.reset(new websocketpp::lib::thread([&](){
      relay_server.run();
    }));

    while (bidi_sock.running()) {
      if (running.load()) {
        bidi_sock.consume([&](std::deque<std::string>& buffer){
          while (!buffer.empty()) {   
            for (auto it : connection_list) {
              
              JsmpegFrame frame;
              frame.type = jsmpeg_frame_type_video;
              frame.size = 
                  swap_int32(sizeof(JsmpegFrame) + buffer.front().size());
              memcpy(send_buffer, &frame, sizeof(frame));
              memcpy(send_buffer + sizeof(frame), 
                     buffer.front().c_str(), 
                     buffer.front().size());
              try {
                relay_server.send(it, 
                                  send_buffer, 
                                  sizeof(JsmpegFrame) + buffer.front().size(), 
                                  websocketpp::frame::opcode::binary);
              } catch (websocketpp::exception const & e) {
                LOG(ERROR) << "Error broadcasting message: " << e.what();
              }
            }
            buffer.pop_front();
          }
        });
      } else {
        bidi_sock.stop();
      }
    }

    delete[] send_buffer;

    bidi_sock.join();
    relay_server.stop();
    ws_thread->join();
  } catch (SocketException &e) {
    LOG(FATAL) << "Error occurred when accepting connection: " << e.what();
  }
  LOG(INFO) << "Bye bye";

  return 0;
}