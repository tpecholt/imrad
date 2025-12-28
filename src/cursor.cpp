#include "cursor.h"
#include "utils.h"
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

GLFWcursor* glfwCreateCursor(const unsigned char* data)
{
    GLFWimage image;
    image.width = data[0];
    image.height = data[1];
    std::vector<uint32_t> bmp;
    bmp.reserve(4 * image.width * image.height);
    for (int i = 0; i < image.width * image.height; ++i)
    {
        const char c = data[4 + i];
        if (c == '.')
            bmp.push_back(0xffffffff);
        else if (c == 'X')
            bmp.push_back(0xff000000);
        else
            bmp.push_back(0x0);
    }
    image.pixels = (unsigned char*)bmp.data();
    return glfwCreateCursor(&image, data[2], data[3]);
}

struct WrappedCursor
{
    enum Type { WAIT, CROSS };
    WrappedCursor(Type type)
        : type(type), cursor()
    {
    }
    ~WrappedCursor()
    {
        if (cursor)
            glfwDestroyCursor(cursor);
        cursor = nullptr;
    }
    GLFWcursor* get()
    {
        if (!cursor) {
            if (type == CROSS)
                cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
            else if (type == WAIT)
                cursor = glfwCreateCursor(CUSTOM_BUSY_CURSOR);
        }
        return cursor;
    }

private:
    Type type;
    GLFWcursor* cursor = nullptr;
};

WrappedCursor curWait(WrappedCursor::WAIT);
WrappedCursor curCross(WrappedCursor::CROSS);

void SetWaitCursor()
{
    glfwSetCursor(glfwWindow, curWait.get());
}

void SetCrossCursor()
{
    glfwSetCursor(glfwWindow, curCross.get());
}

void DestroyCursors()
{
    curWait.~WrappedCursor();
    curCross.~WrappedCursor();
}
