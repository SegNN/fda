// Dota 2 engine globals via signature scan
#pragma once
#include <cstdint>

namespace engine {
    constexpr auto dwEntityList             = 0x662CD68; // client.dll
    constexpr auto dwViewMatrix             = 0x621EB30; // client.dll
    constexpr auto dwLocalPlayerController  = 0x6215E88; // client.dll
}
