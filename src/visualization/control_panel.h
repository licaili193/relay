#ifndef __CONTROL_PANEL__
#define __CONTROL_PANEL__

#include <chrono>

#include "nanogui_includes.h"

namespace relay {
namespace visualization {

class ControlPanel : public nanogui::Widget {
 public:
  ControlPanel(nanogui::Widget* parent) : nanogui::Widget(parent) {
    using namespace nanogui;

    setLayout(new GridLayout(
        Orientation::Vertical, 2, Alignment::Fill, 0, 5));
    
    control_status_ = new Label(this, "Control Status", "sans-bold");
    auto button_1 = new Button(this, "Command 1");
    button_1->setCallback([&] {
        LOG(INFO) << "Button 1 clicked";
    });
    throttling_status_ = new Label(this, "Throttling", "sans-bold");
    auto button_2 = new Button(this, "Command 2");
    button_2->setCallback([&] {
        LOG(INFO) << "Button 2 clicked";
    });
    steering_status_ = new Label(this, "Steering", "sans-bold");
    auto button_3 = new Button(this, "Command 3");
    button_3->setCallback([&] {
        LOG(INFO) << "Button 3 clicked";
    });
    gear_status_ = new Label(this, "Gear", "sans-bold");
    auto button_4 = new Button(this, "Command 4");
    button_4->setCallback([&] {
        LOG(INFO) << "Button 4 clicked";
    });
  }

  void forceRoutedKeyboardEvent(
      int key, int scancode, int action, int modifiers) {
    // If there is a joystick presents, ignore keyboard control
    if (has_joystick) {
      return;
    }

    if (glfwGetKey(screen()->glfwWindow(), GLFW_KEY_UP) == GLFW_PRESS) {
      throttle_ += t_sensitivity_;
      if (throttle_ > 100) {
        throttle_ = 100;
      }
      throttle_time_ = std::chrono::system_clock::now();
    } else if (
        glfwGetKey(screen()->glfwWindow(), GLFW_KEY_DOWN) == GLFW_PRESS) {
      throttle_ -= t_sensitivity_;
      if (throttle_ < -100) {
        throttle_ = -100;
      }
      throttle_time_ = std::chrono::system_clock::now();
    }

    if (glfwGetKey(screen()->glfwWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS) {
      steering_ += s_sensitivity_;
      if (steering_ > 100) {
        steering_ = 100;
      }
      steer_time_ = std::chrono::system_clock::now();
    } else if (
        glfwGetKey(screen()->glfwWindow(), GLFW_KEY_LEFT) == GLFW_PRESS) {
      steering_ -= s_sensitivity_;
      if (steering_ < -100) {
        steering_ = -100;
      }
      steer_time_ = std::chrono::system_clock::now();
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
      takeover_ = !takeover_;
    }
  }

  virtual void draw(NVGcontext *ctx) override {
    if (glfwJoystickPresent(GLFW_JOYSTICK_1) == GL_TRUE) {
      has_joystick = true;
      
      // TODO (YUHAN)
      int axesCount, buttonCount;
      const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);
      const unsigned char* buttons = 
          glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttonCount);
      throttle_ = int(100 * -axes[1]);
      steering_ = int(100 * -axes[0]);
      if (buttons[0] == 1 && !takeover_) {
        takeover_ = true;
      }
      if (buttons[2] == 1 && takeover_) {
        takeover_ = false;
      }
    } else {
      has_joystick = false;

      if (throttle_time_ + std::chrono::milliseconds(500) < 
          std::chrono::system_clock::now()) {
        if (throttle_ != 0) {
          if (throttle_ > 0) {
            throttle_ -= t_damping_;
            if (throttle_ < 0) {
              throttle_ = 0;
            }
          }  else {
            throttle_ += t_damping_;
            if (throttle_ > 0) {
              throttle_ = 0;
            }
          } 
          // throttle_time_ = std::chrono::system_clock::now();
        }
      }
      if (steer_time_ + std::chrono::milliseconds(500) < 
          std::chrono::system_clock::now()) {
        if (steering_ != 0) {
          if (steering_ > 0) {
            steering_ -= s_damping_;
            if (steering_ < 0) {
              steering_ = 0;
            }
          }  else {
            steering_ += s_damping_;
            if (steering_ > 0) {
              steering_ = 0;
            }
          } 
          // steer_time_ = std::chrono::system_clock::now();
        }
      }
    }

    if (control_status_) {
      if (takeover_) {
        control_status_->setCaption("Takeover Requested");
      } else {
        control_status_->setCaption("Monitoring");
      }
    }

    if (throttling_status_) {
      throttling_status_->setCaption(std::to_string(throttle_));
    }

    if (steering_status_) {
      steering_status_->setCaption(std::to_string(steering_));
    }

    if (gear_status_) {
      if (throttle_ < 0) {
        gear_status_->setCaption("Reversing");
      } else {
        gear_status_->setCaption("Forward");
      }
    }

    Widget::draw(ctx);
  }

  bool getTakeoverRequest() const {
    return takeover_;
  }

  int8_t getYawControl() const {
    return steering_;  
  }

  int8_t getThrottleControl() const {
    return throttle_ < 0 ? -throttle_ : throttle_;  
  }

  uint8_t getGear() const {
    return throttle_ < 0 ? 1 : 0;  
  }

 private:
  nanogui::Label* control_status_ = nullptr;
  nanogui::Label* throttling_status_ = nullptr;
  nanogui::Label* steering_status_ = nullptr;
  nanogui::Label* gear_status_ = nullptr;

  std::chrono::system_clock::time_point throttle_time_ = 
      std::chrono::system_clock::now();
  std::chrono::system_clock::time_point steer_time_ = 
      std::chrono::system_clock::now();

  const uint32_t t_sensitivity_ = 2;
  const uint32_t s_sensitivity_ = 2;
  const uint32_t t_damping_ = 1;
  const uint32_t s_damping_ = 1;

  bool takeover_ = false;
  int8_t throttle_ = 0;
  int8_t steering_ = 0;

  bool has_joystick = false;
};

}
}

#endif
