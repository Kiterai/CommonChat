#include "../IGraphics.hpp"
#include <vulkan/vulkan.hpp>
#ifdef USE_DESKTOP_MODE
#include <GLFW/glfw3.h>
#endif
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>

#ifdef USE_DESKTOP_MODE
pIGraphics makeFromDesktopGui_Vulkan(GLFWwindow* window);
#endif
pIGraphics makeFromXr_Vulkan(xr::Instance xrInst, xr::SystemId xrSysId);

