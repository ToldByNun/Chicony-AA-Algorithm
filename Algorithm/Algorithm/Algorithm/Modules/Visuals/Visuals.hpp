#ifndef VISUALS_HPP
#define VISUALS_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/CircleGuardSnapping/CircleGuardSnapping.hpp"
#include "../../Engine/ImGui/imgui.h"

#include <Windows.h>

struct ImDrawList;

class Visuals : public IManagedClass {
public:
    void draw(ImDrawList* drawList);
    void drawRealCursor(ImDrawList* drawList);
    void drawAssistedCursor(ImDrawList* drawList);
    void drawSnapDetectionLog(ImDrawList* drawList);

private:
    void updateSnapDetection(const glm::vec2& mousePosition, float deltaTime);

    CircleGuardSnapping snapDetection_{};
    ImVec2 realCursorPosition{};
    float elapsedTimeSeconds_ = 0.f;

private:
    float kCursorDrawRadius = 20.f;
    float kFovCircleThickness = 2.f;
    ImU32 kFovCircleColor = IM_COL32(255, 0, 0, 255);
    ImU32 kCircleSizeColor = IM_COL32(255, 120, 120, 170);
    ImU32 kRealCursorColor = IM_COL32(255, 170, 85, 255);
    ImU32 kAssistedCursorColor = IM_COL32(0, 0, 255, 255);
    ImU32 kLogBackgroundColor = IM_COL32(12, 12, 12, 210);
    ImU32 kLogBorderColor = IM_COL32(255, 255, 255, 80);
    ImU32 kLogTextColor = IM_COL32(230, 230, 230, 255);
    ImU32 kLogSnapTextColor = IM_COL32(255, 90, 90, 255);

};

#endif // VISUALS_HPP
