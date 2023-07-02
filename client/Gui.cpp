#include "Gui.hpp"

Gui::Gui() {}

Gui::~Gui() {}

void Gui::mainloop() {
#ifdef USE_DESKTOP_MODE
    desktopGuiSys.mainLoop();
#else
    while (true) {
        xrManger.process();
    }
#endif
    return;
}
