#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

enum ButtonState {
  Released,
  Pressed,
  Held,
};

struct Button {
public:
  Button() {}
  Button(int pin);
  void update();
  ButtonState state() { return _state; }
  std::function<void(ButtonState, ButtonState)> onStateChange;

private:
  int _pin;
  bool _last_pressed;
  ButtonState _state;
  unsigned long _last_debounce_time;
};

#endif // BUTTON_H