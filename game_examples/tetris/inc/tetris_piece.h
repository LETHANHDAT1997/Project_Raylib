#ifndef TETRIS_PIECE_H
#define TETRIS_PIECE_H

#include "tetris_types.h"

Color GetPieceColor(PieceType type);
Color GetPieceGhostColor(PieceType type);
int GetPieceCell(PieceType type, int rotation, int row, int col);
void ShuffleBag(int bag[7]);
PieceType GetNextPieceFromBag(TetrisGame *game);
void InitPieceSystem(TetrisGame *game);
void SpawnPiece(TetrisGame *game);

#endif // TETRIS_PIECE_H
