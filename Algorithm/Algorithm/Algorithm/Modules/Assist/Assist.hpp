#ifndef ASSIST_HPP
#define ASSIST_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/glm/glm.hpp"

class Assist : public IManagedClass {
public:
    struct Settings {
        // Hard outer assist boundary in pixels. Outside = 1:1 mouse pass-through.
        float fov = 200.f;

        // Suggested target circle radius in pixels. Shapes soft edge weights only, not a hard clip.
        float circleSize = 69.f;

        // Global scale for assist pull strength.
        float magnetStrength = 2.45f;

        // Upper cap for how much of the remaining distance can be pulled per frame (prevents hard center snap).
        float maxPull = 0.35f;

        // How much the soft circle-size curve contributes to pull.
        float circleSuggestionInfluence = 0.4f;

        // How much prediction / flick intent contributes to pull.
        float predictionInfluence = 0.65f;

        // How much smooth natural mouse movement reduces assist (higher = more human-like drift).
        float naturalMovementInfluence = 0.55f;

        // Slows offset updates during tangential swipes so blue can linger near target briefly.
        float friction = 0.45f;

        // Prediction time window in seconds.
        float lookahead = 0.1f;

        // Extra prediction boost when moving toward target.
        float flickBoost = 0.22f;

        // Minimum mouse speed (px/s) before swipe / flick logic activates.
        float minSwipeSpeed = 80.f;

        // How fast assist offset follows desired offset inside the zone (lower = softer).
        float smoothing = 8.f;

        // How fast blue re-syncs to orange after leaving the assist zone.
        float syncSpeed = 5.f;

        // How early offset is released when pulling radially away from target (0-1).
        float releaseStrength = 0.85f;
    };

    bool init() override;
    void deinit() override;

    glm::vec2 process(const glm::vec2& mousePosition, const glm::vec2& targetCenter, float deltaTime);

    Settings& settings() { return settings_; }
    const Settings& settings() const { return settings_; }

private:
    static glm::vec2 normalizeOrZero(const glm::vec2& vector);

    float computeCircleSuggestionWeight(float distanceToTarget, float fovRadius, float circleRadius) const;
    float computePredictionWeight(const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float distanceToTarget) const;

    float computeNaturalMovementPreserve(const glm::vec2& mouseVelocity, float mouseSpeed) const;

    void resetState();
    void saveFrameState(const glm::vec2& mousePosition, const glm::vec2& assistPosition, const glm::vec2& mouseVelocity);

    glm::vec2 computeDesiredOffsetInZone(const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float distanceToTarget) const;

    float computeOffsetBlendRate(bool isInsideAssistZone, const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float deltaSeconds) const;

    Settings settings_{};
    glm::vec2 previousMousePosition_{0.f};
    glm::vec2 previousAssistPosition_{0.f};
    glm::vec2 previousMouseVelocity_{0.f};
    bool hasFrameHistory_ = false;

private:
    float kMinDeltaSeconds = 1.f / 1000.f;
    float kRadialReleaseThreshold = 0.1f;
    float kTangentialSwipeThreshold = 0.3f;
    float kNaturalSpeedReference = 220.f;
    float kPi = 3.14159265358979323846f;
};

#endif // ASSIST_HPP
