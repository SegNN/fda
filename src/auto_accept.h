#pragma once
#include "cosmetic_core.h"
#include <string>
namespace autoaccept {
void Observe(uint32_t type,const cosmetic::Bytes& raw);
void Tick(bool inMatch);
std::string Status();
void ClearTemplate();
}
