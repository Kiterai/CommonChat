#include "../IGraphics.hpp"
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <openxr/openxr.hpp>

#ifdef USE_DESKTOP_MODE
pIGraphics makeFromDesktopGui_Vulkan(GLFWwindow* window);
#endif
pIGraphics makeFromXr_Vulkan(xr::Instance xrInst, xr::SystemId xrSysId);

