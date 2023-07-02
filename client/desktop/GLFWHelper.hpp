#include <GLFW/glfw3.h>

#define __STRINGIFY(x) #x
#define __STRINGIFY2(x) __STRINGIFY(x)
#define __MAKE_ERROR_STRING(file, line, desc) std::string("in " file ", line " line ": ") + desc
#define __ERROR_THROW(file, line, desc) \
    { throw std::runtime_error(std::string("in " file ", line " line ": ") + desc); }
#define __GLFW_ERROR_THROW                                                    \
    {                                                                         \
        const char *desc;                                                     \
        glfwGetError(&desc);                                                  \
        auto s = __MAKE_ERROR_STRING(__FILE__, __STRINGIFY2(__LINE__), desc); \
        throw std::runtime_error(s);                                          \
    }
#define __GLFW_TERMINATE_ERROR_THROW                                          \
    {                                                                         \
        const char *desc;                                                     \
        glfwGetError(&desc);                                                  \
        auto s = __MAKE_ERROR_STRING(__FILE__, __STRINGIFY2(__LINE__), desc); \
        glfwTerminate();                                                      \
        throw std::runtime_error(s);                                          \
    }
