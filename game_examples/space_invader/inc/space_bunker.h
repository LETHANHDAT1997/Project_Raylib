#ifndef SPACE_BUNKER_H
#define SPACE_BUNKER_H

#include "space_types.h"

void InitBunkers(SpaceBunker bunkers[MAX_BUNKERS]);
void DrawBunkers(const SpaceBunker bunkers[MAX_BUNKERS]);
bool CheckBunkerCollision(SpaceBunker bunkers[MAX_BUNKERS], float bx, float by, float radius, SpaceGame *game);

#endif // SPACE_BUNKER_H
