#include "Visuals.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../Assist/Assist.hpp"
#include "../../Engine/glm/glm.hpp"

float getAssistFov() {
    if (!globals.assist) return 200.f;

    return globals.assist->settings().fov;
}

float getCircleSize() {
    if (!globals.assist) return 69.f;

    return globals.assist->settings().circleSize;
}

void Visuals::draw(ImDrawList* drawList) {
    if (!drawList) return;

    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    const ImVec2 targetCenter(screenSize.x * 0.5f, screenSize.y * 0.5f);

    drawRealCursor(drawList);

    const glm::vec2 mousePosition(realCursorPosition.x, realCursorPosition.y);
    updateSnapDetection(mousePosition, ImGui::GetIO().DeltaTime);

    drawList->AddCircle(targetCenter, getAssistFov(), kFovCircleColor, 0, kFovCircleThickness);
    drawList->AddCircle(targetCenter, getCircleSize(), kCircleSizeColor, 0, 1.5f);
    drawAssistedCursor(drawList);
    drawSnapDetectionLog(drawList);
}

void Visuals::drawRealCursor(ImDrawList* drawList) {
    POINT screenCursorPoint{};

    if (GetCursorPos(&screenCursorPoint)) {
        realCursorPosition.x = static_cast<float>(screenCursorPoint.x);
        realCursorPosition.y = static_cast<float>(screenCursorPoint.y);
    }

    drawList->AddCircle(realCursorPosition, kCursorDrawRadius, kRealCursorColor, 0, kFovCircleThickness);
}

void Visuals::drawAssistedCursor(ImDrawList* drawList) {
    if (!globals.assist) return;

    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    const ImVec2 targetCenter(screenSize.x * 0.5f, screenSize.y * 0.5f);

    const glm::vec2 mousePosition(realCursorPosition.x, realCursorPosition.y);
    const glm::vec2 targetCenterGlm(targetCenter.x, targetCenter.y);

    const glm::vec2 assistedPosition = globals.assist->process(mousePosition, targetCenterGlm, ImGui::GetIO().DeltaTime);

    drawList->AddCircle(ImVec2(assistedPosition.x, assistedPosition.y), kCursorDrawRadius, kAssistedCursorColor, 0, kFovCircleThickness);
}

void Visuals::updateSnapDetection(const glm::vec2& mousePosition, float deltaTime) {
    elapsedTimeSeconds_ += deltaTime;
    snapDetection_.addSample(mousePosition, elapsedTimeSeconds_);
}

void Visuals::drawSnapDetectionLog(ImDrawList* drawList) {
    const std::vector<std::string>& logLines = snapDetection_.logLines();
    if (logLines.empty()) return;

    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    const float padding = 10.f;
    const float panelWidth = 320.f;
    const float panelHeight = padding * 2.f + lineHeight * static_cast<float>(logLines.size());
    const ImVec2 panelTopLeft(screenSize.x - panelWidth - padding, padding);
    const ImVec2 panelBottomRight(panelTopLeft.x + panelWidth, panelTopLeft.y + panelHeight);

    drawList->AddRectFilled(panelTopLeft, panelBottomRight, kLogBackgroundColor, 6.f);
    drawList->AddRect(panelTopLeft, panelBottomRight, kLogBorderColor, 6.f);

    ImVec2 textPosition(panelTopLeft.x + padding, panelTopLeft.y + padding);
    for (const std::string& logLine : logLines) {
        const bool isSnapLine = logLine.starts_with("status: SNAP") || logLine.starts_with("[snap]");

        const ImU32 textColor = isSnapLine ? kLogSnapTextColor : kLogTextColor;
        drawList->AddText(textPosition, textColor, logLine.c_str());
        textPosition.y += lineHeight;
    }
}
