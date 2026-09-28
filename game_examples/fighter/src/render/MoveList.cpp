#include "render/MoveList.hpp"
#include "characters/Roster.hpp"
#include "core/Text.hpp"
#include <cmath>
#include <cstdio>

namespace fighter {

namespace {
struct SystemMove { const char *name; const char *input; };
const SystemMove kSystem[] = {
    {"Đòn nhẹ / vừa / mạnh",   "J K L  (hoặc Z X C)"},
    {"Đòn ngồi (có quét chân)", "↓ + đòn"},
    {"Đòn nhảy (đòn trên)",     "Khi nhảy + đòn"},
    {"Đỡ đòn (đứng / ngồi)",    "Giữ ← / ↙"},
    {"Vật · phá vật",           "J+K (hoặc Z+X)"},
    {"Lướt tới / lùi",          "→ → / ← ←"},
    {"Nối đòn (combo)",         "J → K → L → chiêu"},
    {"Chiêu nhanh",             "I / V (+ → ↓ ←)"},
};
} // namespace

void DrawMoveList(const CharacterDef &def, Rectangle area, float time)
{
    DrawRectangleRounded(area, 0.04f, 8, Color{12, 11, 20, 240});
    DrawRectangleRoundedLines(area, 0.04f, 8, def.accent);

    float y = area.y + 18.0f;
    const float x = area.x + 24.0f;
    const float w = area.width - 48.0f;

    char title[96];
    std::snprintf(title, sizeof(title), "BẢNG CHIÊU  ·  %s", def.name.c_str());
    DrawTextBold(title, Vector2{x, y}, 26.0f, def.accent);
    DrawText(def.title.c_str(), Vector2{x + TextWidthBold(title, 26.0f) + 14.0f, y + 8.0f}, 15.0f,
             Color{170, 166, 164, 220});
    y += 44.0f;

    // --- chiêu riêng ------------------------------------------------------
    const float inputCol = x + w * 0.42f;
    for (size_t i = 0; i < def.moveList.size(); ++i) {
        const MoveListEntry &m = def.moveList[i];
        const bool super = i + 1 == def.moveList.size();
        if (i % 2 == 0 || super) {
            DrawRectangleRec(Rectangle{x - 8, y - 4, w + 16, 46}, Color{255, 255, 255, 8});
        }
        if (super) {
            const float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);
            DrawRectangleRec(Rectangle{x - 8, y - 4, 4, 46},
                             Color{def.accent.r, def.accent.g, def.accent.b, (unsigned char)(150 + 100 * pulse)});
        }
        DrawTextBold(m.name.c_str(), Vector2{x, y}, 19.0f,
                     super ? def.accent : Color{236, 230, 220, 255});
        DrawTextBold(m.input.c_str(), Vector2{inputCol, y}, 19.0f, Color{255, 214, 120, 255});
        DrawText(m.note.c_str(), Vector2{x, y + 23.0f}, 13.0f, Color{160, 156, 156, 230});
        y += 50.0f;
    }

    // --- thao tác chung ---------------------------------------------------
    y += 6.0f;
    DrawLine((int)x, (int)y, (int)(x + w), (int)y, Color{70, 66, 84, 220});
    y += 12.0f;
    DrawTextBold("THAO TÁC CHUNG", Vector2{x, y}, 15.0f, Color{170, 166, 164, 230});
    y += 24.0f;

    const float colW = w * 0.5f;
    for (int i = 0; i < (int)(sizeof(kSystem) / sizeof(kSystem[0])); ++i) {
        const float cx = x + (i % 2) * colW;
        const float cy = y + (i / 2) * 24.0f;
        DrawText(kSystem[i].name, Vector2{cx, cy}, 14.0f, Color{200, 196, 190, 235});
        DrawTextBold(kSystem[i].input, Vector2{cx + colW * 0.56f, cy}, 14.0f, Color{255, 214, 120, 235});
    }

    DrawText("Di chuyển: WASD hoặc mũi tên (khi chơi 1 người) · Siêu chiêu: U hoặc B · → là tiến về phía đối thủ.",
             Vector2{x, area.y + area.height - 28.0f}, 13.0f, Color{140, 136, 140, 220});
}

} // namespace fighter
