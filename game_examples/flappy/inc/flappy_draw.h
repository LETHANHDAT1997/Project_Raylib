#ifndef FLAPPY_DRAW_H
#define FLAPPY_DRAW_H

#include "flappy_types.h"

// Bố cục các nút bấm được dùng chung giữa phần vẽ và phần xử lý chuột,
// để vùng bấm luôn khớp đúng với thứ người chơi nhìn thấy (toạ độ canvas).
Rectangle FlappyMenuPlaneCard(int index);
Rectangle FlappyMenuDifficultyChip(int index);
Rectangle FlappyMenuStartButton(void);
Rectangle FlappyGameOverButton(int index);   // 0 = Chơi lại, 1 = Menu

void DrawFlappyGame(const FlappyGame *game);

#endif // FLAPPY_DRAW_H
