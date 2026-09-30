#include "caro_runner.h"
#include "caro_types.h"
#include "caro_game.h"
#include "caro_draw.h"
#include "caro_assets.h"
#include "font_vn.h"

static CaroGame s_caroGame;
static bool s_caroInitialized = false;

void InitCaroApp(void)
{
    InitVietnameseFont();
    LoadCaroAssets();
    InitCaroAudio();
    InitCaroGame(&s_caroGame);
    s_caroInitialized = true;
}

void UpdateCaroApp(float dt)
{
    if (s_caroInitialized) UpdateCaroGame(&s_caroGame, dt);
}

void DrawCaroApp(void)
{
    if (s_caroInitialized) DrawCaroGame(&s_caroGame);
}

void CloseCaroApp(void)
{
    if (s_caroInitialized) {
        CloseCaroGame(&s_caroGame);
        CloseCaroAudio();
        UnloadCaroAssets();
        CloseVietnameseFont();
        s_caroInitialized = false;
    }
}
