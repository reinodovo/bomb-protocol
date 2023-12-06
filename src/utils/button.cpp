#include <Arduino.h>
#include <utils/button.h>

const int DEBOUNCE_TIME = 50;
const int HOLD_TIME = 1000;

void Button::update() {
  bool pressed = digitalRead(_pin);
  if (pressed != _last_pressed)
    _last_debounce_time = millis();
  if (millis() - _last_debounce_time > DEBOUNCE_TIME) {
    ButtonState nextState = pressed ? Pressed : Released;
    if (pressed && millis() - _last_debounce_time > HOLD_TIME)
      nextState = Held;
    if (nextState != _state) {
      ButtonState last_state = _state;
      _state = nextState;
      if (onStateChange != nullptr)
        onStateChange(_state, last_state);
    }
  }
  _last_pressed = pressed;
}

Button::Button(int pin) : _pin(pin) { pinMode(_pin, INPUT); }