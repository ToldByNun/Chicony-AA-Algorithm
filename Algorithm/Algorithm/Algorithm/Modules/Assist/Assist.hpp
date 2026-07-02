#ifndef ASSIST_HPP
#define ASSIST_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/glm/glm.hpp"

class Assist : public IManagedClass {
public:
    struct Settings {
        // Max pull toward target at center of assist zone (0 = none, 1 = full snap to target).
        float magnetStrength = 1.f;

        // Slows offset updates during tangential swipes so blue stays near target while orange moves on.
        float friction = 0.65f;

        // Inner radius (px) around target where extra sticky pull is applied.
        float stickyRadius = 25.f;

        // How strongly blue is pulled to target center inside stickyRadius.
        float stickyStrength = 0.7f;

        // Prediction time window (seconds): estimates where orange will be next frame.
        float lookahead = 0.1f;

        // Extra magnetism when flicking toward target or when predicted path gets closer.
        float flickBoost = 0.3f;

        // Minimum mouse speed (px/s) before flick detection and friction logic activate.
        float minSwipeSpeed = 80.f;

        // How fast the assist offset follows magnetism while inside the zone (higher = snappier).
        float smoothing = 14.f;

        // How fast blue re-syncs to orange after leaving the assist zone (higher = faster catch-up).
        float syncSpeed = 20.f;

        // How early offset is released when pulling mouse radially away from target (0-1).
        float releaseStrength = 0.85f;
    };

    bool init() override;
    void deinit() override;

    glm::vec2 process(const glm::vec2& mousePosition, const glm::vec2& targetCenter, float assistZoneRadius, float deltaTime);

    Settings& settings() { return settings_; }
    const Settings& settings() const { return settings_; }

private:
    static float computeMagnetPull(float distanceToTarget, float assistZoneRadius);
    static glm::vec2 normalizeOrZero(const glm::vec2& vector);
    void resetState();
    void saveFrameState(const glm::vec2& mousePosition, const glm::vec2& assistPosition);

    glm::vec2 computeDesiredOffsetInZone(const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float distanceToTarget, float assistZoneRadius) const;

    float computeOffsetBlendRate(bool isInsideAssistZone, const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float deltaSeconds) const;

    Settings settings_{};
    glm::vec2 previousMousePosition_{0.f};
    glm::vec2 previousAssistPosition_{0.f};
    bool hasFrameHistory_ = false;
};

#endif // ASSIST_HPP
