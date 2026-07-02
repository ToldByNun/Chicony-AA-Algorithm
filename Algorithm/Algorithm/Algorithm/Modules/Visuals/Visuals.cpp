#include "Visuals.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../Assist/Assist.hpp"
#include "../../Engine/glm/glm.hpp"

void Visuals::draw(ImDrawList* drawList) {
    if (!drawList) return;

    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    const ImVec2 targetCenter(screenSize.x * 0.5f, screenSize.y * 0.5f);

    drawList->AddCircle(targetCenter, kAssistZoneRadius, IM_COL32(255, 0, 0, 255), 0, 2.f);
    drawRealCursor(drawList);
    drawAssistedCursor(drawList);
}

void Visuals::drawRealCursor(ImDrawList* drawList) {
    POINT screenCursorPoint{};

    if (GetCursorPos(&screenCursorPoint)) {
        realCursorPosition.x = static_cast<float>(screenCursorPoint.x);
        realCursorPosition.y = static_cast<float>(screenCursorPoint.y);
    }

    drawList->AddCircle(realCursorPosition, 20.0f, IM_COL32(255, 170, 85, 255), 0, 2.0f);
}

void Visuals::drawAssistedCursor(ImDrawList* drawList) {
    if (!globals.assist) return;

    const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    const ImVec2 targetCenter(screenSize.x * 0.5f, screenSize.y * 0.5f);

    const glm::vec2 mousePosition(realCursorPosition.x, realCursorPosition.y);
    const glm::vec2 targetCenterGlm(targetCenter.x, targetCenter.y);

    const glm::vec2 assistedPosition = globals.assist->process( mousePosition, targetCenterGlm, kAssistZoneRadius, ImGui::GetIO().DeltaTime);

    drawList->AddCircle(ImVec2(assistedPosition.x, assistedPosition.y), 20.0f, IM_COL32(0, 0, 255, 255), 0, 2.0f);
}
