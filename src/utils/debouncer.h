#ifndef DEBOUNCER_H
#define DEBOUNCER_H

#include <functional>

struct Debouncer {
public:
  Debouncer(unsigned long delay);
  bool operator()(std::function<void()>);

private:
  unsigned long _delay;
  unsigned long _last_execution;
};

#endif // DEBOUNCER_H