#ifndef ASSIST_HPP
#define ASSIST_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/glm/glm.hpp"

class Assist : public IManagedClass {
public:
    struct Settings {
        // Outer activation boundary. Assist cannot begin outside this radius.
        float fov = 200.f;

        // Assist area radius. Target is treated as a circle, not a center point.
        float circleSize = 69.f;

        // Global scale for distance-based correction strength (1 = full pull toward assist edge).
        float assistStrength = 1.5f;

        // Optional cap: max offset as a fraction of remaining distance to assist edge (0 = disabled).
        float maxOffsetFraction = 0.f;

        // Hard cap on assist offset length in pixels (0 = no cap). Does not scale strength.
        float maxOffsetPixels = 0.f;

        // How much prediction contributes on top of distance weighting.
        float predictionInfluence = 0.6f;

        // How much smooth natural movement reduces correction.
        float naturalMovementInfluence = 0.55f;

        // Slows offset updates near the assist radius edge (humanized entry braking).
        float radiusBrakeStrength = 0.65f;

        // How quickly assist ramps in after crossing the FOV boundary (mirrors exit decay feel).
        float fovEntryRamp = 3.f;

        // Max extra entry ramp from inward velocity (0 = ignore velocity, 0.35 = up to +35%).
        float fovEntryVelocityBoost = 0.35f;

        // Reference inward speed (px/s) for velocity-based entry ramp.
        float fovEntryVelocityReference = 300.f;

        // Prediction lookahead window in seconds.
        float lookahead = 0.1f;

        // Extra boost when flicking toward the assist area.
        float flickBoost = 0.25f;

        // Minimum cursor speed (px/s) required to update the offset. Below = offset freezes in place.
        float minMovementSpeed = 25.f;

        // Minimum speed before flick / swipe logic applies.
        float minSwipeSpeed = 80.f;

        // How quickly the persistent offset moves toward the desired offset.
        float offsetSmoothing = 6.f;

        // How quickly offset decays outside FOV or when assist is inactive.
        float offsetDecay = 5.f;

        // How quickly offset decays while inside the assist disk (no point pull).
        float insideOffsetDecay = 4.f;

        // How strongly radial movement away releases offset.
        float offsetRelease = 0.85f;

        // Slows offset updates during tangential swipes across the assist area.
        float friction = 0.45f;
    };

    bool init() override;
    void deinit() override;

    glm::vec2 process(const glm::vec2& mousePosition, const glm::vec2& targetCenter, float deltaTime);

    Settings& settings() { return settings_; }
    const Settings& settings() const { return settings_; }

private:
    struct AssistFrameContext {
        glm::vec2 mousePosition{};
        glm::vec2 targetCenter{};
        glm::vec2 mouseVelocity{};
        glm::vec2 previousAssistOffset{};
        float mouseSpeed = 0.f;
        float distanceToCenter = 0.f;
        float deltaSeconds = 0.f;
        float fovEntryBlend = 0.f;
        bool isInsideFov = false;
        bool isInsideAssistDisk = false;
    };

    static glm::vec2 normalizeOrZero(const glm::vec2& vector);
    static float smoothStep01(float value);
    bool hasActiveMovement(const AssistFrameContext& context) const;

    bool shouldApplyAssist(const AssistFrameContext& context) const;
    glm::vec2 computeAssistTargetOnRadius(const AssistFrameContext& context) const;
    float computeDistanceWeight(const AssistFrameContext& context) const;
    float computePredictionWeight(const AssistFrameContext& context) const;
    float computeNaturalMovementPreserve(const AssistFrameContext& context) const;
    float computeRadiusBrakeFactor(const AssistFrameContext& context) const;
    float computeOffsetUpdateRate(const AssistFrameContext& context, float correctionStrength) const;
    float computeOffsetReleaseAmount(const AssistFrameContext& context) const;

    glm::vec2 computeDesiredOffset(const AssistFrameContext& context) const;
    glm::vec2 decayOffset(const AssistFrameContext& context, float decayRate) const;
    glm::vec2 clampOffsetToMaxPixels(const glm::vec2& offset) const;
    glm::vec2 capOffsetByFractionOfDistance(const glm::vec2& offset, float correctionDistance) const;

    void resetState();
    void saveFrameState(const glm::vec2& mousePosition, const glm::vec2& assistPosition, const glm::vec2& mouseVelocity);

    Settings settings_{};
    glm::vec2 previousMousePosition_{0.f};
    glm::vec2 previousAssistPosition_{0.f};
    glm::vec2 previousMouseVelocity_{0.f};
    float fovEntryBlend_ = 0.f;
    bool hasFrameHistory_ = false;
};

#endif // ASSIST_HPP
