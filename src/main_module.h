#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

#include <bomb_protocol.h>

namespace MainModule {
const int MAX_MODULES = 15;
const int SPEED_STAGES = 4;

using OnSolved = std::function<void()>;
using OnFailed = std::function<void()>;
using OnStrike = std::function<void(int strikes)>;

extern OnSolved onSolved;
extern OnFailed onFailed;
extern OnStrike onStrike;

bool setup();
void update();

void setMaxStrikes(int max_strikes);
void setDuration(unsigned long duration);

void startAfter(int seconds);
void reset();

BombInfo bombInfo();

int strikes();
int maxStrikes();

bool onStartCountdown();
bool started();
bool starting();
bool solved();
bool failed();

int code();

int speed();
char *timeStr(bool);
unsigned long timeToStart();
unsigned long timeToNextSecond();
} // namespace MainModule

#endif