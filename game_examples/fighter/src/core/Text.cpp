#include "core/Text.hpp"
#include <string>
#include <vector>

extern "C" {
#include "font_vn.h"
}

namespace fighter {

void TextInit()     { InitVietnameseFont(); }
void TextShutdown() { CloseVietnameseFont(); }

void DrawText(const char *s, Vector2 pos, float size, Color color)
{
    DrawTextVNPro(s, pos, size, size * 0.06f, color);
}

void DrawTextBold(const char *s, Vector2 pos, float size, Color color)
{
    DrawTextVNBoldPro(s, pos, size, size * 0.06f, color);
}

float TextWidth(const char *s, float size)
{
    return MeasureTextVNPro(s, size, size * 0.06f).x;
}

float TextWidthBold(const char *s, float size)
{
    return MeasureTextVNBoldPro(s, size, size * 0.06f).x;
}

void DrawTextCentered(const char *s, float cx, float y, float size, Color color)
{
    DrawText(s, Vector2{cx - TextWidth(s, size) * 0.5f, y}, size, color);
}

void DrawTextBoldCentered(const char *s, float cx, float y, float size, Color color)
{
    DrawTextBold(s, Vector2{cx - TextWidthBold(s, size) * 0.5f, y}, size, color);
}

float DrawTextWrapped(const char *s, Rectangle box, float size, float lineGap,
                      Color color, bool centered)
{
    if (!s || !*s) return 0.0f;

    std::string text(s);
    std::vector<std::string> words;
    {
        std::string cur;
        for (char c : text) {
            if (c == ' ') { if (!cur.empty()) { words.push_back(cur); cur.clear(); } }
            else cur.push_back(c);
        }
        if (!cur.empty()) words.push_back(cur);
    }

    float y = box.y;
    std::string line;
    auto flush = [&]() {
        if (line.empty()) return;
        if (centered) DrawTextCentered(line.c_str(), box.x + box.width * 0.5f, y, size, color);
        else          DrawText(line.c_str(), Vector2{box.x, y}, size, color);
        y += size + lineGap;
        line.clear();
    };

    for (const std::string &w : words) {
        const std::string candidate = line.empty() ? w : line + " " + w;
        if (TextWidth(candidate.c_str(), size) > box.width && !line.empty()) flush();
        line = line.empty() ? w : line + " " + w;
    }
    flush();

    return y - box.y;
}

} // namespace fighter
