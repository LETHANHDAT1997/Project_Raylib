#include "flappy_runner.h"
#include "flappy_types.h"
#include "flappy_game.h"
#include "flappy_draw.h"
#include "flappy_assets.h"
#include "flappy_audio.h"
#include "font_vn.h"

static FlappyGame s_flappyGame;
static bool s_flappyInitialized = false;

void InitFlappyApp(void)
{
    InitVietnameseFont();
    LoadFlappyAssets();
    InitFlappyAudio();
    InitFlappyGame(&s_flappyGame);
    s_flappyInitialized = true;
}

void UpdateFlappyApp(float dt)
{
    if (s_flappyInitialized) {
        UpdateFlappyGame(&s_flappyGame, dt);
    }
}

void DrawFlappyApp(void)
{
    if (s_flappyInitialized) {
        DrawFlappyGame(&s_flappyGame);
    }
}

void CloseFlappyApp(void)
{
    if (s_flappyInitialized) {
        CloseFlappyGame(&s_flappyGame);
        CloseFlappyAudio();
        UnloadFlappyAssets();
        CloseVietnameseFont();
        s_flappyInitialized = false;
    }
}
