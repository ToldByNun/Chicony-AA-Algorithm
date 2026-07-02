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
    ImVec2 cursorPosition;

};

#endif // VISUALS_HPP
