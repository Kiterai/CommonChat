#include "xr/Xr.hpp"
#ifdef USE_DESKTOP_MODE
#include "desktop/DesktopGui.hpp"
#endif

class Gui {
  private:
#ifdef USE_DESKTOP_MODE
    DesktopGuiSystem desktopGuiSys;
#else
    XrManager xrManger;
#endif

  public:
    Gui();
    ~Gui();

    void mainloop();
};
