/**
 * main.c - Điểm vào của Arcade Hub.
 *
 * File này chỉ lo phần "hạ tầng": môi trường, cửa sổ, vòng lặp.
 * Mọi logic điều phối màn hình nằm trong core/boot_app.c.
 */
#include "raylib.h"
#include "boot_types.h"
#include "boot_app.h"
#include "font_vn.h"

#include <stdlib.h>

#define WINDOW_TITLE       "Arcade Hub · Bộ sưu tập game Raylib"
#define WINDOW_MIN_WIDTH   1024
#define WINDOW_MIN_HEIGHT  600
#define WINDOW_SCREEN_FILL 0.85f

// Bộ gõ tiếng Việt (ibus/fcitx/Bamboo) nuốt phím khi game đang chạy,
// nên tắt hẳn IME cho tiến trình này trước khi mở cửa sổ.
static void DisableInputMethod(void)
{
    setenv("XMODIFIERS", "@im=none", 1);
    setenv("GTK_IM_MODULE", "", 1);
    setenv("QT_IM_MODULE", "", 1);
    setenv("GLFW_IM_MODULE", "none", 1);
}

// Mở cửa sổ chiếm phần lớn màn hình và căn giữa.
static void FitWindowToMonitor(void)
{
    int monitor = GetCurrentMonitor();
    int screenW = GetMonitorWidth(monitor);
    int screenH = GetMonitorHeight(monitor);
    if (screenW <= 0 || screenH <= 0) return;

    int w = (int)(screenW * WINDOW_SCREEN_FILL);
    int h = (int)(screenH * WINDOW_SCREEN_FILL);
    if (w < WINDOW_MIN_WIDTH || h < WINDOW_MIN_HEIGHT) return;

    SetWindowSize(w, h);
    SetWindowPosition((screenW - w) / 2, (screenH - h) / 2);
}

int main(void)
{
    DisableInputMethod();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 720, WINDOW_TITLE);
    SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   // Esc dùng cho điều hướng trong Hub, không thoát app

    InitAudioDevice();
    InitVietnameseFont();
    FitWindowToMonitor();

    BootApp app = {0};
    BootAppInit(&app);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;   // Chặn bước nhảy lớn khi cửa sổ bị kéo/thu nhỏ

        BootAppUpdate(&app, dt);
        BootAppDraw(&app);
    }

    BootAppClose(&app);
    CloseVietnameseFont();
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();

    return 0;
}
