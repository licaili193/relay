#ifndef __DASH_BOARD__
#define __DASH_BOARD__

#include <iomanip>
#include <sstream>
#include <limits>
#include <cmath>

#include "nanogui_includes.h"

namespace relay {
namespace visualization {

class DashBoard : public nanogui::Widget {
 public:
  DashBoard(nanogui::Widget* parent) : nanogui::Widget(parent) {
    using namespace nanogui;

    setLayout(new BoxLayout(
        Orientation::Vertical, Alignment::Fill, 0, 5));
    
    control_mode_label_ = new Label(this, "Control Mode", "sans-bold");
    gear_label_ = new Label(this, "Gear", "sans-bold");
    speed_label_ = new Label(this, "Speed", "sans-bold");
  }

  void setControlMode(uint8_t mode) {
    if (control_mode_label_) {
      switch (mode) {
        case 0:
          control_mode_label_->setCaption("Manual");
          control_mode_label_->setColor(nanogui::Color(255, 255, 255, 255));
          break;
        case 1:
          control_mode_label_->setCaption("Auto");
          control_mode_label_->setColor(nanogui::Color(50, 250, 50, 255));
          break;
        case 2:
          control_mode_label_->setCaption("Remote");
          control_mode_label_->setColor(nanogui::Color(50, 50, 250, 255));
          break;
        default:
          control_mode_label_->setCaption("Unkown Mode");
          control_mode_label_->setColor(nanogui::Color(50, 50, 50, 255));
          break;
      }  
    }
  }

  void setGear(uint8_t gear) {
    if (gear_label_) {
      switch (gear) {
        case 0:
          gear_label_->setCaption("D ->");
          break;
        case 1:
          gear_label_->setCaption("R <-");
          break;
        default:
          gear_label_->setCaption("Unkown Gear");
          break;
      }  
    }
  }

  void setSpeed(int32_t in_speed) {
    float speed = static_cast<float>(in_speed) / 
                  static_cast<float>(std::numeric_limits<int32_t>::max()) *
                  100.f;
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << speed;
    std::string s = stream.str();
    s += " m/s";
    speed_label_->setCaption(s);
  }

 private:
  nanogui::Label* control_mode_label_ = nullptr;
  nanogui::Label* gear_label_ = nullptr;
  nanogui::Label* speed_label_ = nullptr;
};

}
}

#endif
