#include "tetris_piece.h"
#include "tetris_board.h"
#include <stdlib.h>

// 4x4 matrix for each piece and each of 4 rotations [piece][rotation][row][col]
static const int PIECE_SHAPES[8][4][4][4] = {
    // PIECE_NONE
    {{{0}}},
    // PIECE_I
    {
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}},
        {{0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}}
    },
    // PIECE_O
    {
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}
    },
    // PIECE_T
    {
        {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    },
    // PIECE_S
    {
        {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,1,0}, {0,0,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
        {{1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    },
    // PIECE_Z
    {
        {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,1,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}}
    },
    // PIECE_J
    {
        {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0}}
    },
    // PIECE_L
    {
        {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0}},
        {{1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}}
    }
};

Color GetPieceColor(PieceType type)
{
    switch (type) {
        case PIECE_I: return (Color){ 0, 215, 255, 255 };    // Cyan
        case PIECE_O: return (Color){ 241, 196, 15, 255 };   // Bright Yellow
        case PIECE_T: return (Color){ 168, 85, 247, 255 };   // Vibrant Purple
        case PIECE_S: return (Color){ 46, 204, 113, 255 };   // Emerald Green
        case PIECE_Z: return (Color){ 239, 68, 68, 255 };    // Crimson Red
        case PIECE_J: return (Color){ 59, 130, 246, 255 };   // Blue
        case PIECE_L: return (Color){ 249, 115, 22, 255 };   // Orange
        default:      return (Color){ 30, 30, 40, 255 };
    }
}

Color GetPieceGhostColor(PieceType type)
{
    Color c = GetPieceColor(type);
    c.a = 75; // Semi-transparent
    return c;
}

int GetPieceCell(PieceType type, int rotation, int row, int col)
{
    if (type <= PIECE_NONE || type >= PIECE_COUNT) return 0;
    int rot = (rotation % 4 + 4) % 4;
    if (row < 0 || row >= 4 || col < 0 || col >= 4) return 0;
    return PIECE_SHAPES[type][rot][row][col];
}

void ShuffleBag(int bag[7])
{
    for (int i = 0; i < 7; i++) {
        bag[i] = i + 1; // PIECE_I through PIECE_L
    }
    for (int i = 6; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int temp = bag[i];
        bag[i] = bag[j];
        bag[j] = temp;
    }
}

PieceType GetNextPieceFromBag(TetrisGame *game)
{
    if (game->bagIndex >= 7) {
        ShuffleBag(game->bag);
        game->bagIndex = 0;
    }
    return (PieceType)game->bag[game->bagIndex++];
}

void InitPieceSystem(TetrisGame *game)
{
    game->bagIndex = 7; // Forces fresh shuffle
    ShuffleBag(game->bag);
    game->bagIndex = 0;

    for (int i = 0; i < PREVIEW_COUNT; i++) {
        game->nextPieces[i] = GetNextPieceFromBag(game);
    }
}

void SpawnPiece(TetrisGame *game)
{
    game->current.type = game->nextPieces[0];
    for (int i = 0; i < PREVIEW_COUNT - 1; i++) {
        game->nextPieces[i] = game->nextPieces[i + 1];
    }
    game->nextPieces[PREVIEW_COUNT - 1] = GetNextPieceFromBag(game);

    game->current.rotation = 0;
    game->current.x = 3;
    game->current.y = 0;
    game->canHold = true;

    UpdateGhostPiece(game);

    // If spawned piece collides immediately, game over!
    if (CheckCollision(&game->board, &game->current, 0, 0, 0)) {
        game->state = STATE_GAME_OVER;
    }
}
