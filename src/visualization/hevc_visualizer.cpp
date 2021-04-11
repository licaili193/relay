#include "nanogui_includes.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <cstdint>
#include <memory>
#include <utility>

#include <cuda.h>
#include <signal.h>
#include <unistd.h>

#include "practical_socket.h"
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "NvCodecUtils.h"
#include "bidi_tcp_socket.h"
#include "nv_hevc_decoder.h"


#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

DEFINE_string(foreign_addr, "127.0.0.1", "Foreign address");
DEFINE_int32(foreign_port, 6000, "Foreign port");

class CameraGLCanvas : public nanogui::GLCanvas {
 public:
  CameraGLCanvas(Widget *parent) : nanogui::GLCanvas(parent) {
    using namespace nanogui;

    camera_size = {600, 800};

    const char* vertex_shader = 
        R"(
          #version 410

          in vec4 vertexIn;
          in vec2 textureIn;
          out vec2 textureOut;

          void main(void) {
            gl_Position = vertexIn;
            textureOut = textureIn;
          }
        )";

    const char* fragment_shader = 
        R"(
          #version 410

          in vec2 textureOut;
          out vec4 fragColor;

          uniform sampler2D tex_y;
          uniform sampler2D tex_u;
          uniform sampler2D tex_v;

          void main(void) {
            vec3 yuv;
            vec3 rgb;
                
            yuv.x = texture(tex_y, textureOut).r;
            yuv.y = texture(tex_u, textureOut).r - 0.5;
            yuv.z = texture(tex_v, textureOut).r - 0.5;
                
            rgb = mat3( 1,       1,         1,
                        0,       -0.21482,  2.12798,
                        1.28033, -0.38059,  0) * yuv;
                
            fragColor = vec4(rgb, 1);
          }
        )";

    mShader.init(
        /* An identifying name */
        "a_simple_shader",
        /* Vertex shader */
        vertex_shader,
        /* Fragment shader */
        fragment_shader
    );

    MatrixXu indices(3, 2);
    indices.col( 0) << 0, 1, 3;
    indices.col( 1) << 1, 2, 3;

    MatrixXf verteices(2, 4);
    verteices.col(0) << 1.0f, -1.0f;
    verteices.col(1) << 1.0f, 1.0f;
    verteices.col(2) << -1.0f, 1.0f;
    verteices.col(3) << -1.0f, -1.0f;

    MatrixXf textures(2, 4);
    textures.col(0) << 1.0f, 1.0f;
    textures.col(1) << 1.0f, 0.0f;
    textures.col(2) << 0.0f, 0.0f;
    textures.col(3) << 0.0f, 1.0f;

    mShader.bind();

    textureUniformY = mShader.uniform("tex_y");
    textureUniformU = mShader.uniform("tex_u");
    textureUniformV = mShader.uniform("tex_v");

    glGenTextures(1, &id_y);
    glBindTexture(GL_TEXTURE_2D, id_y);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
    glGenTextures(1, &id_u);
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
    glGenTextures(1, &id_v);
    glBindTexture(GL_TEXTURE_2D, id_v);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    mShader.uploadIndices(indices);

    mShader.uploadAttrib("vertexIn", verteices);
    mShader.uploadAttrib("textureIn", textures);
  }

  ~CameraGLCanvas() {
    mShader.free();
  }

  virtual void drawGL() override {
    using namespace nanogui;

    if (yuv420_data) {
      mShader.bind();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, id_y);
      glTexImage2D(GL_TEXTURE_2D, 
                   0, 
                   GL_RED, 
                   camera_size.x(), 
                   camera_size.y(), 
                   0, 
                   GL_RED, 
                   GL_UNSIGNED_BYTE, 
                   yuv420_data);
      glUniform1i(textureUniformY, 0);
            
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, id_u);
      glTexImage2D(GL_TEXTURE_2D, 
                   0, 
                   GL_RED, 
                   camera_size.x() / 2, 
                   camera_size.y() / 2, 
                   0, 
                   GL_RED, 
                   GL_UNSIGNED_BYTE, 
                   (char*)yuv420_data + camera_size.x()*camera_size.y());
      glUniform1i(textureUniformU, 1);
            
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, id_v);
      glTexImage2D(GL_TEXTURE_2D, 
                   0, 
                   GL_RED, 
                   camera_size.x() / 2, 
                   camera_size.y() / 2, 
                   0, 
                   GL_RED, 
                   GL_UNSIGNED_BYTE, 
                   (char*)yuv420_data + 
                       camera_size.x()*camera_size.y() * 5 / 4);
      glUniform1i(textureUniformV, 2);

      mShader.drawIndexed(GL_TRIANGLES, 0, 2);
    }
  }

  void setCameraSize(Eigen::Vector2i size) {
    camera_size = size;
  }

  const Eigen::Vector2i& cameraSize() const {
    return camera_size;
  }

  void setYUVData(unsigned char* data) {
    yuv420_data = data;
  }

 private:
  nanogui::GLShader mShader;

  Eigen::Vector2i camera_size;

  GLuint textureUniformY;
  GLuint textureUniformU;
  GLuint textureUniformV;

  GLuint id_y;
  GLuint id_u;
  GLuint id_v;

  unsigned char* yuv420_data = nullptr;
};


