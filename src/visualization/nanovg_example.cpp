#include "nanogui_includes.h"
#include <string>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

// Includes for the GLTexture class.
#include <cstdint>
#include <memory>
#include <utility>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

class MyGLCanvas : public nanogui::GLCanvas {
 public:
  MyGLCanvas(Widget *parent) : 
      nanogui::GLCanvas(parent), 
      mRotation(nanogui::Vector3f(0.25f, 0.5f, 0.33f)) {
    using namespace nanogui;

    const char* vertex_shader = 
        R"(
          #version 330
          uniform mat4 modelViewProj;
          in vec3 position;
          in vec3 color;
          out vec4 frag_color;
          void main() {
            frag_color = 3.0 * modelViewProj * vec4(color, 1.0);
            gl_Position = modelViewProj * vec4(position, 1.0);
          }
        )";

    const char* fragment_shader = 
        R"(
          #version 330
          out vec4 color;
          in vec4 frag_color;
          void main() {
            color = frag_color;
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

    MatrixXu indices(3, 12); /* Draw a cube */
    indices.col( 0) << 0, 1, 3;
    indices.col( 1) << 3, 2, 1;
    indices.col( 2) << 3, 2, 6;
    indices.col( 3) << 6, 7, 3;
    indices.col( 4) << 7, 6, 5;
    indices.col( 5) << 5, 4, 7;
    indices.col( 6) << 4, 5, 1;
    indices.col( 7) << 1, 0, 4;
    indices.col( 8) << 4, 0, 3;
    indices.col( 9) << 3, 7, 4;
    indices.col(10) << 5, 6, 2;
    indices.col(11) << 2, 1, 5;

    MatrixXf positions(3, 8);
    positions.col(0) << -1,  1,  1;
    positions.col(1) << -1,  1, -1;
    positions.col(2) <<  1,  1, -1;
    positions.col(3) <<  1,  1,  1;
    positions.col(4) << -1, -1,  1;
    positions.col(5) << -1, -1, -1;
    positions.col(6) <<  1, -1, -1;
    positions.col(7) <<  1, -1,  1;

    MatrixXf colors(3, 12);
    colors.col( 0) << 1, 0, 0;
    colors.col( 1) << 0, 1, 0;
    colors.col( 2) << 1, 1, 0;
    colors.col( 3) << 0, 0, 1;
    colors.col( 4) << 1, 0, 1;
    colors.col( 5) << 0, 1, 1;
    colors.col( 6) << 1, 1, 1;
    colors.col( 7) << 0.5, 0.5, 0.5;
    colors.col( 8) << 1, 0, 0.5;
    colors.col( 9) << 1, 0.5, 0;
    colors.col(10) << 0.5, 1, 0;
    colors.col(11) << 0.5, 1, 0.5;

    mShader.bind();
    mShader.uploadIndices(indices);

    mShader.uploadAttrib("position", positions);
    mShader.uploadAttrib("color", colors);
  }

  ~MyGLCanvas() {
    mShader.free();
  }

  void setRotation(nanogui::Vector3f vRotation) {
    mRotation = vRotation;
  }

  virtual void drawGL() override {
    using namespace nanogui;

    mShader.bind();

    Matrix4f mvp;
    mvp.setIdentity();
    float fTime = (float)glfwGetTime();
    mvp.topLeftCorner<3,3>() = Eigen::Matrix3f(
        Eigen::AngleAxisf(mRotation[0]*fTime, Vector3f::UnitX()) *
        Eigen::AngleAxisf(mRotation[1]*fTime,  Vector3f::UnitY()) *
        Eigen::AngleAxisf(mRotation[2]*fTime, Vector3f::UnitZ())) * 0.25f;

    mShader.setUniform("modelViewProj", mvp);

    glEnable(GL_DEPTH_TEST);
    /* Draw 12 triangles starting at index 0 */
    mShader.drawIndexed(GL_TRIANGLES, 0, 12);
    glDisable(GL_DEPTH_TEST);
  }

 private:
  nanogui::GLShader mShader;
  Eigen::Vector3f mRotation;
};


class ExampleApplication : public nanogui::Screen {
 public:
  ExampleApplication() : 
      nanogui::Screen(Eigen::Vector2i(800, 600), "NanoGUI Test", true) {
    using namespace nanogui;

    setLayout(new BoxLayout(
        Orientation::Vertical, Alignment::Middle, 0, 5));

    mCanvas = new MyGLCanvas(this);
    mCanvas->setBackgroundColor({100, 100, 100, 255});
    mCanvas->setSize({400, 400});

    tools = new Widget(this);
    tools->setLayout(new BoxLayout(
        Orientation::Horizontal, Alignment::Middle, 0, 5));

    Button *b0 = new Button(tools, "Random Color");
    b0->setCallback([this]() { 
      mCanvas->setBackgroundColor(
          Vector4i(rand() % 256, rand() % 256, rand() % 256, 255)); 
    });

    Button *b1 = new Button(tools, "Random Rotation");
    b1->setCallback([this]() { 
      mCanvas->setRotation(
          nanogui::Vector3f((rand() % 100) / 100.0f, 
                            (rand() % 100) / 100.0f, 
                            (rand() % 100) / 100.0f)); 
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
    /* Draw the user interface */
    constexpr int window_margin = 10;
    const auto& tool_size = tools->layout()->preferredSize(ctx, tools);
    const auto& window_size = size();
    mCanvas->setSize(
        {window_size.x(), window_size.y() - tool_size.y() - window_margin});
    performLayout();
    Screen::draw(ctx);
  }

 private:
  MyGLCanvas *mCanvas;
  Widget *tools;
};

DEFINE_string(picture, "", "Sample picture path file");

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  try {
    nanogui::init();

    /* scoped variables */ 
    {
      nanogui::ref<ExampleApplication> app = new ExampleApplication();
      app->drawAll();
      app->setVisible(true);
      nanogui::mainloop();
    }

    nanogui::shutdown();
  } catch (const std::runtime_error &e) {
    std::string error_msg = 
        std::string("Caught a fatal error: ") + std::string(e.what());
    LOG(ERROR) << error_msg;
    return -1;
  }

  return 0;
}