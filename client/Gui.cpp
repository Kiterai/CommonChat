#include "Gui.hpp"

Gui::Gui() {}

Gui::~Gui() {}

void Gui::mainloop() {
#ifdef USE_DESKTOP_MODE
    desktopGuiSys.mainLoop();
#else
    xrManger.mainLoop();
#endif
    return;
}
