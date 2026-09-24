#ifndef TETRIS_BOARD_H
#define TETRIS_BOARD_H

#include "tetris_types.h"

void InitBoard(TetrisBoard *board);
bool CheckCollision(const TetrisBoard *board, const TetrisPiece *piece, int offsetX, int offsetY, int testRotation);
bool TryMovePiece(TetrisGame *game, int dx, int dy);
bool TryRotatePiece(TetrisGame *game, int dir); // dir = 1 (CW) or -1 (CCW)
void LockCurrentPiece(TetrisGame *game);
void UpdateGhostPiece(TetrisGame *game);
int FindFullLines(TetrisBoard *board);
void RemoveFullLines(TetrisBoard *board);

#endif // TETRIS_BOARD_H
