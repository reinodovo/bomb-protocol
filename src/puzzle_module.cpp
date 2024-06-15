#include <map>
#include <set>

#include <puzzle_module.h>
#include <utils/debouncer.h>

namespace PuzzleModule {
const int STATUS_LIGHT_STRIKE_DURATION = 1000;
const int STATUS_LIGHT_STRIKE_BLINK_DURATION = 1000;
unsigned long _last_strike;

StatusLight _light;

void strike() {
  _light.strike();
  _last_strike = millis();
  SolveAttempt attempt;
  attempt.fail = false;
  attempt.strike = true;
  Module::queueSolveAttempt(attempt);
}

void solve() {
  Module::solve();
  SolveAttempt attempt;
  attempt.fail = attempt.strike = false;
  Module::queueSolveAttempt(attempt);
}

void updateStatusLight() {
  if (millis() - _last_strike < STATUS_LIGHT_STRIKE_BLINK_DURATION)
    return;
  _light.update(Module::status());
}

void update() {
  updateStatusLight();
  Module::update();
}

bool setup(StatusLight light) {
  _light = light;
  return Module::setup(ModuleType::Puzzle);
}

StatusLight::StatusLight(int redPin, int greenPin)
    : _redPin(redPin), _greenPin(greenPin), _invert(true) {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
}

StatusLight::StatusLight(int redPin, int greenPin, bool invert = true)
    : _redPin(redPin), _greenPin(greenPin), _invert(invert) {
  pinMode(_redPin, OUTPUT);
  pinMode(_greenPin, OUTPUT);
}

void StatusLight::setPin(int pin, bool state) {
  if (_invert)
    state = !state;
  digitalWrite(pin, state);
}

void StatusLight::update(Module::Status status) {
  if (status == Module::Status::Connecting) {
    bool state = (millis() / STATUS_LIGHT_STRIKE_BLINK_DURATION) % 2;
    setPin(_redPin, state);
    setPin(_greenPin, state);
  } else if (status == Module::Status::Connected) {
    setPin(_redPin, HIGH);
    setPin(_greenPin, HIGH);
  } else if (status == Module::Status::Started) {
    setPin(_redPin, LOW);
    setPin(_greenPin, LOW);
  } else if (status == Module::Status::Solved) {
    setPin(_redPin, LOW);
    setPin(_greenPin, HIGH);
  }
}

void StatusLight::strike() {
  setPin(_redPin, HIGH);
  setPin(_greenPin, LOW);
}
} // namespace PuzzleModule