#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include "tetris_types.h"

void InitGame(TetrisGame *game);
void ResetGame(TetrisGame *game);
void UpdateGame(TetrisGame *game, float dt);
void DrawGame(const TetrisGame *game);

void AddParticles(TetrisGame *game, float x, float y, Color color, int count);
void UpdateParticles(TetrisGame *game, float dt);
void DrawParticles(const TetrisGame *game);

#endif // TETRIS_GAME_H
