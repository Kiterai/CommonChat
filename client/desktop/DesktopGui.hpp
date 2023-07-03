#include <GLFW/glfw3.h>
#include <stdexcept>

class DesktopGuiSystem {
  private:
    GLFWwindow *window;

  public:
    DesktopGuiSystem();
    ~DesktopGuiSystem();

    void mainLoop();
};
