#include "Assist.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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
    previousMouseVelocity_ = glm::vec2(0.f);
    hasFrameHistory_ = false;
}

void Assist::saveFrameState(
    const glm::vec2& mousePosition,
    const glm::vec2& assistPosition,
    const glm::vec2& mouseVelocity)
{
    previousMousePosition_ = mousePosition;
    previousAssistPosition_ = assistPosition;
    previousMouseVelocity_ = mouseVelocity;
}

glm::vec2 Assist::normalizeOrZero(const glm::vec2& vector) {
    const float length = glm::length(vector);
    if (length <= std::numeric_limits<float>::epsilon()) return glm::vec2(0.f);

    return vector / length;
}

float Assist::computeCircleSuggestionWeight(float distanceToTarget, float fovRadius, float circleRadius) const {
    if (circleRadius <= std::numeric_limits<float>::epsilon()) return 0.f;

    const float distanceOnCircle = std::clamp(distanceToTarget / circleRadius, 0.f, 1.f);

    // float circleSuggestionWeight = std::sin(distanceOnCircle * kPi);

    // if (distanceToTarget <= circleRadius) return circleSuggestionWeight;

    if (fovRadius <= circleRadius) return 0.f;

    const float outsideDistance = distanceToTarget - circleRadius;
    const float outsideRange = fovRadius - circleRadius;
    const float outsideFalloff = 1.f - std::clamp(outsideDistance / outsideRange, 0.f, 1.f);

    // circleSuggestionWeight * ...
    return outsideFalloff * 0.45f;
}

float Assist::computePredictionWeight(const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float distanceToTarget) const {
    if (mouseSpeed <= settings_.minSwipeSpeed) return 0.f;

    const glm::vec2 directionToTarget = targetCenter - mousePosition;
    if (glm::length(directionToTarget) <= std::numeric_limits<float>::epsilon()) return 0.f;

    const float movementTowardTarget = glm::dot(normalizeOrZero(mouseVelocity), normalizeOrZero(directionToTarget));

    if (movementTowardTarget <= 0.f) return 0.f;

    const float flickIntensity = std::clamp(movementTowardTarget * (mouseSpeed / settings_.minSwipeSpeed), 0.f, 1.f);

    float predictionWeight = flickIntensity * settings_.flickBoost;

    const glm::vec2 predictedMousePosition = mousePosition + mouseVelocity * settings_.lookahead;
    const float predictedDistanceToTarget = glm::distance(predictedMousePosition, targetCenter);

    if (predictedDistanceToTarget < distanceToTarget) predictionWeight = std::clamp(predictionWeight + settings_.flickBoost * 0.25f, 0.f, 1.f);

    return predictionWeight;
}

float Assist::computeNaturalMovementPreserve(const glm::vec2& mouseVelocity, float mouseSpeed) const {
    if (mouseSpeed <= std::numeric_limits<float>::epsilon()) return 0.f;

    const float previousSpeed = glm::length(previousMouseVelocity_);
    if (previousSpeed <= std::numeric_limits<float>::epsilon()) return std::clamp(mouseSpeed / kNaturalSpeedReference, 0.f, 0.35f);

    const float velocityContinuity = glm::dot(normalizeOrZero(mouseVelocity), normalizeOrZero(previousMouseVelocity_));

    const float smoothMovementBlend = std::clamp((velocityContinuity + 1.f) * 0.5f, 0.f, 1.f);
    const float speedBlend = std::clamp(mouseSpeed / kNaturalSpeedReference, 0.f, 1.f);

    return smoothMovementBlend * speedBlend;
}

