#ifndef SPACE_GAME_H
#define SPACE_GAME_H

#include "space_types.h"

void InitSpaceGame(SpaceGame *game);
void ResetSpaceGame(SpaceGame *game, bool resetScore);
void StartNextWave(SpaceGame *game);
void UpdateSpaceGame(SpaceGame *game, float dt);
void DrawSpaceGame(const SpaceGame *game);

void AddSpaceParticles(SpaceGame *game, float x, float y, Color color, int count);
void UpdateSpaceParticles(SpaceGame *game, float dt);
void DrawSpaceParticles(const SpaceGame *game);

#endif // SPACE_GAME_H
