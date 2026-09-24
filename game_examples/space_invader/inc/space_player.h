#ifndef SPACE_PLAYER_H
#define SPACE_PLAYER_H

#include "space_types.h"

void InitPlayer(SpacePlayer *player);
void UpdatePlayer(SpaceGame *game, float dt);
void DrawPlayer(const SpacePlayer *player);
void DrawLives(int lives, int startX, int y);

#endif // SPACE_PLAYER_H
