#ifndef ASSIST_HPP
#define ASSIST_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/glm/glm.hpp"

class Assist : public IManagedClass {
public:
    // Pulls cursor toward target when inside fov radius.
    // Strength scales with distance: stronger when closer to target.
    glm::vec2 algorithm(const glm::vec2& cursorPos, const glm::vec2& targetPos, float fov, float strength) const;
};

#endif // ASSIST_HPP
