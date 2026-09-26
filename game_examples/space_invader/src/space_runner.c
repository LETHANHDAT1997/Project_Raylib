#include "space_runner.h"
#include "space_types.h"
#include "space_game.h"
#include "space_audio.h"

static SpaceGame s_spaceGame;
static bool s_spaceInitialized = false;

void InitSpaceApp(void)
{
    InitSpaceAudio();
    InitSpaceGame(&s_spaceGame);
    s_spaceInitialized = true;
}

void UpdateSpaceApp(float dt)
{
    if (s_spaceInitialized) {
        UpdateSpaceGame(&s_spaceGame, dt);
    }
}

void DrawSpaceApp(void)
{
    if (s_spaceInitialized) {
        DrawSpaceGame(&s_spaceGame);
    }
}

void CloseSpaceApp(void)
{
    if (s_spaceInitialized) {
        CloseSpaceAudio();
        s_spaceInitialized = false;
    }
}
