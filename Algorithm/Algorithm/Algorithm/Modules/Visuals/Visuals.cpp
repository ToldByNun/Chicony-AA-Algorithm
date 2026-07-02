#include "Visuals.hpp"

/// <summary>
/// Draws the circle in the middle
/// Draws the original cursorpos (getAndDrawCursor)
/// Draws the assisted cursorpos (drawAssistedCursor)
/// </summary>
void Visuals::draw(ImDrawList* drawList) {
    if (!drawList) return;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 center(display.x * 0.5f, display.y * 0.5f);

    drawList->AddCircle(center, 69.0f, IM_COL32(255, 0, 0, 255), 0, 2.f);
    this->getAndDrawCursor(drawList);
    this->drawAssistedCursor(drawList);
}

void Visuals::getAndDrawCursor(ImDrawList* drawList) {
    POINT point;

    // Get cursor pos relative to actual screenpos
    // Im to lazy to make it based on window pos but that should be alr if youre on 1920x1080
    if (GetCursorPos(&point)) {
        this->cursorPosition.x = point.x;
        this->cursorPosition.y = point.y;
    }

    drawList->AddCircle(this->cursorPosition, 20.0f, IM_COL32(255, 170, 85, 255), 0, 2.0f);
}

void Visuals::drawAssistedCursor(ImDrawList* drawList) {
    // None yet -> go to Assist.cpp for logic
    return;
}