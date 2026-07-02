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

void Assist::saveFrameState(const glm::vec2& mousePosition, const glm::vec2& assistPosition, const glm::vec2& mouseVelocity){
    previousMousePosition_ = mousePosition;
    previousAssistPosition_ = assistPosition;
    previousMouseVelocity_ = mouseVelocity;
}

glm::vec2 Assist::normalizeOrZero(const glm::vec2& vector) {
    const float length = glm::length(vector);
    if (length <= std::numeric_limits<float>::epsilon()) return glm::vec2(0.f);

    return vector / length;
}

float Assist::smoothStep01(float value) {
    const float clampedValue = std::clamp(value, 0.f, 1.f);
    return clampedValue * clampedValue * (3.f - 2.f * clampedValue);
}

bool Assist::shouldApplyAssist(const AssistFrameContext& context) const {
    if (!context.isInsideFov) return false;

    if (glm::length(context.previousAssistOffset) > kMinimumAssistThreshold) return true;

    if (computeDistanceWeight(context) > kMinimumAssistThreshold) return true;

    return computePredictionWeight(context) > kMinimumAssistThreshold;
}

glm::vec2 Assist::computeAssistTargetOnRadius(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk) return context.mousePosition;

    if (context.distanceToCenter <= std::numeric_limits<float>::epsilon()) return context.targetCenter + glm::vec2(settings_.circleSize, 0.f);

    const glm::vec2 directionFromCenter = normalizeOrZero(context.mousePosition - context.targetCenter);
    return context.targetCenter + directionFromCenter * settings_.circleSize;
}

float Assist::computeDistanceWeight(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk) return 0.f;

    if (settings_.fov <= settings_.circleSize) return 0.f;

    const float distanceBeyondAssistDisk = context.distanceToCenter - settings_.circleSize;
    const float activationRingWidth = settings_.fov - settings_.circleSize;
    const float normalizedDistance = std::clamp(distanceBeyondAssistDisk / activationRingWidth, 0.f, 1.f);

    return smoothStep01(normalizedDistance);
}

float Assist::computePredictionWeight(const AssistFrameContext& context) const {
    if (context.mouseSpeed <= settings_.minSwipeSpeed) return 0.f;

    const glm::vec2 directionToAssistArea = computeAssistTargetOnRadius(context) - context.mousePosition;
    if (glm::length(directionToAssistArea) <= std::numeric_limits<float>::epsilon()) return 0.f;

    const float movementTowardAssistArea = glm::dot(normalizeOrZero(context.mouseVelocity), normalizeOrZero(directionToAssistArea));

    if (movementTowardAssistArea <= 0.f) return 0.f;

    const float flickIntensity = std::clamp(movementTowardAssistArea * (context.mouseSpeed / settings_.minSwipeSpeed), 0.f, 1.f);

    float predictionWeight = flickIntensity * settings_.flickBoost;

    const glm::vec2 predictedMousePosition = context.mousePosition + context.mouseVelocity * settings_.lookahead;
    const float predictedDistanceToCenter = glm::distance(predictedMousePosition, context.targetCenter);
    const float currentDistanceToCenter = context.distanceToCenter;

    if (predictedDistanceToCenter < currentDistanceToCenter) predictionWeight = std::clamp(predictionWeight + settings_.flickBoost * 0.25f, 0.f, 1.f);

    return predictionWeight;
}

float Assist::computeNaturalMovementPreserve(const AssistFrameContext& context) const {
    if (context.mouseSpeed <= std::numeric_limits<float>::epsilon()) return 0.f;

    const float previousSpeed = glm::length(previousMouseVelocity_);
    if (previousSpeed <= std::numeric_limits<float>::epsilon()) return std::clamp(context.mouseSpeed / kNaturalSpeedReference, 0.f, 0.35f);

    const float velocityContinuity = glm::dot(normalizeOrZero(context.mouseVelocity), normalizeOrZero(previousMouseVelocity_));

    const float smoothMovementBlend = std::clamp((velocityContinuity + 1.f) * 0.5f, 0.f, 1.f);
    const float speedBlend = std::clamp(context.mouseSpeed / kNaturalSpeedReference, 0.f, 1.f);

    return smoothMovementBlend * speedBlend;
}

float Assist::computeRadiusBrakeFactor(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk) return settings_.radiusBrakeStrength;

    if (settings_.fov <= settings_.circleSize) return 1.f;

    const float distanceBeyondAssistDisk = context.distanceToCenter - settings_.circleSize;
    const float activationRingWidth = settings_.fov - settings_.circleSize;
    const float depthInActivationRing = std::clamp(distanceBeyondAssistDisk / activationRingWidth, 0.f, 1.f);

    return std::clamp(settings_.radiusBrakeStrength + depthInActivationRing * (1.f - settings_.radiusBrakeStrength), settings_.radiusBrakeStrength, 1.f);
}

float Assist::computeOffsetReleaseAmount(const AssistFrameContext& context) const {
    if (context.mouseSpeed <= settings_.minSwipeSpeed) return 0.f;

    const glm::vec2 directionAwayFromCenter = context.mousePosition - context.targetCenter;
    if (glm::length(directionAwayFromCenter) <= std::numeric_limits<float>::epsilon()) return 0.f;

    const glm::vec2 awayDirection = normalizeOrZero(directionAwayFromCenter);
    const float radialAwayMovement = std::max(glm::dot(normalizeOrZero(context.mouseVelocity), awayDirection), 0.f);

    if (radialAwayMovement <= kRadialReleaseThreshold) return 0.f;

    const float distanceFromCenterRatio = std::clamp(context.distanceToCenter / std::max(settings_.fov, 1.f), 0.f, 1.f);
    return radialAwayMovement * distanceFromCenterRatio * settings_.offsetRelease;
}

