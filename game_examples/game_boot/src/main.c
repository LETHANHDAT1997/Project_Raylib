#include "raylib.h"
#include "boot_types.h"
#include "boot_canvas.h"
#include "boot_menu.h"
#include "font_vn.h"

// Include các giao diện Runner tách biệt của từng game
#include "tetris_runner.h"
#include "space_runner.h"
#include "snake_runner.h"
#include "fighter_runner.h"

#include <stdlib.h>

int main(void)
{
    // Tắt bộ gõ tiếng Việt (Bamboo, ibus, fcitx) để tránh IME chặn phím game
    setenv("XMODIFIERS", "@im=none", 1);
    setenv("GTK_IM_MODULE", "", 1);
    setenv("QT_IM_MODULE", "", 1);
    setenv("GLFW_IM_MODULE", "none", 1);

    // 1. Cấu hình cờ cửa sổ: Tự do co giãn, đồng bộ VSync, khử răng cưa và hỗ trợ màn hình 4K/Retina
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);

    // Khởi tạo cửa sổ ban đầu
    InitWindow(1280, 720, "Raylib Retro Arcade Hub - Multi-Game Virtual Canvas");
    SetTargetFPS(60);

    // Tải font Unicode Tiếng Việt
    InitVietnameseFont();

    // Tự động nhận diện màn hình và đặt cửa sổ chiếm 85% màn hình người dùng, căn giữa
    int monitor = GetCurrentMonitor();
    int screenW = GetMonitorWidth(monitor);
    int screenH = GetMonitorHeight(monitor);
    if (screenW > 0 && screenH > 0) {
        int initW = (int)(screenW * 0.85f);
        int initH = (int)(screenH * 0.85f);
        if (initW >= 1280 && initH >= 720) {
            SetWindowSize(initW, initH);
            SetWindowPosition((screenW - initW) / 2, (screenH - initH) / 2);
        }
    }

    // 2. Khởi tạo Virtual Canvas trung tâm cho Arcade Hub (1280x720)
    BootApp app;
    app.currentScreen = BOOT_SCREEN_HUB;
    app.targetScreen = BOOT_SCREEN_HUB;
    app.fadeAlpha = 0.0f;
    app.isTransitioning = false;
    app.selectedGameIndex = 0;
    app.globalTime = 0.0f;

    InitBootCanvas(&app.canvas, HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT);
    InitBootMenu(&app);

    // Khởi tạo thiết bị âm thanh chung
    InitAudioDevice();

    // 3. Vòng lặp chính của Arcade Hub
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        app.globalTime += dt;

        // Cập nhật canvas và ánh xạ chuột ảo ngay đầu khung hình
        UpdateBootCanvas(&app.canvas);

        // Phím F11: Chuyển đổi Toàn màn hình (Fullscreen)
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // Phím F1 hoặc phím Home: Quay trở về Arcade Hub
        if (app.currentScreen != BOOT_SCREEN_HUB && (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_HOME))) {
            app.targetScreen = BOOT_SCREEN_HUB;
            app.isTransitioning = true;
        }

        // Xử lý chuyển cảnh (Transition)
        if (app.isTransitioning) {
            app.fadeAlpha += dt * 4.0f;
            if (app.fadeAlpha >= 1.0f) {
                app.fadeAlpha = 1.0f;

                // Dọn dẹp game cũ nếu rời khỏi
                if (app.currentScreen == BOOT_SCREEN_TETRIS && app.targetScreen != BOOT_SCREEN_TETRIS) {
                    CloseTetrisApp();
                } else if (app.currentScreen == BOOT_SCREEN_SPACE_INVADER && app.targetScreen != BOOT_SCREEN_SPACE_INVADER) {
                    CloseSpaceApp();
                } else if (app.currentScreen == BOOT_SCREEN_SNAKE && app.targetScreen != BOOT_SCREEN_SNAKE) {
                    CloseSnakeApp();
                } else if (app.currentScreen == BOOT_SCREEN_FIGHTER && app.targetScreen != BOOT_SCREEN_FIGHTER) {
                    CloseFighterApp();
                }

                // Chuyển sang màn hình mới và cập nhật kích thước Canvas ảo tương ứng
                app.currentScreen = app.targetScreen;
                app.isTransitioning = false;

                if (app.currentScreen == BOOT_SCREEN_HUB) {
                    SetBootCanvasSize(&app.canvas, HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT);
                } else if (app.currentScreen == BOOT_SCREEN_TETRIS) {
                    SetBootCanvasSize(&app.canvas, 780, 740);
                    InitTetrisApp();
                } else if (app.currentScreen == BOOT_SCREEN_SPACE_INVADER) {
                    SetBootCanvasSize(&app.canvas, 800, 880);
                    InitSpaceApp();
                } else if (app.currentScreen == BOOT_SCREEN_SNAKE) {
                    SetBootCanvasSize(&app.canvas, 960, 720);
                    InitSnakeApp();
                } else if (app.currentScreen == BOOT_SCREEN_FIGHTER) {
                    SetBootCanvasSize(&app.canvas, 1024, 600);
                    InitFighterApp();
                }
            }
        } else if (app.fadeAlpha > 0.0f) {
            app.fadeAlpha -= dt * 4.0f;
            if (app.fadeAlpha < 0.0f) app.fadeAlpha = 0.0f;
        }

        // Cập nhật logic theo màn hình hiện tại
        if (app.currentScreen == BOOT_SCREEN_HUB) {
            UpdateBootMenu(&app, dt);
        } else if (app.currentScreen == BOOT_SCREEN_TETRIS) {
            UpdateTetrisApp(dt);
        } else if (app.currentScreen == BOOT_SCREEN_SPACE_INVADER) {
            UpdateSpaceApp(dt);
        } else if (app.currentScreen == BOOT_SCREEN_SNAKE) {
            UpdateSnakeApp(dt);
        } else if (app.currentScreen == BOOT_SCREEN_FIGHTER) {
            UpdateFighterApp(dt);
        }

        // Vẽ vào Virtual Canvas
        BeginBootCanvas(&app.canvas);

            if (app.currentScreen == BOOT_SCREEN_HUB) {
                DrawBootMenu(&app);
            } else if (app.currentScreen == BOOT_SCREEN_TETRIS) {
                DrawTetrisApp();
            } else if (app.currentScreen == BOOT_SCREEN_SPACE_INVADER) {
                DrawSpaceApp();
            } else if (app.currentScreen == BOOT_SCREEN_SNAKE) {
                DrawSnakeApp();
            } else if (app.currentScreen == BOOT_SCREEN_FIGHTER) {
                DrawFighterApp();
            }

            // Thanh điều hướng nhanh góc trên khi đang chơi game
            if (app.currentScreen != BOOT_SCREEN_HUB) {
                DrawRectangle(app.canvas.virtualWidth - 210, 8, 202, 26, (Color){15, 23, 42, 190});
                DrawRectangleLines(app.canvas.virtualWidth - 210, 8, 202, 26, (Color){51, 65, 85, 200});
                DrawTextVN("[ F1 ]: Về Menu Hub", app.canvas.virtualWidth - 200, 14, 12, (Color){56, 189, 248, 240});
                DrawTextVN("[ F11 ]: Phóng To", app.canvas.virtualWidth - 95, 14, 12, (Color){148, 163, 184, 240});
            }

            // Hiệu ứng mờ chuyển cảnh Fade
            if (app.fadeAlpha > 0.001f) {
                DrawRectangle(0, 0, app.canvas.virtualWidth, app.canvas.virtualHeight, Fade(BLACK, app.fadeAlpha));
            }

        EndBootCanvas(&app.canvas);
    }

    // 4. Dọn dẹp tài nguyên
    if (app.currentScreen == BOOT_SCREEN_TETRIS) CloseTetrisApp();
    if (app.currentScreen == BOOT_SCREEN_SPACE_INVADER) CloseSpaceApp();
    if (app.currentScreen == BOOT_SCREEN_SNAKE) CloseSnakeApp();
    if (app.currentScreen == BOOT_SCREEN_FIGHTER) CloseFighterApp();

    CloseBootCanvas(&app.canvas);
    CloseVietnameseFont();

    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    CloseWindow();

    return 0;
}
