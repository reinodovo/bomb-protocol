#ifndef MODULE_H
#define MODULE_H

#include <bomb_protocol.h>

namespace Module {
enum class Status { Connecting, Connected, Started, Solved };

using OnRestart = std::function<void()>;
using OnStart = std::function<void()>;
using OnManualCode = std::function<void(int)>;

extern OnRestart onRestart;
extern OnStart onStart;
extern OnManualCode onManualCode;

bool setup(ModuleType type);
void withBombInfo(BombInfoCallback callback);
void queueSolveAttempt(SolveAttempt attempt);
Status status();
void update();
void solve();
}; // namespace Module

#endif // MODULE_H