#include "Assist.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

    constexpr float kMinDeltaSeconds = 1.f / 1000.f;
    constexpr float kRadialReleaseThreshold = 0.1f;
    constexpr float kTangentialSwipeThreshold = 0.3f;

}

bool Assist::init() {
    resetState();
    return true;
}

void Assist::deinit() {
    resetState();
}

void Assist::resetState() {
    previousMousePosition_ = glm::vec2(0.f);
    previousAssistPosition_ = glm::vec2(0.f);
    hasFrameHistory_ = false;
}

void Assist::saveFrameState(const glm::vec2& mousePosition, const glm::vec2& assistPosition) {
    previousMousePosition_ = mousePosition;
    previousAssistPosition_ = assistPosition;
}

float Assist::computeMagnetPull(float distanceToTarget, float assistZoneRadius) {
    if (assistZoneRadius <= 0.f) return 0.f;

    const float normalizedDistance = std::clamp(1.f - distanceToTarget / assistZoneRadius, 0.f, 1.f);
    return normalizedDistance * normalizedDistance * (3.f - 2.f * normalizedDistance);
}

glm::vec2 Assist::normalizeOrZero(const glm::vec2& vector) {
    const float length = glm::length(vector);
    if (length <= std::numeric_limits<float>::epsilon()) return glm::vec2(0.f);

    return vector / length;
}

glm::vec2 Assist::computeDesiredOffsetInZone(const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float distanceToTarget, float assistZoneRadius) const {
    const glm::vec2 predictedMousePosition = mousePosition + mouseVelocity * settings_.lookahead;

    float magnetPull = computeMagnetPull(distanceToTarget, assistZoneRadius) * settings_.magnetStrength;

    const glm::vec2 directionToTarget = targetCenter - mousePosition;
    const bool hasMeaningfulDirection = glm::length(directionToTarget) > std::numeric_limits<float>::epsilon();
    const bool isFastEnoughForSwipeLogic = mouseSpeed > settings_.minSwipeSpeed;

    if (isFastEnoughForSwipeLogic && hasMeaningfulDirection) {
        const float movementTowardTarget = glm::dot(normalizeOrZero(mouseVelocity), normalizeOrZero(directionToTarget));

        if (movementTowardTarget > 0.f) {
            const float flickIntensity = std::clamp(
                movementTowardTarget * (mouseSpeed / settings_.minSwipeSpeed),
                0.f,
                1.f
            );

            magnetPull = std::clamp(magnetPull + flickIntensity * settings_.flickBoost, 0.f, 1.f);
        }
    }

    const float predictedDistanceToTarget = glm::distance(predictedMousePosition, targetCenter);
    if (predictedDistanceToTarget < distanceToTarget) magnetPull = std::clamp(magnetPull + settings_.flickBoost * 0.5f, 0.f, 1.f);

    glm::vec2 magnetPosition = glm::mix(mousePosition, targetCenter, magnetPull);

    if (distanceToTarget < settings_.stickyRadius) {
        const float stickyBlend = 1.f - distanceToTarget / settings_.stickyRadius;
        magnetPosition = glm::mix(magnetPosition, targetCenter, stickyBlend * settings_.stickyStrength);
    }

    glm::vec2 desiredAssistOffset = magnetPosition - mousePosition;

    if (!isFastEnoughForSwipeLogic) return desiredAssistOffset;

    const glm::vec2 directionAwayFromTarget = mousePosition - targetCenter;
    if (glm::length(directionAwayFromTarget) <= std::numeric_limits<float>::epsilon()) return desiredAssistOffset;

    const glm::vec2 awayDirection = normalizeOrZero(directionAwayFromTarget);
    const float radialAwayMovement = std::max(glm::dot(normalizeOrZero(mouseVelocity), awayDirection), 0.f);

    if (radialAwayMovement <= kRadialReleaseThreshold) return desiredAssistOffset;

    const float distanceFromCenterRatio = std::clamp(distanceToTarget / assistZoneRadius, 0.f, 1.f);
    const float releaseAmount = radialAwayMovement * distanceFromCenterRatio * settings_.releaseStrength;

    return glm::mix(desiredAssistOffset, glm::vec2(0.f), releaseAmount);
}

float Assist::computeOffsetBlendRate(bool isInsideAssistZone, const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float deltaSeconds) const {
    if (!isInsideAssistZone) return std::clamp(settings_.syncSpeed * deltaSeconds, 0.f, 1.f);

    float offsetBlendRate = std::clamp(settings_.smoothing * deltaSeconds, 0.f, 1.f);

    if (mouseSpeed <= settings_.minSwipeSpeed) return offsetBlendRate;

    const glm::vec2 directionAwayFromTarget = mousePosition - targetCenter;
    if (glm::length(directionAwayFromTarget) <= std::numeric_limits<float>::epsilon()) return offsetBlendRate;

    const glm::vec2 awayDirection = normalizeOrZero(directionAwayFromTarget);
    const float radialAwayMovement = std::max(glm::dot(normalizeOrZero(mouseVelocity), awayDirection), 0.f);

    if (radialAwayMovement >= kTangentialSwipeThreshold) return offsetBlendRate;

    const float tangentialSwipeAmount = 1.f - radialAwayMovement;
    offsetBlendRate *= 1.f - tangentialSwipeAmount * settings_.friction;

    return offsetBlendRate;
}

glm::vec2 Assist::process(const glm::vec2& mousePosition, const glm::vec2& targetCenter, float assistZoneRadius, float deltaTime) {
    const float deltaSeconds = std::max(deltaTime, kMinDeltaSeconds);

    if (!hasFrameHistory_) {
        saveFrameState(mousePosition, mousePosition);
        hasFrameHistory_ = true;
        return mousePosition;
    }

    const glm::vec2 mouseVelocity = (mousePosition - previousMousePosition_) / deltaSeconds;
    const float mouseSpeed = glm::length(mouseVelocity);
    const float distanceToTarget = glm::distance(mousePosition, targetCenter);
    const bool isInsideAssistZone = distanceToTarget <= assistZoneRadius;
    const glm::vec2 previousAssistOffset = previousAssistPosition_ - previousMousePosition_;

    if (!isInsideAssistZone) {
        const float syncBlendRate = std::clamp(settings_.syncSpeed * deltaSeconds, 0.f, 1.f);
        const glm::vec2 syncedAssistOffset = glm::mix(previousAssistOffset, glm::vec2(0.f), syncBlendRate);
        const glm::vec2 assistPosition = mousePosition + syncedAssistOffset;

        saveFrameState(mousePosition, assistPosition);
        return assistPosition;
    }

    const glm::vec2 desiredAssistOffset = computeDesiredOffsetInZone( mousePosition, targetCenter, mouseVelocity, mouseSpeed, distanceToTarget, assistZoneRadius);

    const float offsetBlendRate = computeOffsetBlendRate(isInsideAssistZone, mousePosition, targetCenter, mouseVelocity, mouseSpeed, deltaSeconds);

    const glm::vec2 blendedAssistOffset = glm::mix(previousAssistOffset, desiredAssistOffset, offsetBlendRate);
    const glm::vec2 assistPosition = mousePosition + blendedAssistOffset;

    saveFrameState(mousePosition, assistPosition);
    return assistPosition;
}
