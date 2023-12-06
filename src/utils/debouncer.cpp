#include <Arduino.h>
#include <utils/debouncer.h>

Debouncer::Debouncer(unsigned long delay) : _delay(delay), _last_execution(0) {}
bool Debouncer::operator()(std::function<void()> executor) {
  if (millis() - _last_execution < _delay)
    return false;
  _last_execution = millis();
  executor();
  return true;
}