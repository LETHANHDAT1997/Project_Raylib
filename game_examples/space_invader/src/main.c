#include "raylib.h"
#include "space_types.h"
#include "space_game.h"
#include "space_audio.h"

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Space Invaders - Classic Arcade Remastered");
    SetTargetFPS(60);

    InitSpaceAudio();

    SpaceGame game;
    InitSpaceGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        UpdateSpaceGame(&game, dt);

        BeginDrawing();
            DrawSpaceGame(&game);
        EndDrawing();
    }

    CloseSpaceAudio();
    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    CloseWindow();

    return 0;
}
