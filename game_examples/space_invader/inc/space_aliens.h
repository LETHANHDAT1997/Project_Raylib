#ifndef SPACE_ALIENS_H
#define SPACE_ALIENS_H

#include "space_types.h"

void InitAliens(SpaceGame *game);
void UpdateAliens(SpaceGame *game, float dt);
void DrawAlien(AlienType type, int frame, float x, float y, Color color);
void DrawAliens(const SpaceGame *game);

void InitUfo(SpaceUfo *ufo);
void UpdateUfo(SpaceGame *game, float dt);
void DrawUfo(const SpaceUfo *ufo);

#endif // SPACE_ALIENS_H
