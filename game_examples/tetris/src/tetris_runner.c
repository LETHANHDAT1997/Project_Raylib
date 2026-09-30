#include "tetris_runner.h"
#include "tetris_types.h"
#include "tetris_game.h"
#include "tetris_audio.h"
#include "save_data.h"

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
        // Thoát giữa ván (F1 về Hub) vẫn giữ lại kỷ lục vừa lập.
        SaveDataSubmitBest(TETRIS_SAVE_ID, "best", s_tetrisGame.highScore);
        CloseTetrisAudio();
        s_tetrisInitialized = false;
    }
}