float Assist::computeOffsetUpdateRate(const AssistFrameContext& context, float correctionStrength) const {
    if (correctionStrength <= kMinimumAssistThreshold) return std::clamp(settings_.offsetDecay * context.deltaSeconds, 0.f, 1.f);

    float updateRate = std::clamp(settings_.offsetSmoothing * context.deltaSeconds, 0.f, 1.f);
    updateRate *= computeRadiusBrakeFactor(context);

    if (context.mouseSpeed <= settings_.minSwipeSpeed) return updateRate;

    const glm::vec2 directionAwayFromCenter = context.mousePosition - context.targetCenter;
    if (glm::length(directionAwayFromCenter) <= std::numeric_limits<float>::epsilon()) return updateRate;

    const glm::vec2 awayDirection = normalizeOrZero(directionAwayFromCenter);
    const float radialAwayMovement = std::max(glm::dot(normalizeOrZero(context.mouseVelocity), awayDirection), 0.f);

    if (radialAwayMovement >= kTangentialSwipeThreshold) return updateRate;

    const float tangentialSwipeAmount = 1.f - radialAwayMovement;
    updateRate *= 1.f - tangentialSwipeAmount * settings_.friction;

    return updateRate;
}

glm::vec2 Assist::decayOffset(const AssistFrameContext& context, float decayRate) const {
    const float clampedDecayRate = std::clamp(decayRate * context.deltaSeconds, 0.f, 1.f);
    return glm::mix(context.previousAssistOffset, glm::vec2(0.f), clampedDecayRate);
}

glm::vec2 Assist::computeDesiredOffset(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk) return decayOffset(context, settings_.insideOffsetDecay);

    const glm::vec2 assistTargetOnRadius = computeAssistTargetOnRadius(context);
    const glm::vec2 correctionVector = assistTargetOnRadius - context.mousePosition;
    const float correctionDistance = glm::length(correctionVector);

    if (correctionDistance <= std::numeric_limits<float>::epsilon()) return glm::vec2(0.f);

    const float distanceWeight = computeDistanceWeight(context);
    const float predictionWeight = computePredictionWeight(context);
    const float naturalMovementPreserve = computeNaturalMovementPreserve(context);

    float correctionStrength = distanceWeight * settings_.assistStrength + predictionWeight * settings_.predictionInfluence;
    correctionStrength *= 1.f - naturalMovementPreserve * settings_.naturalMovementInfluence;
    correctionStrength = std::clamp(correctionStrength, 0.f, settings_.maxOffsetFraction);

    if (correctionStrength <= kMinimumAssistThreshold) return decayOffset(context, settings_.insideOffsetDecay);

    glm::vec2 desiredOffset = normalizeOrZero(correctionVector) * correctionDistance * correctionStrength;

    const float releaseAmount = computeOffsetReleaseAmount(context);
    if (releaseAmount <= 0.f) return desiredOffset;

    return glm::mix(desiredOffset, glm::vec2(0.f), releaseAmount);
}

glm::vec2 Assist::process(const glm::vec2& mousePosition, const glm::vec2& targetCenter, float deltaTime) {
    const float deltaSeconds = std::max(deltaTime, kMinDeltaSeconds);

    if (!hasFrameHistory_) {
        saveFrameState(mousePosition, mousePosition, glm::vec2(0.f));
        hasFrameHistory_ = true;
        return mousePosition;
    }

    const glm::vec2 mouseVelocity = (mousePosition - previousMousePosition_) / deltaSeconds;
    const float distanceToCenter = glm::distance(mousePosition, targetCenter);

    AssistFrameContext context{};
    context.mousePosition = mousePosition;
    context.targetCenter = targetCenter;
    context.mouseVelocity = mouseVelocity;
    context.previousAssistOffset = previousAssistPosition_ - previousMousePosition_;
    context.mouseSpeed = glm::length(mouseVelocity);
    context.distanceToCenter = distanceToCenter;
    context.deltaSeconds = deltaSeconds;
    context.isInsideFov = distanceToCenter <= settings_.fov;
    context.isInsideAssistDisk = distanceToCenter <= settings_.circleSize;

    if (!context.isInsideFov) {
        const glm::vec2 decayedOffset = decayOffset(context, settings_.offsetDecay);
        const glm::vec2 assistPosition = mousePosition + decayedOffset;

        saveFrameState(mousePosition, assistPosition, mouseVelocity);
        return assistPosition;
    }

    if (!shouldApplyAssist(context)) {
        const glm::vec2 decayedOffset = decayOffset(context, settings_.offsetDecay);
        const glm::vec2 assistPosition = mousePosition + decayedOffset;

        saveFrameState(mousePosition, assistPosition, mouseVelocity);
        return assistPosition;
    }

    const glm::vec2 desiredOffset = computeDesiredOffset(context);
    const float correctionStrength = computeDistanceWeight(context) + computePredictionWeight(context);
    const float offsetUpdateRate = computeOffsetUpdateRate(context, correctionStrength);
    const glm::vec2 blendedOffset = glm::mix(context.previousAssistOffset, desiredOffset, offsetUpdateRate);
    const glm::vec2 assistPosition = mousePosition + blendedOffset;

    saveFrameState(mousePosition, assistPosition, mouseVelocity);
    return assistPosition;
}
