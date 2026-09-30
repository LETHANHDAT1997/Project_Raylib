#ifndef IS_BUILD_ALL

#include "raylib.h"
#include "caro_types.h"
#include "caro_game.h"
#include "caro_draw.h"
#include "caro_assets.h"
#include "font_vn.h"
#include <math.h>

int main(void)
{
    // Cấu hình cờ cửa sổ: Tự do co giãn, đồng bộ VSync, khử răng cưa và hỗ trợ màn hình 4K/Retina
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(CARO_CANVAS_W, CARO_CANVAS_H, "Raylib Cờ Caro");
    SetExitKey(KEY_NULL);   // Esc dùng trong game (tạm dừng / về menu), thoát bằng Esc ở menu
    SetTargetFPS(60);

    InitVietnameseFont();
    LoadCaroAssets();
    InitCaroAudio();

    // Virtual Canvas: game luôn vẽ ở 1280x720 rồi scale letterbox ra cửa sổ thật
    RenderTexture2D canvas = LoadRenderTexture(CARO_CANVAS_W, CARO_CANVAS_H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    CaroGame game;
    InitCaroGame(&game);

    while (!WindowShouldClose() && !game.quitRequested) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        float scale = fminf((float)GetScreenWidth() / CARO_CANVAS_W, (float)GetScreenHeight() / CARO_CANVAS_H);
        if (scale <= 0.0f) scale = 1.0f;
        float destW = CARO_CANVAS_W * scale;
        float destH = CARO_CANVAS_H * scale;
        Rectangle destRec = {(GetScreenWidth() - destW) * 0.5f, (GetScreenHeight() - destH) * 0.5f, destW, destH};
        Rectangle sourceRec = {0.0f, 0.0f, (float)CARO_CANVAS_W, -(float)CARO_CANVAS_H};

        // Chuột được quy đổi về toạ độ canvas để các nút bấm khớp khi phóng to
        SetMouseOffset((int)-destRec.x, (int)-destRec.y);
        SetMouseScale(1.0f / scale, 1.0f / scale);

        UpdateCaroGame(&game, dt);

        BeginTextureMode(canvas);
            DrawCaroGame(&game);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, sourceRec, destRec, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    CloseCaroGame(&game);
    CloseCaroAudio();
    UnloadCaroAssets();
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseVietnameseFont();
    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}

#endif // IS_BUILD_ALL
