#include "Assist.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr float kMinDeltaSeconds = 1.f / 1000.f;
constexpr float kRadialReleaseThreshold = 0.1f;
constexpr float kTangentialSwipeThreshold = 0.3f;
constexpr float kNaturalSpeedReference = 220.f;
constexpr float kMinimumAssistThreshold = 0.001f;

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
    previousMouseVelocity_ = glm::vec2(0.f);
    fovEntryBlend_ = 0.f;
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
    if (length <= std::numeric_limits<float>::epsilon())
        return glm::vec2(0.f);

    return vector / length;
}

float Assist::smoothStep01(float value) {
    const float clampedValue = std::clamp(value, 0.f, 1.f);
    return clampedValue * clampedValue * (3.f - 2.f * clampedValue);
}

bool Assist::hasActiveMovement(const AssistFrameContext& context) const {
    return context.mouseSpeed >= settings_.minMovementSpeed;
}

bool Assist::shouldApplyAssist(const AssistFrameContext& context) const {
    if (!context.isInsideFov)
        return false;

    if (computeDistanceWeight(context) > kMinimumAssistThreshold)
        return true;

    return computePredictionWeight(context) > kMinimumAssistThreshold;
}

glm::vec2 Assist::computeAssistTargetOnRadius(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk)
        return context.mousePosition;

    if (context.distanceToCenter <= std::numeric_limits<float>::epsilon())
        return context.targetCenter + glm::vec2(settings_.circleSize, 0.f);

    const glm::vec2 directionFromCenter = normalizeOrZero(context.mousePosition - context.targetCenter);
    return context.targetCenter + directionFromCenter * settings_.circleSize;
}

float Assist::computeDistanceWeight(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk)
        return 0.f;

    if (settings_.fov <= settings_.circleSize)
        return 0.f;

    const float distanceBeyondAssistDisk = context.distanceToCenter - settings_.circleSize;
    const float activationRingWidth = settings_.fov - settings_.circleSize;
    const float depthIntoActivationRing = 1.f - std::clamp(distanceBeyondAssistDisk / activationRingWidth, 0.f, 1.f);

    return smoothStep01(depthIntoActivationRing);
}

float Assist::computePredictionWeight(const AssistFrameContext& context) const {
    if (context.mouseSpeed <= settings_.minSwipeSpeed)
        return 0.f;

    const glm::vec2 directionToAssistArea = computeAssistTargetOnRadius(context) - context.mousePosition;
    if (glm::length(directionToAssistArea) <= std::numeric_limits<float>::epsilon())
        return 0.f;

    const float movementTowardAssistArea = glm::dot(
        normalizeOrZero(context.mouseVelocity),
        normalizeOrZero(directionToAssistArea));

    if (movementTowardAssistArea <= 0.f)
        return 0.f;

    const float flickIntensity = std::clamp(
        movementTowardAssistArea * (context.mouseSpeed / settings_.minSwipeSpeed),
        0.f,
        1.f);

    float predictionWeight = flickIntensity * settings_.flickBoost;

    const glm::vec2 predictedMousePosition = context.mousePosition + context.mouseVelocity * settings_.lookahead;
    const float predictedDistanceToCenter = glm::distance(predictedMousePosition, context.targetCenter);

    if (predictedDistanceToCenter < context.distanceToCenter)
        predictionWeight = std::clamp(predictionWeight + settings_.flickBoost * 0.25f, 0.f, 1.f);

    return predictionWeight;
}

float Assist::computeNaturalMovementPreserve(const AssistFrameContext& context) const {
    if (context.mouseSpeed <= std::numeric_limits<float>::epsilon())
        return 0.f;

    const float previousSpeed = glm::length(previousMouseVelocity_);
    if (previousSpeed <= std::numeric_limits<float>::epsilon())
        return std::clamp(context.mouseSpeed / kNaturalSpeedReference, 0.f, 0.35f);

    const float velocityContinuity = glm::dot(
        normalizeOrZero(context.mouseVelocity),
        normalizeOrZero(previousMouseVelocity_));

    const float smoothMovementBlend = std::clamp((velocityContinuity + 1.f) * 0.5f, 0.f, 1.f);
    const float speedBlend = std::clamp(context.mouseSpeed / kNaturalSpeedReference, 0.f, 1.f);

    return smoothMovementBlend * speedBlend;
}

