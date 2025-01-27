#include <needy_module.h>

namespace NeedyModule {
void fail() {
  SolveAttempt attempt;
  attempt.strike = false;
  attempt.fail = true;
  Module::queueSolveAttempt(attempt);
}

bool setup(String name) { return Module::setup(name, Needy); }

void update() { Module::update(); }
}; // namespace NeedyModule