class Visualizer : public nanogui::Screen {
 public:
  Visualizer() : 
      nanogui::Screen(Eigen::Vector2i(800, 600), "NanoGUI Test", true) {
    using namespace nanogui;

    mCanvas = new CameraGLCanvas(this);
    mCanvas->setBackgroundColor({100, 100, 100, 255});
    mCanvas->setCameraSize({640, 360});

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
    CHECK(socket) << "No socket";

    socket->comsume([&](std::deque<std::string>& buffer){
      while (!buffer.empty()) {
        decoder->push(std::move(buffer.front()));
        buffer.pop_front();
      }
    });

    bool got_decoded_frame = false;
    decoder->comsume([&](std::deque<std::string>& buffer) {
      if (!buffer.empty()) {
        internal_buffer = std::move(buffer.front());
        got_decoded_frame = true;
        mCanvas->setYUVData(
            const_cast<unsigned char*>(
                reinterpret_cast<const unsigned char*>(
                    internal_buffer.c_str())));
        mCanvas->setCameraSize({640, 360});
        buffer.pop_front();
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
          {canvas_width, camera_aspect_ratio * canvas_width});
      mCanvas->setPosition(
          {0, (canvas_height - camera_aspect_ratio * canvas_width) / 2});
    }

    mTools->setPosition(
        {(window_size.x() - tool_size.x()) / 2, canvas_height + window_margin});

    performLayout();
    Screen::draw(ctx);
  }
  
  void setDecoder(relay::codec::NvHEVCDecoder* d) {
    decoder = d;
  }

  void setSocket(relay::communication::BiDirectionalTCPSocket* s) {
    socket = s;
  }

 private:
  CameraGLCanvas *mCanvas;
  Widget *mTools;

  relay::codec::NvHEVCDecoder* decoder = nullptr;
  relay::communication::BiDirectionalTCPSocket* socket = nullptr;

  std::string internal_buffer;
};

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  try {
    LOG(INFO) << "Connecting to server: "
              << FLAGS_foreign_addr 
              << ":" 
              << FLAGS_foreign_port;

    relay::communication::BiDirectionalTCPSocket bidi_sock(
        new TCPSocket(FLAGS_foreign_addr, FLAGS_foreign_port));

    int iGpu = 0;
    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    if (iGpu < 0 || iGpu >= nGpu) {
        LOG(FATAL) << "GPU ordinal out of range. Should be within"
                    " [" << 0 << ", " << nGpu - 1 << "]";
    }

    CUcontext cuContext = NULL;
    createCudaContext(&cuContext, iGpu, 0);
    
    relay::codec::NvHEVCDecoder decoder(cuContext);

    nanogui::init();

    /* scoped variables */ 
    {
      nanogui::ref<Visualizer> app = new Visualizer();
      app->setDecoder(&decoder);
      app->setSocket(&bidi_sock);
      app->drawAll();
      app->setVisible(true);
      nanogui::mainloop();
    }
    
    bidi_sock.stop();

    nanogui::shutdown();
  } catch (const std::runtime_error &e) {
    std::string error_msg = 
        std::string("Caught a fatal error: ") + std::string(e.what());
    LOG(ERROR) << error_msg;
    return -1;
  }

  return 0;
}