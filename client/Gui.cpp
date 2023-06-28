#include "Gui.hpp"

Gui::Gui() {}

Gui::~Gui() {}

void Gui::mainloop() {
    while (true) {
        xrManger.process();
    }
    return;
}
