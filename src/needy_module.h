#ifndef NEEDY_MODULE_H
#define NEEDY_MODULE_H

#include <bomb_protocol.h>
#include <module.h>

namespace NeedyModule {
struct NeedyDisplay {};

void fail();
bool setup(String name);
void update();

}; // namespace NeedyModule

#endif // NEEDY_MODULE_H