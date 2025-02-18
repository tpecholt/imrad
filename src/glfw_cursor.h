#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <stdint.h>

const unsigned char CUSTOM_BUSY_CURSOR[] = {
    "\x11\x16\x0\x0"
    " XXXXXXXXXXXXXXX "
    "X.............. X"
    "X.............. X"
    " XXXXXXXXXXXXXXX "
    "  X...........X  "
    "  X...........X  "
    "  X...........X  "
    "   X..X.X.X..X   "
    "   X...X.X...X   "
    "    X...X...X    "
    "     X.....X     "
    "     X..X..X     "
    "    X.......X    "
    "   X....X....X   "
    "   X.........X   "
    "  X.....X.....X  "
    "  X....X.X....X  "
    "  X...X.X.X...X  "
    " XXXXXXXXXXXXXXX "
    "X...............X"
    "X...............X"
    " XXXXXXXXXXXXXXX "
};

inline GLFWcursor* glfwCreateCursor(const unsigned char* data)
{
    GLFWimage image;
    image.width = data[0];
    image.height = data[1];
    std::vector<uint32_t> bmp;
    bmp.reserve(4 * image.width*image.height);
    for (int i = 0; i < image.width*image.height; ++i)
    {
        const char c = data[4 + i];
        if (c == '.')
            bmp.push_back(0xffffffff);
        else if (c == 'X')
            bmp.push_back(0xff000000);
        else
            bmp.push_back(0x0);
    }
    image.pixels = (unsigned char *)bmp.data();
    return glfwCreateCursor(&image, data[2], data[3]);
}
