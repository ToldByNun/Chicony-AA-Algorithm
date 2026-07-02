#pragma once

#include "../glm/glm.hpp"

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

struct SnapSample {
    glm::vec2 position{};
    float timeSeconds = 0.f;
};

struct DetectedSnap {
    float timeSeconds = 0.f;
    float angleDegrees = 0.f;
    float minLegDistancePixels = 0.f;
    glm::vec2 middlePosition{};
};

struct CircleGuardSnapSettings {
    // CircleGuard default: only flag snaps sharper than 10 degrees.
    float maxAngleDegrees = 10.f;

    // CircleGuard default: both movement legs must be longer than 8 osu! pixels.
    float minDistancePixels = 8.f;

    std::size_t maxLogEntries = 14;
};

class CircleGuardSnapping {
public:
    void reset();

    void setSettings(const CircleGuardSnapSettings& settings);
    const CircleGuardSnapSettings& settings() const { return settings_; }

    void addSample(const glm::vec2& position, float timeSeconds);

    bool hasValidTriple() const { return hasValidTriple_; }
    bool isSnapDetectedThisFrame() const { return snapDetectedThisFrame_; }
    float currentAngleDegrees() const { return currentAngleDegrees_; }
    float currentMinLegDistancePixels() const { return currentMinLegDistancePixels_; }
    std::size_t totalSnapCount() const { return snapHistory_.size(); }

    const std::vector<DetectedSnap>& snapHistory() const { return snapHistory_; }
    const std::vector<std::string>& logLines() const { return logLines_; }

private:
    static float computeDistance(const glm::vec2& from, const glm::vec2& to);
    static float computeAngleDegrees(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC);

    bool isSnapCandidate(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC, float& outAngleDegrees, float& outMinLegDistancePixels) const;

    void evaluateLatestTriple();
    void registerSnap(const DetectedSnap& snap);
    void rebuildLogLines();

    CircleGuardSnapSettings settings_{};
    std::deque<SnapSample> samples_;
    std::vector<DetectedSnap> snapHistory_;
    std::vector<std::string> logLines_;

    float currentAngleDegrees_ = 0.f;
    float currentMinLegDistancePixels_ = 0.f;
    bool hasValidTriple_ = false;
    bool snapDetectedThisFrame_ = false;
};
