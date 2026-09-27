#ifndef IS_BUILD_ALL

#include "raylib.h"
#include "Game.hpp"
#include "core/Config.hpp"
#include <cmath>
#include <cstdlib>

int main(void)
{
    // Tắt bộ gõ tiếng Việt (Bamboo, ibus, fcitx) để IME không nuốt phím game.
    setenv("XMODIFIERS", "@im=none", 1);
    setenv("GTK_IM_MODULE", "", 1);
    setenv("QT_IM_MODULE", "", 1);
    setenv("GLFW_IM_MODULE", "none", 1);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(fighter::kCanvasWidth, fighter::kCanvasHeight, "Raylib Đấu Sĩ - 2D Fighter");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   // Esc dùng trong game, không dùng để thoát

    // Canvas ảo: toàn bộ game vẽ ở 1280x720 rồi scale ra cửa sổ, giữ đúng tỉ lệ.
    RenderTexture2D canvas = LoadRenderTexture(fighter::kCanvasWidth, fighter::kCanvasHeight);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    fighter::Game game;
    game.Init();

    while (!WindowShouldClose() && !game.ShouldExit()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        const float scale = fminf((float)GetScreenWidth()  / fighter::kCanvasWidth,
                                  (float)GetScreenHeight() / fighter::kCanvasHeight);
        const float destW = fighter::kCanvasWidth  * scale;
        const float destH = fighter::kCanvasHeight * scale;
        const Rectangle dest{
            (GetScreenWidth()  - destW) * 0.5f,
            (GetScreenHeight() - destH) * 0.5f,
            destW, destH
        };
        const Rectangle src{0.0f, 0.0f,
                            (float)fighter::kCanvasWidth,
                            -(float)fighter::kCanvasHeight};

        SetMouseOffset((int)-dest.x, (int)-dest.y);
        SetMouseScale(1.0f / scale, 1.0f / scale);

        game.Update(dt);

        BeginTextureMode(canvas);
            game.Draw();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, src, dest, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    game.Shutdown();
    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}

#endif // IS_BUILD_ALL
