#ifndef VISUALS_HPP
#define VISUALS_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"
#include "../../Engine/ImGui/imgui.h"

#include <Windows.h>

struct ImDrawList;

class Visuals : public IManagedClass {
public:
    void draw(ImDrawList* drawList);
    void getAndDrawCursor(ImDrawList* drawList);
    void drawAssistedCursor(ImDrawList* drawList);

private:
    static constexpr float kAssistFov = 69.f;
    static constexpr float kAssistStrength = 0.75f;

    ImVec2 cursorPosition{};
};

#endif // VISUALS_HPP
