#include <GLFW/glfw3.h>
#include <stdexcept>

class DesktopGuiSystem {
  public:
    DesktopGuiSystem();
    ~DesktopGuiSystem();

    void mainLoop();
};
