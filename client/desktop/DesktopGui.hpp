#include <GLFW/glfw3.h>
#include <stdexcept>
#include "../graphics/IGraphics.hpp"

class DesktopGuiSystem {
  private:
    GLFWwindow *window;
    pIGraphics g;
  public:
    DesktopGuiSystem();
    ~DesktopGuiSystem();

    void mainLoop();
};
