#include "snake_runner.h"
#include "snake_types.h"
#include "snake_game.h"
#include "snake_audio.h"
#include "font_vn.h"

static SnakeGame s_snakeGame;
static bool s_snakeInitialized = false;

void InitSnakeApp(void)
{
    InitVietnameseFont();
    InitSnakeAudio();
    InitSnakeGame(&s_snakeGame);
    s_snakeInitialized = true;
}

void UpdateSnakeApp(float dt)
{
    if (s_snakeInitialized) {
        UpdateSnakeGame(&s_snakeGame, dt);
    }
}

void DrawSnakeApp(void)
{
    if (s_snakeInitialized) {
        DrawSnakeGame(&s_snakeGame);
    }
}

void CloseSnakeApp(void)
{
    if (s_snakeInitialized) {
        CloseSnakeGame(&s_snakeGame);
        CloseSnakeAudio();
        CloseVietnameseFont();
        s_snakeInitialized = false;
    }
}
