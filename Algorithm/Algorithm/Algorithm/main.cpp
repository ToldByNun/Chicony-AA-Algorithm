#include "Manager/Globals/Globals.hpp"
#include "Manager/Classmanager/Classmanager.hpp"
#include "Modules/Overlay/Overlay.hpp"

int main() {
    ClassManager classManager;
    globals.classManager = &classManager;

    globals.overlay = classManager.addClass<Overlay>("Overlay");
    if (!classManager.init())
        return 1;

    while (globals.overlay->isRunning())
        globals.overlay->update();

    classManager.deinit();
    return 0;
}
