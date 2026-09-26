#ifndef IS_BUILD_ALL

#include "raylib.h"
#include "snake_types.h"
#include "snake_game.h"
#include "snake_audio.h"
#include "font_vn.h"
#include <math.h>

int main(void)
{
    // Cấu hình cờ cửa sổ: Tự do co giãn, đồng bộ VSync, khử răng cưa và hỗ trợ màn hình 4K/Retina
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Snake - Rắn Săn Mồi Retro Arcade");
    SetTargetFPS(60);

    // Tải font Unicode Tiếng Việt
    InitVietnameseFont();

    // Khởi tạo Virtual Canvas cho game
    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    InitSnakeAudio();

    SnakeGame game;
    InitSnakeGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // Phím F11: Chuyển đổi toàn màn hình
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // Tính toán tỷ lệ scale và vùng hiển thị Letterbox đối xứng
        float scale = fminf((float)GetScreenWidth() / SCREEN_WIDTH, (float)GetScreenHeight() / SCREEN_HEIGHT);
        if (scale <= 0.0f) scale = 1.0f;
        float destW = SCREEN_WIDTH * scale;
        float destH = SCREEN_HEIGHT * scale;
        Rectangle destRec = {
            (GetScreenWidth() - destW) * 0.5f,
            (GetScreenHeight() - destH) * 0.5f,
            destW, destH
        };
        Rectangle sourceRec = {0.0f, 0.0f, (float)SCREEN_WIDTH, -(float)SCREEN_HEIGHT};

        // Chuyển đổi tọa độ chuột ảo tự động cho các nút bấm trong game Snake
        SetMouseOffset((int)-destRec.x, (int)-destRec.y);
        SetMouseScale(1.0f / scale, 1.0f / scale);

        UpdateSnakeGame(&game, dt);

        // 1. Vẽ game lên Virtual Canvas
        BeginTextureMode(canvas);
            DrawSnakeGame(&game);
        EndTextureMode();

        // 2. Vẽ Virtual Canvas đã scale lên cửa sổ thật
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, sourceRec, destRec, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    CloseSnakeGame(&game);
    CloseSnakeAudio();

    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    CloseVietnameseFont();
    UnloadRenderTexture(canvas);
    CloseWindow();

    return 0;
}

#endif // IS_BUILD_ALL
