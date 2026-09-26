#include "fighter_runner.h"
#include "fighter_types.h"
#include "fighter_game.h"

static FighterGame s_fighterGame;
static bool s_fighterInitialized = false;

void InitFighterApp(void)
{
    InitFighterGame(&s_fighterGame);
    s_fighterInitialized = true;
}

void UpdateFighterApp(float dt)
{
    if (s_fighterInitialized) {
        UpdateFighterGame(&s_fighterGame, dt);
    }
}

void DrawFighterApp(void)
{
    if (s_fighterInitialized) {
        DrawFighterGame(&s_fighterGame);
    }
}

void CloseFighterApp(void)
{
    if (s_fighterInitialized) {
        CloseFighterGame(&s_fighterGame);
        s_fighterInitialized = false;
    }
}
