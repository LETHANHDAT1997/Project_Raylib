#ifndef FIGHTER_DRAW_H
#define FIGHTER_DRAW_H

#include "fighter_types.h"

// Vẽ background (sân đấu kiểu Nhật Bản)
void DrawBackground(FighterGame *game);

// Vẽ HUD (thanh máu, tên, timer, round indicators)
void DrawHUD(FighterGame *game);

// Vẽ các màn hình chuyển cảnh
void DrawTitleScreen(FighterGame *game);
void DrawIntroScreen(FighterGame *game);
void DrawRoundEndScreen(FighterGame *game);
void DrawMatchEndScreen(FighterGame *game);

#endif // FIGHTER_DRAW_H
