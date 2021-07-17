#ifndef __VISUALIZER__
#define __VISUALIZER__

#include "nanogui_includes.h"
#include "camera_gl_canvas.h"
#include "dash_board.h"
#include "control_panel.h"

#include "receive_interface.h"
#include "nv_hevc_sync_decoder.h"
#include "shm_socket.h"
#include "messages.h"

namespace relay {
namespace visualization {

class Visualizer : public nanogui::Screen {
 public:
  Visualizer(size_t width = 640, size_t height = 480) : 
      nanogui::Screen(Eigen::Vector2i(800, 600), "NanoGUI Test", true) {
    using namespace nanogui;

    image_width = width;
    image_height = height;

    mCanvas = new CameraGLCanvas(this);
    mCanvas->setBackgroundColor({100, 100, 100, 255});
    mCanvas->setCameraSize({width, height});

    mTools = new Widget(this);
    mTools->setLayout(new BoxLayout(
        Orientation::Horizontal, Alignment::Middle, 0, 20));

    d0 = new DashBoard(mTools);
    c0 = new ControlPanel(mTools);

    performLayout();

    decode_buffer = new char[image_width * image_height * 3];
  }

  ~Visualizer() {
    delete[] decode_buffer;
  }

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) {
      return true;
    }

    if (c0) {
      c0->forceRoutedKeyboardEvent(key, scancode, action, modifiers);
    }

    return false;
  }

  virtual void draw(NVGcontext *ctx) {
    CHECK(decoder) << "No decoder";
    CHECK(receive_interface) << "No receive interface";
    CHECK(shm_interface) << "No SHM interface";

    receive_interface->consume([&](std::deque<std::string>& buffer){
      while (!buffer.empty()) {
        decoder->push(buffer.front().size(), buffer.front().c_str());
        buffer.pop_front();

        size_t size = 0;
        while (decoder->get(size, decode_buffer));
        if (size) {
          mCanvas->setYUVData(
              reinterpret_cast<unsigned char*>(decode_buffer));
          mCanvas->setCameraSize({image_width, image_height});
        }
      }
    });

    /* Draw the user interface */
    float camera_aspect_ratio = 
        static_cast<float>(mCanvas->cameraSize().x()) / 
        static_cast<float>(mCanvas->cameraSize().y());

    constexpr int window_margin = 5;
    const auto& tool_size = mTools->preferredSize(ctx);
    const auto& window_size = size();

    int canvas_height = window_size.y() - tool_size.y() - 2 * window_margin;
    int canvas_width = window_size.x();

    float current_aspect_ratio = 
        static_cast<float>(canvas_width) / static_cast<float>(canvas_height);

    if (camera_aspect_ratio < current_aspect_ratio) {
      mCanvas->setSize(
          {camera_aspect_ratio * canvas_height, canvas_height});
      mCanvas->setPosition(
          {(canvas_width - camera_aspect_ratio * canvas_height) / 2, 0});
    } else {
      mCanvas->setSize(
          {canvas_width, canvas_width / camera_aspect_ratio});
      mCanvas->setPosition(
          {0, (canvas_height - canvas_width / camera_aspect_ratio) / 2});
    }

    mTools->setPosition(
        {(window_size.x() - tool_size.x()) / 2, canvas_height + window_margin});

    VehicleState* state = shm_interface->receive();
    if (state) {
      d0->setControlMode(state->control_mode);
      d0->setGear(state->gear);
      d0->setSpeed(state->speed);
    }

    ControlCommand cmd = {c0->getTakeoverRequest(), 
                          c0->getYawControl(), 
                          c0->getThrottleControl(), 
                          c0->getGear()};
    shm_interface->send(cmd);

    performLayout();
    Screen::draw(ctx);
  }
  
  void setDecoder(relay::codec::NvHEVCSyncDecoder* d) {
    decoder = d;
  }

  void setVideoSocket(relay::communication::ReceiveInterface* s) {
    receive_interface = s;
  }

  void setSHMSocket(
      relay::communication::SHMSocket<ControlCommand, VehicleState>* h) {
    shm_interface = h;
  }

 private:
  CameraGLCanvas *mCanvas;
  Widget *mTools;
  DashBoard *d0 = nullptr;
  ControlPanel *c0 = nullptr;

  relay::codec::NvHEVCSyncDecoder* decoder = nullptr;
  relay::communication::ReceiveInterface* receive_interface = nullptr;
  relay::communication::SHMSocket<ControlCommand, VehicleState>* shm_interface = nullptr;

  std::string internal_buffer;

  char* decode_buffer = nullptr;

  size_t image_width = 640;
  size_t image_height = 480;
};

}
}

#endif
