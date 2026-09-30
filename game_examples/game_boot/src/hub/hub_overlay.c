#include "hub_overlay.h"
#include "boot_settings.h"
#include "game_registry.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "font_vn.h"

#include <math.h>
#include <stdio.h>

#define OVERLAY_FS     12.0f
#define OVERLAY_TRACK  0.6f
#define OVERLAY_PAD_X  14.0f
#define OVERLAY_H      26.0f
#define OVERLAY_MARGIN 10.0f
#define OVERLAY_GAP    8.0f

// Một "viên" nhãn tối bán trong suốt, cùng kiểu cho dải gợi ý và bộ đếm FPS.
static void DrawPill(Rectangle r, Color border)
{
    DrawRectangleRounded(r, 0.5f, 8, (Color){10, 18, 36, 170});
    DrawRectangleRoundedLinesEx(r, 0.5f, 8, 1.0f, border);
}

// Màu theo mức mượt: xanh >= 55, vàng >= 30, đỏ dưới 30 - nhìn lướt là biết
// máy yếu (vd. Raspberry Pi) có đang theo kịp game hay không.
static Color FpsColor(int fps)
{
    if (fps >= 55) return (Color){126, 232, 160, 255};
    if (fps >= 30) return (Color){255, 214, 102, 255};
    return (Color){255, 128, 120, 255};
}

// Khi đang chơi, launcher chỉ chèn một dải gợi ý mảnh ở góc trên bên phải
// (kèm bộ đếm FPS nếu bật trong Cài đặt hoặc bằng F3).
// Cố tình KHÔNG dùng hiệu ứng kính ở đây: canvas lúc này thuộc về game,
// không có backdrop để lấy mẫu, và lớp phủ càng nhẹ càng đỡ che tầm nhìn.
void HubOverlayDraw(const BootApp *app)
{
    const GameEntry *game = GameRegistryGet(app->activeGame);
    if (!game) return;

    float canvasW = (float)app->canvas.virtualWidth;
    Color border = UiAlpha(game->accent, 0.5f);

    const char *hint = "F1  Về Hub      F3  FPS      F11  Toàn màn hình";
    float hintW = MeasureTextVNPro(hint, OVERLAY_FS, OVERLAY_TRACK).x + OVERLAY_PAD_X * 2.0f;
    Rectangle bar = {canvasW - hintW - OVERLAY_MARGIN, 8.0f, hintW, OVERLAY_H};

    DrawPill(bar, border);
    DrawTextVNPro(hint, (Vector2){bar.x + OVERLAY_PAD_X, bar.y + (OVERLAY_H - OVERLAY_FS) * 0.5f - 1.0f},
                  OVERLAY_FS, OVERLAY_TRACK, (Color){214, 228, 250, 220});

    if (!BootSettingsGet()->showFps) return;

    // Bộ đếm FPS nằm ngay bên trái dải gợi ý. Độ rộng tính theo chuỗi mẫu cố
    // định để viên nhãn không co giãn giật cục khi số FPS thay đổi.
    int fps = GetFPS();
    char text[24];
    snprintf(text, sizeof(text), "%d FPS", fps);
    float pillW = MeasureTextVNBoldPro("000 FPS", OVERLAY_FS, OVERLAY_TRACK).x + OVERLAY_PAD_X * 2.0f;
    Rectangle pill = {bar.x - OVERLAY_GAP - pillW, bar.y, pillW, OVERLAY_H};

    // Canvas hẹp (vd. Tetris 780px) không đủ chỗ thì xuống dòng dưới dải gợi ý.
    if (pill.x < OVERLAY_MARGIN) {
        pill.x = canvasW - pillW - OVERLAY_MARGIN;
        pill.y = bar.y + OVERLAY_H + 6.0f;
    }

    DrawPill(pill, border);
    float textW = MeasureTextVNBoldPro(text, OVERLAY_FS, OVERLAY_TRACK).x;
    DrawTextVNBoldPro(text, (Vector2){pill.x + (pillW - textW) * 0.5f, pill.y + (OVERLAY_H - OVERLAY_FS) * 0.5f - 1.0f},
                      OVERLAY_FS, OVERLAY_TRACK, FpsColor(fps));
}