glm::vec2 Assist::computeDesiredOffsetInZone(const glm::vec2& mousePosition, const glm::vec2& targetCenter, const glm::vec2& mouseVelocity, float mouseSpeed, float distanceToTarget) const {
    const float fovRadius = settings_.fov;
    const float circleRadius = settings_.circleSize;

    const float circleSuggestionWeight = computeCircleSuggestionWeight(distanceToTarget, fovRadius, circleRadius);

    const float predictionWeight = computePredictionWeight(mousePosition, targetCenter, mouseVelocity, mouseSpeed, distanceToTarget);

    const float naturalMovementPreserve = computeNaturalMovementPreserve(mouseVelocity, mouseSpeed);

    float combinedPull = (circleSuggestionWeight * settings_.circleSuggestionInfluence + predictionWeight * settings_.predictionInfluence) * settings_.magnetStrength;

    combinedPull *= 1.f - naturalMovementPreserve * settings_.naturalMovementInfluence;
    combinedPull = std::clamp(combinedPull, 0.f, settings_.maxPull);

    if (combinedPull <= std::numeric_limits<float>::epsilon()) return glm::vec2(0.f);

    const glm::vec2 directionToTarget = normalizeOrZero(targetCenter - mousePosition);
    const float pullDistance = combinedPull * distanceToTarget;
    glm::vec2 desiredAssistOffset = directionToTarget * pullDistance;

    if (mouseSpeed <= settings_.minSwipeSpeed) return desiredAssistOffset;

    const glm::vec2 directionAwayFromTarget = mousePosition - targetCenter;
    if (glm::length(directionAwayFromTarget) <= std::numeric_limits<float>::epsilon()) return desiredAssistOffset;

    const glm::vec2 awayDirection = normalizeOrZero(directionAwayFromTarget);
    const float radialAwayMovement = std::max(glm::dot(normalizeOrZero(mouseVelocity), awayDirection), 0.f);

    if (radialAwayMovement <= kRadialReleaseThreshold) return desiredAssistOffset;

    const float distanceFromCenterRatio = std::clamp(distanceToTarget / std::max(fovRadius, 1.f), 0.f, 1.f);
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

glm::vec2 Assist::process(const glm::vec2& mousePosition, const glm::vec2& targetCenter, float deltaTime) {
    const float deltaSeconds = std::max(deltaTime, kMinDeltaSeconds);
    const float fovRadius = settings_.fov;

    if (!hasFrameHistory_) {
        saveFrameState(mousePosition, mousePosition, glm::vec2(0.f));
        hasFrameHistory_ = true;
        return mousePosition;
    }

    const glm::vec2 mouseVelocity = (mousePosition - previousMousePosition_) / deltaSeconds;
    const float mouseSpeed = glm::length(mouseVelocity);
    const float distanceToTarget = glm::distance(mousePosition, targetCenter);
    const bool isInsideAssistZone = distanceToTarget <= fovRadius;
    const glm::vec2 previousAssistOffset = previousAssistPosition_ - previousMousePosition_;

    if (!isInsideAssistZone) {
        const float syncBlendRate = std::clamp(settings_.syncSpeed * deltaSeconds, 0.f, 1.f);
        const glm::vec2 syncedAssistOffset = glm::mix(previousAssistOffset, glm::vec2(0.f), syncBlendRate);
        const glm::vec2 assistPosition = mousePosition + syncedAssistOffset;

        saveFrameState(mousePosition, assistPosition, mouseVelocity);
        return assistPosition;
    }

    const glm::vec2 desiredAssistOffset = computeDesiredOffsetInZone(mousePosition, targetCenter, mouseVelocity, mouseSpeed, distanceToTarget);

    const float offsetBlendRate = computeOffsetBlendRate(isInsideAssistZone, mousePosition, targetCenter, mouseVelocity, mouseSpeed, deltaSeconds);

    const glm::vec2 blendedAssistOffset = glm::mix(previousAssistOffset, desiredAssistOffset, offsetBlendRate);
    const glm::vec2 assistPosition = mousePosition + blendedAssistOffset;

    saveFrameState(mousePosition, assistPosition, mouseVelocity);
    return assistPosition;
}
