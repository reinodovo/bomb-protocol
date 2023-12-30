#ifndef PUZZLE_MODULE_H
#define PUZZLE_MODULE_H

#include <bomb_protocol.h>

namespace PuzzleModule {
enum class ModuleStatus { Connecting, Connected, Started, Solved };

class StatusLight {
public:
  StatusLight(){};
  StatusLight(int redPin, int greenPin);
  StatusLight(int redPin, int greenPin, bool invert);
  void update(ModuleStatus status);
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

using OnRestart = std::function<void()>;
using OnStart = std::function<void()>;
using OnManualCode = std::function<void(int)>;

extern OnRestart onRestart;
extern OnStart onStart;
extern OnManualCode onManualCode;

bool setup(StatusLight light);
void withBombInfo(BombInfoCallback callback);
ModuleStatus status();
void solve();
void strike();
void update();
} // namespace PuzzleModule

#endif