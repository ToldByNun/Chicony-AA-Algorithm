#include "Assist.hpp"

#include <algorithm>
#include <limits>

glm::vec2 Assist::algorithm(const glm::vec2& cursorPos, const glm::vec2& targetPos, float fov, float strength) const {
    const float distance = glm::distance(cursorPos, targetPos);

    if (distance > fov) return cursorPos;

    if (distance <= std::numeric_limits<float>::epsilon()) return targetPos;

    const float clampedStrength = std::clamp(strength, 0.f, 1.f);
    const float pull = (1.f - distance / fov) * clampedStrength;

    return glm::mix(cursorPos, targetPos, pull);
}
