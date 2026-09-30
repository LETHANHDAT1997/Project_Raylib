#ifndef FLAPPY_GAME_H
#define FLAPPY_GAME_H

#include "flappy_types.h"

void InitFlappyGame(FlappyGame *game);
void UpdateFlappyGame(FlappyGame *game, float dt);
void CloseFlappyGame(FlappyGame *game);

#endif // FLAPPY_GAME_H
