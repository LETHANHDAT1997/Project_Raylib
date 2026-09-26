#ifndef IS_BUILD_ALL

#include "raylib.h"
#include "fighter_types.h"
#include "fighter_game.h"
#include <math.h>
#include <stdlib.h>

int main(void)
{
    // Tắt bộ gõ tiếng Việt (Bamboo, ibus, fcitx) để tránh IME chặn phím game
    setenv("XMODIFIERS", "@im=none", 1);
    setenv("GTK_IM_MODULE", "", 1);
    setenv("QT_IM_MODULE", "", 1);
    setenv("GLFW_IM_MODULE", "none", 1);

    // Cấu hình cửa sổ
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Fighter - Street Fighter Style");
    SetTargetFPS(60);

    // Virtual Canvas cho game (giữ tỷ lệ cố định)
    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    // Khởi tạo game
    FighterGame game;
    InitFighterGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // F11: Fullscreen
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // Letterbox scaling
        float scale = fminf((float)GetScreenWidth() / SCREEN_WIDTH,
                           (float)GetScreenHeight() / SCREEN_HEIGHT);
        if (scale <= 0.0f) scale = 1.0f;
        float destW = SCREEN_WIDTH * scale;
        float destH = SCREEN_HEIGHT * scale;
        Rectangle destRec = {
            (GetScreenWidth() - destW) * 0.5f,
            (GetScreenHeight() - destH) * 0.5f,
            destW, destH
        };
        Rectangle sourceRec = {0.0f, 0.0f, (float)SCREEN_WIDTH, -(float)SCREEN_HEIGHT};

        // Virtual mouse
        SetMouseOffset((int)-destRec.x, (int)-destRec.y);
        SetMouseScale(1.0f / scale, 1.0f / scale);

        // Update
        UpdateFighterGame(&game, dt);

        // Render to canvas
        BeginTextureMode(canvas);
            DrawFighterGame(&game);
        EndTextureMode();

        // Render canvas to screen
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, sourceRec, destRec,
                          (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    CloseFighterGame(&game);
    UnloadRenderTexture(canvas);
    CloseWindow();

    return 0;
}

#endif // IS_BUILD_ALL
