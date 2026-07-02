#pragma once

#include "../imgui.h"

namespace Custom {

inline ImVec4 ColorFromHex(unsigned int hex) {
    return ImVec4(
        static_cast<float>((hex >> 16) & 0xFF) / 255.f,
        static_cast<float>((hex >> 8) & 0xFF) / 255.f,
        static_cast<float>(hex & 0xFF) / 255.f,
        1.f
    );
}

}
