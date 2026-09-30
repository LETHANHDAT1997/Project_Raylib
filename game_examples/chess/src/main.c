#ifndef IS_BUILD_ALL

#include "raylib.h"
#include "chess_types.h"
#include "chess_game.h"
#include "chess_render.h"
#include "chess_assets.h"
#include "font_vn.h"
#include <math.h>

int main(void)
{
    // Cấu hình cờ cửa sổ: Tự do co giãn, đồng bộ VSync, khử răng cưa và hỗ trợ màn hình 4K/Retina
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 720, "Raylib Cờ Vua 3D");
    SetExitKey(KEY_NULL);   // Esc dùng trong game (bỏ chọn / tạm dừng), thoát bằng Esc ở menu
    SetTargetFPS(60);

    InitVietnameseFont();
    ChessRenderInit();
    InitChessAudio();

    // Virtual Canvas: game luôn vẽ ở 1600x900 rồi scale letterbox ra cửa sổ thật
    RenderTexture2D canvas = LoadRenderTexture(CHESS_CANVAS_W, CHESS_CANVAS_H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    ChessGame game;
    InitChessGame(&game);

    while (!WindowShouldClose() && !game.quitRequested) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        float scale = fminf((float)GetScreenWidth() / CHESS_CANVAS_W, (float)GetScreenHeight() / CHESS_CANVAS_H);
        if (scale <= 0.0f) scale = 1.0f;
        float destW = CHESS_CANVAS_W * scale;
        float destH = CHESS_CANVAS_H * scale;
        Rectangle destRec = {(GetScreenWidth() - destW) * 0.5f, (GetScreenHeight() - destH) * 0.5f, destW, destH};
        Rectangle sourceRec = {0.0f, 0.0f, (float)CHESS_CANVAS_W, -(float)CHESS_CANVAS_H};

        // Chuột được quy đổi về toạ độ canvas để các nút bấm khớp khi phóng to
        SetMouseOffset((int)-destRec.x, (int)-destRec.y);
        SetMouseScale(1.0f / scale, 1.0f / scale);

        UpdateChessGame(&game, dt);     // Gồm cả dựng cảnh 3D, phải nằm ngoài BeginTextureMode

        BeginTextureMode(canvas);
            DrawChessGame(&game);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, sourceRec, destRec, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    CloseChessGame(&game);
    CloseChessAudio();
    ChessRenderClose();
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseVietnameseFont();
    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}

#endif // IS_BUILD_ALL
