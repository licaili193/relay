#ifndef __RECEIVE_INTERFACE__
#define __RECEIVE_INTERFACE__

#include <deque>
#include <functional>
#include <string>

namespace relay {
namespace communication {

class ReceiveInterface {
 public:
  ReceiveInterface() = default;

  virtual void consume(std::function<void(std::deque<std::string>&)> fun) = 0;
};

}
}

#endif