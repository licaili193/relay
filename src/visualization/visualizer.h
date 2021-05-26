#ifndef __VISUALIZER__
#define __VISUALIZER__

#include "nanogui_includes.h"
#include "camera_gl_canvas.h"

#include "receive_interface.h"
#include "nv_hevc_decoder.h"

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
        Orientation::Horizontal, Alignment::Middle, 0, 5));

    Button *b0 = new Button(mTools, "Random Color");
    b0->setCallback([this]() { 
      // Do nothing 
    });

    Button *b1 = new Button(mTools, "Random Rotation");
    b1->setCallback([this]() { 
      // Do nothing
    });

    performLayout();
  }

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) {
      return true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      setVisible(false);
      return true;
    }
    return false;
  }

  virtual void draw(NVGcontext *ctx) {
    CHECK(decoder) << "No decoder";
    CHECK(receive_interface) << "No receive interface";

    receive_interface->consume([&](std::deque<std::string>& buffer){
      while (!buffer.empty()) {
        decoder->push(std::move(buffer.front()));
        buffer.pop_front();
      }
    });

    bool got_decoded_frame = false;
    decoder->consume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        internal_buffer = std::move(buffer.back());
        got_decoded_frame = true;
        mCanvas->setYUVData(
            const_cast<unsigned char*>(
                reinterpret_cast<const unsigned char*>(
                    internal_buffer.c_str())));
        mCanvas->setCameraSize({image_width, image_height});
        buffer.clear();
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

    performLayout();
    Screen::draw(ctx);
  }
  
  void setDecoder(relay::codec::NvHEVCDecoder* d) {
    decoder = d;
  }

  void setVideoSocket(relay::communication::ReceiveInterface* s) {
    receive_interface = s;
  }

 private:
  CameraGLCanvas *mCanvas;
  Widget *mTools;

  relay::codec::NvHEVCDecoder* decoder = nullptr;
  relay::communication::ReceiveInterface* receive_interface = nullptr;

  std::string internal_buffer;

  size_t image_width = 640;
  size_t image_height = 480;
};

}
}

#endif
