#include "hub_overlay.h"
#include "game_registry.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "font_vn.h"

#include <math.h>

// Khi đang chơi, launcher chỉ chèn một dải gợi ý mảnh ở góc trên bên phải.
// Cố tình KHÔNG dùng hiệu ứng kính ở đây: canvas lúc này thuộc về game,
// không có backdrop để lấy mẫu, và lớp phủ càng nhẹ càng đỡ che tầm nhìn.
void HubOverlayDraw(const BootApp *app)
{
    const GameEntry *game = GameRegistryGet(app->activeGame);
    if (!game) return;

    int canvasW = app->canvas.virtualWidth;

    const char *hint = "F1  Về Hub      F11  Toàn màn hình";
    float fs = 12.0f;
    float textW = MeasureTextVNPro(hint, fs, 0.6f).x;

    float padX = 14.0f;
    float w = textW + padX * 2.0f;
    float h = 26.0f;
    Rectangle bar = {canvasW - w - 10.0f, 8.0f, w, h};

    DrawRectangleRounded(bar, 0.5f, 8, (Color){10, 18, 36, 170});
    DrawRectangleRoundedLinesEx(bar, 0.5f, 8, 1.0f, UiAlpha(game->accent, 0.5f));

    DrawTextVNPro(hint, (Vector2){bar.x + padX, bar.y + (h - fs) * 0.5f - 1.0f}, fs, 0.6f,
                  (Color){214, 228, 250, 220});
}
