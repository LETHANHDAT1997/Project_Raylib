#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include "snake_types.h"

void InitSnakeGame(SnakeGame *game);
void ResetSnakeGame(SnakeGame *game);
void UpdateSnakeGame(SnakeGame *game, float dt);
void DrawSnakeGame(SnakeGame *game);
void CloseSnakeGame(SnakeGame *game);

#endif // SNAKE_GAME_H
