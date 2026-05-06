#include "get_nfd_handle.h"
#include "utils.h"
#ifdef WIN32
  #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
  #define GLFW_EXPOSE_NATIVE_COCOA
#else //linux
  #define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


nfdwindowhandle_t GetNfdHandle()
{
    nfdwindowhandle_t h;
#ifdef _WIN32
    h.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
    h.handle = glfwGetWin32Window(glfwWindow);
#elif defined(__APPLE__)
    h.type = NFD_WINDOW_HANDLE_TYPE_COCOA;
    h.handle = glfwGetCocoaWindow(glfwWindow);
#else
    h.type = NFD_WINDOW_HANDLE_TYPE_X11;
    h.handle = (void*)glfwGetX11Window(glfwWindow);
#endif
    return h;
}
