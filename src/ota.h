#ifndef OTA_H
#define OTA_H

#include <Arduino.h>

namespace OTA {
bool shouldStart();
void update();
void start(String version, String name);
}; // namespace OTA

#endif // OTA_H