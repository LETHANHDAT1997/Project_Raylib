#ifndef CARO_GAME_H
#define CARO_GAME_H

#include "caro_types.h"

void InitCaroGame(CaroGame *game);
void UpdateCaroGame(CaroGame *game, float dt);
void CloseCaroGame(CaroGame *game);

// Chuyển dữ liệu kiểu cũ (nếu có) - Caro là game mới nên hiện không cần làm gì,
// giữ hàm để Hub gọi đồng nhất với các game khác.
void MigrateCaroSave(void);

#endif // CARO_GAME_H
