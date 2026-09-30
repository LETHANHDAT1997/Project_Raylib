#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include "chess_types.h"

void InitChessGame(ChessGame *game);
void UpdateChessGame(ChessGame *game, float dt);   // Gồm cả dựng cảnh 3D (ngoài BeginTextureMode)
void DrawChessGame(const ChessGame *game);         // Vẽ lên canvas hiện tại
void CloseChessGame(ChessGame *game);

#endif // CHESS_GAME_H
