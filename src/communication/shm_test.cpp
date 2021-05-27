#include <memory>
#include <string>
#include <thread>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "messages.h"
#include "shm.h"

void worker1() {
  LOG(INFO) << "Worker 1";
  auto* r = shmmap<SPMCQueue<ControlCommand, 1024>>("SHM_TEST_1");
  if (!r) {
    return;
  }
  auto reader = r->getReader();

  auto* w = shmmap<SPMCQueue<VehicleState, 1024>>("SHM_TEST_2");
  if (!w) {
    return;
  }

  LOG(INFO) << "Worker 1 sending";
  w->write([](VehicleState& msg) {
    msg.control_mode = 0;
    msg.gear = 0;
    msg.speed = 0;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  LOG(INFO) << "Worker 1 sending";
  w->write([](VehicleState& msg) {
    msg.control_mode = 1;
    msg.gear = 0;
    msg.speed = 0;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  LOG(INFO) << "Worker 1 sending";
  w->write([](VehicleState& msg) {
    msg.control_mode = 2;
    msg.gear = 0;
    msg.speed = 0;
  });

  size_t received = 0;
  while (true) {
    ControlCommand* msg = reader.read();
    if (!msg) {
      continue;
    }
    LOG(INFO) << "Worker 1 receiving: " << (int)msg->yaw_control;
    received++;
    if (received >= 2) {
      break;
    }
  }
}

void worker2() {
  LOG(INFO) << "Worker 2";
  auto* r = shmmap<SPMCQueue<VehicleState, 1024>>("SHM_TEST_2");
  if (!r) {
    return;
  }
  auto reader = r->getReader();

  auto* w = shmmap<SPMCQueue<ControlCommand, 1024>>("SHM_TEST_1");
  if (!w) {
    return;
  }

  LOG(INFO) << "Worker 2 sending";
  w->write([](ControlCommand& msg) {
    msg.yaw_control = 0;
    msg.gear = 0;
    msg.takeover_request = 0;
    msg.throttle_control = 0;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  LOG(INFO) << "Worker 2 sending";
  w->write([](ControlCommand& msg) {
    msg.yaw_control = 1;
    msg.gear = 0;
    msg.takeover_request = 0;
    msg.throttle_control = 0;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  LOG(INFO) << "Worker 2 sending";
  w->write([](ControlCommand& msg) {
    msg.yaw_control = 2;
    msg.gear = 0;
    msg.takeover_request = 0;
    msg.throttle_control = 0;
  });

  size_t received = 0;
  while (true) {
    VehicleState* msg = reader.read();
    if (!msg) {
      continue;
    }
    LOG(INFO) << "Worker 2 receiving: " << (int)msg->control_mode;
    received++;
    if (received >= 2) {
      break;
    }
  }
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  LOG(INFO) << "Welcome";

  std::thread t1(worker1);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::thread t2(worker2);

  t1.join();
  t2.join();

  return 0;
}