float Assist::computeRadiusBrakeFactor(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk)
        return settings_.radiusBrakeStrength;

    if (settings_.fov <= settings_.circleSize)
        return 1.f;

    const float distanceBeyondAssistDisk = context.distanceToCenter - settings_.circleSize;
    const float activationRingWidth = settings_.fov - settings_.circleSize;
    const float depthInActivationRing = std::clamp(distanceBeyondAssistDisk / activationRingWidth, 0.f, 1.f);

    return std::clamp(
        settings_.radiusBrakeStrength + depthInActivationRing * (1.f - settings_.radiusBrakeStrength),
        settings_.radiusBrakeStrength,
        1.f);
}

float Assist::computeOffsetReleaseAmount(const AssistFrameContext& context) const {
    if (context.mouseSpeed <= settings_.minSwipeSpeed)
        return 0.f;

    const glm::vec2 directionAwayFromCenter = context.mousePosition - context.targetCenter;
    if (glm::length(directionAwayFromCenter) <= std::numeric_limits<float>::epsilon())
        return 0.f;

    const glm::vec2 awayDirection = normalizeOrZero(directionAwayFromCenter);
    const float radialAwayMovement = std::max(
        glm::dot(normalizeOrZero(context.mouseVelocity), awayDirection),
        0.f);

    if (radialAwayMovement <= kRadialReleaseThreshold)
        return 0.f;

    const float distanceFromCenterRatio = std::clamp(context.distanceToCenter / std::max(settings_.fov, 1.f), 0.f, 1.f);
    return radialAwayMovement * distanceFromCenterRatio * settings_.offsetRelease;
}

float Assist::computeOffsetUpdateRate(const AssistFrameContext& context, float correctionStrength) const {
    if (correctionStrength <= kMinimumAssistThreshold)
        return std::clamp(settings_.offsetDecay * context.deltaSeconds, 0.f, 1.f);

    float updateRate = std::clamp(settings_.offsetSmoothing * context.deltaSeconds, 0.f, 1.f);
    updateRate *= computeRadiusBrakeFactor(context);
    updateRate *= context.fovEntryBlend;

    if (context.mouseSpeed > settings_.minSwipeSpeed) {
        const float speedBoost = std::clamp(context.mouseSpeed / settings_.minSwipeSpeed, 1.f, 3.f);
        updateRate = std::clamp(updateRate * speedBoost, 0.f, 1.f);
    }

    if (context.mouseSpeed <= settings_.minSwipeSpeed)
        return updateRate;

    const glm::vec2 directionAwayFromCenter = context.mousePosition - context.targetCenter;
    if (glm::length(directionAwayFromCenter) <= std::numeric_limits<float>::epsilon())
        return updateRate;

    const glm::vec2 awayDirection = normalizeOrZero(directionAwayFromCenter);
    const float radialAwayMovement = std::max(
        glm::dot(normalizeOrZero(context.mouseVelocity), awayDirection),
        0.f);

    if (radialAwayMovement >= kTangentialSwipeThreshold)
        return updateRate;

    const float tangentialSwipeAmount = 1.f - radialAwayMovement;
    updateRate *= 1.f - tangentialSwipeAmount * settings_.friction;

    return updateRate;
}

glm::vec2 Assist::decayOffset(const AssistFrameContext& context, float decayRate) const {
    const float clampedDecayRate = std::clamp(decayRate * context.deltaSeconds, 0.f, 1.f);
    return clampOffsetToMaxPixels(glm::mix(context.previousAssistOffset, glm::vec2(0.f), clampedDecayRate));
}

glm::vec2 Assist::clampOffsetToMaxPixels(const glm::vec2& offset) const {
    if (settings_.maxOffsetPixels <= 0.f)
        return offset;

    const float offsetLength = glm::length(offset);
    if (offsetLength <= settings_.maxOffsetPixels || offsetLength <= std::numeric_limits<float>::epsilon())
        return offset;

    return offset * (settings_.maxOffsetPixels / offsetLength);
}

glm::vec2 Assist::capOffsetByFractionOfDistance(const glm::vec2& offset, float correctionDistance) const {
    if (settings_.maxOffsetFraction <= 0.f || correctionDistance <= std::numeric_limits<float>::epsilon())
        return offset;

    const float fractionCap = correctionDistance * settings_.maxOffsetFraction;
    const float offsetLength = glm::length(offset);
    if (offsetLength <= fractionCap || offsetLength <= std::numeric_limits<float>::epsilon())
        return offset;

    return offset * (fractionCap / offsetLength);
}

