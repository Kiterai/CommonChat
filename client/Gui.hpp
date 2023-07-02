#include "xr/Xr.hpp"
#ifdef USE_DESKTOP_MODE
#include "desktop/DesktopGui.hpp"
#endif

class Gui {
  private:
    XrManager xrManger;
#ifdef USE_DESKTOP_MODE
    DesktopGuiSystem desktopGuiSys;
#endif

  public:
    Gui();
    ~Gui();

    void mainloop();
};
