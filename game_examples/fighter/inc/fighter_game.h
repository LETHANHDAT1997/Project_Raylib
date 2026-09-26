#ifndef FIGHTER_GAME_H
#define FIGHTER_GAME_H

#include "fighter_types.h"

// Khởi tạo & giải phóng game
void InitFighterGame(FighterGame *game);
void CloseFighterGame(FighterGame *game);

// Game loop
void UpdateFighterGame(FighterGame *game, float dt);
void DrawFighterGame(FighterGame *game);

#endif // FIGHTER_GAME_H
