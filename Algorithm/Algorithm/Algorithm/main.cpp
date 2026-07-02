#include "Manager/Globals/Globals.hpp"
#include "Manager/Classmanager/Classmanager.hpp"
#include "Modules/Overlay/Overlay.hpp"
#include "Modules/Visuals/Visuals.hpp"

int main() {
    ClassManager classManager;
    globals.classManager = &classManager;

    globals.overlay = classManager.addClass<Overlay>("Overlay");
    globals.visuals = classManager.addClass<Visuals>("Visuals");

    if (!classManager.init()) return 1;

    while (globals.overlay->isRunning())
        globals.overlay->update();

    classManager.deinit();
    return 0;
}
