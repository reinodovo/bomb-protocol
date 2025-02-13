#ifndef PUZZLE_MODULE_H
#define PUZZLE_MODULE_H

#include <bomb_protocol.h>
#include <module.h>

namespace PuzzleModule {
class StatusLight {
public:
  StatusLight() {};
  StatusLight(int redPin, int greenPin);
  StatusLight(int redPin, int greenPin, bool invert);
  void update(Module::Status status);
  void strike();

private:
  void setPin(int pin, bool state);
  int _redPin, _greenPin;
  bool _invert;
};

enum class LightStatus {
  Nothing,
  Off,
  Green,
  Red,
  Yellow,
};

extern StatusLight statusLight;

void solve();
void strike();
bool setup();
void update();
} // namespace PuzzleModule

#endif