#include "CircleGuardSnapping.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

namespace {

    constexpr float kDuplicateSampleEpsilon = 0.01f;

    bool isDuplicateSample(const SnapSample& previousSample, const glm::vec2& position, float timeSeconds) {
        if (previousSample.timeSeconds != timeSeconds) return false;

        return glm::distance(previousSample.position, position) <= kDuplicateSampleEpsilon;
    }

}

void CircleGuardSnapping::reset() {
    samples_.clear();
    snapHistory_.clear();
    logLines_.clear();

    currentAngleDegrees_ = 0.f;
    currentMinLegDistancePixels_ = 0.f;
    hasValidTriple_ = false;
    snapDetectedThisFrame_ = false;

    rebuildLogLines();
}

void CircleGuardSnapping::setSettings(const CircleGuardSnapSettings& settings) {
    settings_ = settings;
    rebuildLogLines();
}

void CircleGuardSnapping::addSample(const glm::vec2& position, float timeSeconds) {
    snapDetectedThisFrame_ = false;
    hasValidTriple_ = false;

    if (!samples_.empty() && isDuplicateSample(samples_.back(), position, timeSeconds)) return;

    samples_.push_back({ position, timeSeconds });

    constexpr std::size_t kRequiredSampleCount = 3;
    while (samples_.size() > kRequiredSampleCount) samples_.pop_front();

    if (samples_.size() < kRequiredSampleCount) {
        rebuildLogLines();
        return;
    }

    evaluateLatestTriple();
    rebuildLogLines();
}

float CircleGuardSnapping::computeDistance(const glm::vec2& from, const glm::vec2& to) {
    return glm::length(to - from);
}

float CircleGuardSnapping::computeAngleDegrees(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC) {
    const float distanceAB = computeDistance(pointA, pointB);
    const float distanceBC = computeDistance(pointB, pointC);
    const float distanceAC = computeDistance(pointA, pointC);

    const float denominator = 2.f * distanceAB * distanceBC;
    if (denominator <= std::numeric_limits<float>::epsilon()) return std::numeric_limits<float>::quiet_NaN();

    float cosineBeta = -(distanceAC * distanceAC - distanceAB * distanceAB - distanceBC * distanceBC) / denominator;
    cosineBeta = std::clamp(cosineBeta, -1.f, 1.f);

    return glm::degrees(std::acos(cosineBeta));
}

bool CircleGuardSnapping::isSnapCandidate(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC, float& outAngleDegrees, float& outMinLegDistancePixels) const {
    const float distanceAB = computeDistance(pointA, pointB);
    const float distanceBC = computeDistance(pointB, pointC);
    outMinLegDistancePixels = std::min(distanceAB, distanceBC);

    if (outMinLegDistancePixels <= settings_.minDistancePixels) return false;

    outAngleDegrees = computeAngleDegrees(pointA, pointB, pointC);
    if (std::isnan(outAngleDegrees)) return false;

    return outAngleDegrees < settings_.maxAngleDegrees;
}

void CircleGuardSnapping::evaluateLatestTriple() {
    const glm::vec2 pointA = samples_[0].position;
    const glm::vec2 pointB = samples_[1].position;
    const glm::vec2 pointC = samples_[2].position;

    hasValidTriple_ = true;
    currentAngleDegrees_ = computeAngleDegrees(pointA, pointB, pointC);
    currentMinLegDistancePixels_ = std::min(computeDistance(pointA, pointB), computeDistance(pointB, pointC));

    float snapAngleDegrees = 0.f;
    float snapMinLegDistancePixels = 0.f;
    if (!isSnapCandidate(pointA, pointB, pointC, snapAngleDegrees, snapMinLegDistancePixels)) return;

    snapDetectedThisFrame_ = true;

    DetectedSnap detectedSnap{};
    detectedSnap.timeSeconds = samples_[1].timeSeconds;
    detectedSnap.angleDegrees = snapAngleDegrees;
    detectedSnap.minLegDistancePixels = snapMinLegDistancePixels;
    detectedSnap.middlePosition = pointB;

    registerSnap(detectedSnap);
}

void CircleGuardSnapping::registerSnap(const DetectedSnap& snap) {
    snapHistory_.push_back(snap);
}

void CircleGuardSnapping::rebuildLogLines() {
    logLines_.clear();
    logLines_.push_back("CircleGuard Snapping");

    if (!hasValidTriple_) {
        logLines_.push_back("waiting for 3 samples...");
        return;
    }

    logLines_.push_back(std::format("live angle: {:.2f} deg", currentAngleDegrees_));

    logLines_.push_back(std::format("live min leg: {:.2f} px", currentMinLegDistancePixels_));

    logLines_.push_back(std::format("thresholds: < {:.1f} deg, > {:.1f} px", settings_.maxAngleDegrees, settings_.minDistancePixels));

    logLines_.push_back(snapDetectedThisFrame_ ? "status: SNAP" : "status: normal");

    if (snapHistory_.empty()) {
        logLines_.push_back("events: none");
        return;
    }

    logLines_.push_back(std::format("events: {}", snapHistory_.size()));

    const std::size_t firstVisibleSnapIndex = snapHistory_.size() > settings_.maxLogEntries ? snapHistory_.size() - settings_.maxLogEntries : 0;

    for (std::size_t snapIndex = firstVisibleSnapIndex; snapIndex < snapHistory_.size(); ++snapIndex) {
        const DetectedSnap& snap = snapHistory_[snapIndex];
        logLines_.push_back(std::format("[snap] t={:.2f}s angle={:.2f} dist={:.2f}", snap.timeSeconds, snap.angleDegrees, snap.minLegDistancePixels));
    }
}
