#include <GLFW/glfw3.h>
#include <stdexcept>
#include <memory>
#include "../graphics/IGraphics.hpp"
#include "../graphics/vulkan/VulkanGlfwAdapter.hpp"

class DesktopGuiSystem {
  private:
    GLFWwindow *window;
    pIGraphics g;
    std::unique_ptr<VulkanManagerGlfw> graphicManager;
  public:
    DesktopGuiSystem();
    ~DesktopGuiSystem();

    void mainLoop();
};
