#ifndef IMGUI_EXTENSIONS_H
#define IMGUI_EXTENSIONS_H

#include "imgui.h"

namespace ImGui
{
    /* Limitations: DO NOT USE in combination with printf style formating, use only single wide char arguments. */
    static void TextOutlined(const char* fmt, ...)
    {
        ImVec2 pos = GetCursorPos();

        va_list args;
        va_start(args, fmt);
        pos.x += 1;
        pos.y += 1;
        SetCursorPos(pos);
        TextColoredV(ImVec4(0, 0, 0, 255), fmt, args);
        pos.x -= 1;
        pos.y -= 1;
        SetCursorPos(pos);
        TextV(fmt, args);
        va_end(args);
    }
}

#endif