#ifndef __CAMERA_GL_CANVAS__
#define __CAMERA_GL_CANVAS__

#include "nanogui_includes.h"

namespace relay {
namespace visualization {

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
      glGenTextures(1, &id_y);
      glBindTexture(GL_TEXTURE_2D, id_y);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
      glGenTextures(1, &id_u);
      glBindTexture(GL_TEXTURE_2D, id_u);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
      glGenTextures(1, &id_v);
      glBindTexture(GL_TEXTURE_2D, id_v);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

      glDeleteTextures(1, &id_y);
      glDeleteTextures(1, &id_u);
      glDeleteTextures(1, &id_v);
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

}
}

#endif
