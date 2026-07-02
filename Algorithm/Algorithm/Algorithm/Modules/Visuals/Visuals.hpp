#ifndef VISUALS_HPP
#define VISUALS_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/ImGui/imgui.h"

#include <Windows.h>

struct ImDrawList;

class Visuals : public IManagedClass {
public:
    void draw(ImDrawList* drawList);
    void drawRealCursor(ImDrawList* drawList);
    void drawAssistedCursor(ImDrawList* drawList);

private:
    static constexpr float kAssistZoneRadius = 69.f;

    ImVec2 realCursorPosition{};
};

#endif // VISUALS_HPP
