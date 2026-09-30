#include "space_runner.h"
#include "space_types.h"
#include "space_game.h"
#include "space_audio.h"
#include "save_data.h"

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
        // Thoát giữa ván (F1 về Hub) vẫn giữ lại kỷ lục vừa lập.
        SaveDataSubmitBest(SPACE_SAVE_ID, "best", s_spaceGame.highScore);
        CloseSpaceAudio();
        s_spaceInitialized = false;
    }
}
