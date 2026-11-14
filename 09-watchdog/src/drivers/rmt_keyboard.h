#pragma once
#include <stdint.h>
#include <functional>

namespace drivers {

enum class RmtKeyboardKey {
  Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Asterisk, Sharp, Up, Down, Left, Right, OK, Undefined,
};

using kbCallback_t = std::function<void(RmtKeyboardKey key)>;

class RmtKeyboard {
public:
  virtual ~RmtKeyboard() = default;

  virtual void begin() = 0;
  virtual void onRx(kbCallback_t cb) = 0;
};

}  //namespace drivers
