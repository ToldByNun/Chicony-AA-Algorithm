#include "Visuals.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../Assist/Assist.hpp"
#include "../../Engine/glm/glm.hpp"

void Visuals::draw(ImDrawList* drawList) {
    if (!drawList) return;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 center(display.x * 0.5f, display.y * 0.5f);

    drawList->AddCircle(center, kAssistFov, IM_COL32(255, 0, 0, 255), 0, 2.f);
    getAndDrawCursor(drawList);
    drawAssistedCursor(drawList);
}

void Visuals::getAndDrawCursor(ImDrawList* drawList) {
    POINT point{};

    if (GetCursorPos(&point)) {
        cursorPosition.x = static_cast<float>(point.x);
        cursorPosition.y = static_cast<float>(point.y);
    }

    drawList->AddCircle(cursorPosition, 20.0f, IM_COL32(255, 170, 85, 255), 0, 2.0f);
}

void Visuals::drawAssistedCursor(ImDrawList* drawList) {
    if (!globals.assist) return;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 center(display.x * 0.5f, display.y * 0.5f);

    const glm::vec2 cursorPos(cursorPosition.x, cursorPosition.y);
    const glm::vec2 targetPos(center.x, center.y);

    const glm::vec2 assisted = globals.assist->algorithm(
        cursorPos,
        targetPos,
        kAssistFov,
        kAssistStrength
    );

    drawList->AddCircle(ImVec2(assisted.x, assisted.y), 20.0f, IM_COL32(0, 0, 255, 255), 0, 2.0f);
}
