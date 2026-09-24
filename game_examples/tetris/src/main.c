#include "raylib.h"
#include "tetris_types.h"
#include "tetris_game.h"
#include "tetris_audio.h"

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Tetris - Modern Classic Arcade");
    SetTargetFPS(60);

    InitTetrisAudio();

    TetrisGame game;
    InitGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f; // Clamp delta time to avoid large jumps

        UpdateGame(&game, dt);

        BeginDrawing();
            DrawGame(&game);
        EndDrawing();
    }

    CloseTetrisAudio();
    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    CloseWindow();

    return 0;
}
