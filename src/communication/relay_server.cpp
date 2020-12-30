#include <atomic>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <signal.h>
#include <unistd.h>

#include "practical_socket.h"
#include <glog/logging.h>
#include <gflags/gflags.h>

DEFINE_int32(port_1, 7000, "Local port 1");
DEFINE_int32(port_2, 7001, "Local port 2");

#define COMMAND_DISCONNECT 1
#define FIN_INDEX_KEY 12245
#define FIN_SIZE_KEY 5132466

#define MAX_BUFFER_SIZE 10
#define TCP_BUFFER_SIZE 87380

std::atomic_bool running;

void myHandler(int s){
  running.store(false);
}

std::deque<std::string> buffer_1;
std::mutex mu_1;
std::deque<std::string> buffer_2;
std::mutex mu_2;

char fin_test_1[9];
char fin_test_2[9];

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = myHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  uint32_t tmp_1 = FIN_INDEX_KEY;
  uint32_t tmp_2 = FIN_SIZE_KEY;
  uint8_t tmp_3 = COMMAND_DISCONNECT;
  memcpy(fin_test_1, &tmp_1, sizeof(uint32_t));
  memcpy(fin_test_1 + sizeof(uint32_t), &tmp_2, sizeof(uint32_t));
  memcpy(fin_test_1 + 2 * sizeof(uint32_t), &tmp_3, sizeof(uint8_t));
  memcpy(fin_test_2, fin_test_1, 9);

  running.store(true);

  std::thread t_1([&](){
    try {
      TCPServerSocket serv_sock(FLAGS_port_1);

      while (running.load()) {
        LOG(INFO) << "[Con1] Accepting new connection";
        auto sock = std::unique_ptr<TCPSocket>(serv_sock.accept());
        try {
          LOG(INFO) << "[Con1] Connected accepted";
          LOG(INFO) << "[Con1] Connection from: " 
                    << sock->getForeignAddress() 
                    << ":" 
                    << sock->getForeignPort();
        } catch (SocketException e) {
          LOG(ERROR) << "[Con1] Error occurred when getting connection info: " 
                    << e.what();
        }

        bool inner_loop_running = true;
        sock->setBlocking(false);
        {
          std::lock_guard<std::mutex> guard(mu_1);
          buffer_1.clear();
        }
        while (running.load() && inner_loop_running) {
          char buffer[TCP_BUFFER_SIZE];
          int recv_size;
          try {
            while ((recv_size = sock->recv(buffer, TCP_BUFFER_SIZE)) > 0) {
              if (recv_size == 9 && memcmp(buffer, fin_test_1, 9) == 0) {
                inner_loop_running = false;
              }
              std::lock_guard<std::mutex> guard(mu_2);
              if (buffer_2.size() >= MAX_BUFFER_SIZE) {
                buffer_2.pop_front();
              }
              buffer_2.emplace_back(buffer, recv_size);
            }
          } catch (SocketException e) {
            LOG(WARNING) << "[Con1] Error occurred when getting message: " 
                         << e.what();
            LOG(WARNING) << "[Con1] Disconnecting...";
            break;
          }

          {
            std::lock_guard<std::mutex> guard(mu_1);
            while (!buffer_1.empty()) {
              sock->send(buffer_1.front().c_str(), buffer_1.front().size());
              buffer_1.pop_front();
            }
          }
        }
      }
    } catch (SocketException &e) {
      LOG(FATAL) << "[Con1] Error occurred when accepting connection: " 
                 << e.what();
    }
    LOG(INFO) << "[Con1] Bye bye";
  });

  std::thread t_2([&](){
    try {
      TCPServerSocket serv_sock(FLAGS_port_2);

      while (running.load()) {
        LOG(INFO) << "[Con2] Accepting new connection";
        auto sock = std::unique_ptr<TCPSocket>(serv_sock.accept());
        try {
          LOG(INFO) << "[Con2] Connected accepted";
          LOG(INFO) << "[Con2] Connection from: " 
                    << sock->getForeignAddress() 
                    << ":" 
                    << sock->getForeignPort();
        } catch (SocketException e) {
          LOG(ERROR) << "[Con2] Error occurred when getting connection info: " 
                    << e.what();
        }

        bool inner_loop_running = true;
        sock->setBlocking(false);
        {
          std::lock_guard<std::mutex> guard(mu_2);
          buffer_2.clear();
        }
        while (running.load() && inner_loop_running) {
          char buffer[TCP_BUFFER_SIZE];
          int recv_size;
          try {
            while ((recv_size = sock->recv(buffer, TCP_BUFFER_SIZE)) > 0) {
              if (recv_size == 9 && memcmp(buffer, fin_test_2, 9) == 0) {
                inner_loop_running = false;
              }
              std::lock_guard<std::mutex> guard(mu_1);
              if (buffer_1.size() >= MAX_BUFFER_SIZE) {
                buffer_1.pop_front();
              }
              buffer_1.emplace_back(buffer, recv_size);
            }
          } catch (SocketException e) {
            LOG(WARNING) << "[Con2] Error occurred when getting message: " 
                         << e.what();
            LOG(WARNING) << "[Con2] Disconnecting...";
            break;
          }

          {
            std::lock_guard<std::mutex> guard(mu_2);
            while (!buffer_2.empty()) {
              sock->send(buffer_2.front().c_str(), buffer_2.front().size());
              buffer_2.pop_front();
            }
          }
        }
      }
    } catch (SocketException &e) {
      LOG(FATAL) << "[Con2] Error occurred when accepting connection: " 
                 << e.what();
    }
    LOG(INFO) << "[Con2] Bye bye";
  });

  t_1.detach();
  t_2.detach();
  while (running.load()) {
    usleep(100000);
  }
  return 0;
}