glm::vec2 Assist::computeDesiredOffset(const AssistFrameContext& context) const {
    if (context.isInsideAssistDisk)
        return decayOffset(context, settings_.insideOffsetDecay);

    const glm::vec2 assistTargetOnRadius = computeAssistTargetOnRadius(context);
    const glm::vec2 correctionVector = assistTargetOnRadius - context.mousePosition;
    const float correctionDistance = glm::length(correctionVector);

    if (correctionDistance <= std::numeric_limits<float>::epsilon())
        return glm::vec2(0.f);

    const float distanceWeight = computeDistanceWeight(context);
    const float predictionWeight = computePredictionWeight(context);
    const float naturalMovementPreserve = computeNaturalMovementPreserve(context);

    float correctionStrength = distanceWeight * settings_.assistStrength + predictionWeight * settings_.predictionInfluence;
    const float naturalDampen = naturalMovementPreserve * settings_.naturalMovementInfluence;
    const float flickTowardAssistBlend = std::clamp(predictionWeight / std::max(settings_.flickBoost, kMinimumAssistThreshold), 0.f, 1.f);
    correctionStrength *= 1.f - naturalDampen * (1.f - flickTowardAssistBlend);
    correctionStrength *= context.fovEntryBlend;
    correctionStrength = std::max(correctionStrength, 0.f);

    if (correctionStrength <= kMinimumAssistThreshold)
        return decayOffset(context, settings_.insideOffsetDecay);

    glm::vec2 desiredOffset = normalizeOrZero(correctionVector) * correctionDistance * correctionStrength;
    desiredOffset = capOffsetByFractionOfDistance(desiredOffset, correctionDistance);

    const float releaseAmount = computeOffsetReleaseAmount(context);
    if (releaseAmount <= 0.f)
        return clampOffsetToMaxPixels(desiredOffset);

    return clampOffsetToMaxPixels(glm::mix(desiredOffset, glm::vec2(0.f), releaseAmount));
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
    const bool isInsideFov = distanceToCenter <= settings_.fov;

    if (!isInsideFov) {
        fovEntryBlend_ = 0.f;

        AssistFrameContext context{};
        context.mousePosition = mousePosition;
        context.previousAssistOffset = previousAssistPosition_ - previousMousePosition_;
        context.deltaSeconds = deltaSeconds;
        context.mouseSpeed = glm::length(mouseVelocity);

        const glm::vec2 offset = hasActiveMovement(context)
            ? decayOffset(context, settings_.offsetDecay)
            : clampOffsetToMaxPixels(context.previousAssistOffset);
        const glm::vec2 assistPosition = mousePosition + offset;

        saveFrameState(mousePosition, assistPosition, mouseVelocity);
        return assistPosition;
    }

    float inwardSpeed = 0.f;
    const glm::vec2 directionTowardCenter = targetCenter - mousePosition;
    if (glm::length(directionTowardCenter) > std::numeric_limits<float>::epsilon()) {
        inwardSpeed = std::max(
            glm::dot(mouseVelocity, normalizeOrZero(directionTowardCenter)),
            0.f);
    }

    const float velocityBoost = std::clamp(
        inwardSpeed / settings_.fovEntryVelocityReference,
        0.f,
        settings_.fovEntryVelocityBoost);

    const float entryRampRate = settings_.fovEntryRamp * (1.f + velocityBoost) * deltaSeconds;
    if (glm::length(mouseVelocity) >= settings_.minMovementSpeed)
        fovEntryBlend_ = std::clamp(fovEntryBlend_ + entryRampRate, 0.f, 1.f);

    AssistFrameContext context{};
    context.mousePosition = mousePosition;
    context.targetCenter = targetCenter;
    context.mouseVelocity = mouseVelocity;
    context.previousAssistOffset = previousAssistPosition_ - previousMousePosition_;
    context.mouseSpeed = glm::length(mouseVelocity);
    context.distanceToCenter = distanceToCenter;
    context.deltaSeconds = deltaSeconds;
    context.fovEntryBlend = fovEntryBlend_;
    context.isInsideFov = isInsideFov;
    context.isInsideAssistDisk = distanceToCenter <= settings_.circleSize;

    if (!hasActiveMovement(context)) {
        const glm::vec2 assistPosition = mousePosition + clampOffsetToMaxPixels(context.previousAssistOffset);

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
    const float correctionStrength = (computeDistanceWeight(context) + computePredictionWeight(context)) * fovEntryBlend_;
    const float offsetUpdateRate = computeOffsetUpdateRate(context, correctionStrength);
    const glm::vec2 blendedOffset = clampOffsetToMaxPixels(glm::mix(context.previousAssistOffset, desiredOffset, offsetUpdateRate));
    const glm::vec2 assistPosition = mousePosition + blendedOffset;

    saveFrameState(mousePosition, assistPosition, mouseVelocity);
    return assistPosition;
}
