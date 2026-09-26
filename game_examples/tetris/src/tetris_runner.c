#include "tetris_runner.h"
#include "tetris_types.h"
#include "tetris_game.h"
#include "tetris_audio.h"

static TetrisGame s_tetrisGame;
static bool s_tetrisInitialized = false;

void InitTetrisApp(void)
{
    InitTetrisAudio();
    InitGame(&s_tetrisGame);
    s_tetrisInitialized = true;
}

void UpdateTetrisApp(float dt)
{
    if (s_tetrisInitialized) {
        UpdateGame(&s_tetrisGame, dt);
    }
}

void DrawTetrisApp(void)
{
    if (s_tetrisInitialized) {
        DrawGame(&s_tetrisGame);
    }
}

void CloseTetrisApp(void)
{
    if (s_tetrisInitialized) {
        CloseTetrisAudio();
        s_tetrisInitialized = false;
    }
}
