#ifndef IS_BUILD_ALL

#include "raylib.h"
#include "flappy_types.h"
#include "flappy_game.h"
#include "flappy_draw.h"
#include "flappy_assets.h"
#include "flappy_audio.h"
#include "font_vn.h"
#include <math.h>

int main(void)
{
    // Cấu hình cờ cửa sổ: Tự do co giãn, đồng bộ VSync, khử răng cưa và hỗ trợ màn hình 4K/Retina
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(CANVAS_W, CANVAS_H, "Raylib Flappy Plane - Phi Công Tí Hon");
    SetExitKey(KEY_NULL);   // Esc dùng trong game (tạm dừng / về menu), thoát bằng Esc ở menu
    SetTargetFPS(60);

    InitVietnameseFont();
    LoadFlappyAssets();
    InitFlappyAudio();

    // Virtual Canvas: game luôn vẽ ở 1280x768 rồi scale letterbox ra cửa sổ thật
    RenderTexture2D canvas = LoadRenderTexture(CANVAS_W, CANVAS_H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    FlappyGame game;
    InitFlappyGame(&game);

    while (!WindowShouldClose() && !game.quitRequested) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        float scale = fminf((float)GetScreenWidth() / CANVAS_W, (float)GetScreenHeight() / CANVAS_H);
        if (scale <= 0.0f) scale = 1.0f;
        float destW = CANVAS_W * scale;
        float destH = CANVAS_H * scale;
        Rectangle destRec = {
            (GetScreenWidth() - destW) * 0.5f,
            (GetScreenHeight() - destH) * 0.5f,
            destW, destH
        };
        Rectangle sourceRec = {0.0f, 0.0f, (float)CANVAS_W, -(float)CANVAS_H};

        // Chuột được quy đổi về toạ độ canvas để các nút bấm khớp khi phóng to
        SetMouseOffset((int)-destRec.x, (int)-destRec.y);
        SetMouseScale(1.0f / scale, 1.0f / scale);

        UpdateFlappyGame(&game, dt);

        BeginTextureMode(canvas);
            DrawFlappyGame(&game);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, sourceRec, destRec, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    CloseFlappyGame(&game);
    CloseFlappyAudio();
    UnloadFlappyAssets();

    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    CloseVietnameseFont();
    UnloadRenderTexture(canvas);
    CloseWindow();

    return 0;
}

#endif // IS_BUILD_ALL
