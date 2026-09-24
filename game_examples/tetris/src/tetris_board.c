#include "tetris_board.h"
#include "tetris_piece.h"
#include "tetris_game.h"
#include "tetris_audio.h"
#include <string.h>

void InitBoard(TetrisBoard *board)
{
    memset(board->grid, 0, sizeof(board->grid));
    board->linesClearingCount = 0;
    board->clearAnimTimer = 0.0f;
}

bool CheckCollision(const TetrisBoard *board, const TetrisPiece *piece, int offsetX, int offsetY, int testRotation)
{
    if (piece->type == PIECE_NONE) return false;

    int rot = (testRotation % 4 + 4) % 4;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (GetPieceCell(piece->type, rot, r, c)) {
                int boardX = piece->x + c + offsetX;
                int boardY = piece->y + r + offsetY;

                // Check out of board horizontal bounds
                if (boardX < 0 || boardX >= BOARD_WIDTH) return true;

                // Check below board bottom
                if (boardY >= BOARD_HEIGHT) return true;

                // Check against existing blocks (only if inside visible board)
                if (boardY >= 0 && board->grid[boardY][boardX] != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

void UpdateGhostPiece(TetrisGame *game)
{
    if (game->current.type == PIECE_NONE) return;

    int ghostY = game->current.y;
    while (!CheckCollision(&game->board, &game->current, 0, (ghostY - game->current.y) + 1, game->current.rotation)) {
        ghostY++;
    }
    game->ghostY = ghostY;
}

bool TryMovePiece(TetrisGame *game, int dx, int dy)
{
    if (!CheckCollision(&game->board, &game->current, dx, dy, game->current.rotation)) {
        game->current.x += dx;
        game->current.y += dy;
        UpdateGhostPiece(game);
        return true;
    }
    return false;
}

bool TryRotatePiece(TetrisGame *game, int dir)
{
    int newRot = (game->current.rotation + dir + 4) % 4;

    // Standard wall kicks offsets to test
    const int kickOffsets[][2] = {
        {0, 0},   // Basic rotation
        {-1, 0},  // Shift left 1
        {1, 0},   // Shift right 1
        {0, -1},  // Shift up 1
        {-2, 0},  // Shift left 2 (for I-piece)
        {2, 0},   // Shift right 2
        {0, -2}   // Shift up 2
    };

    int tests = (game->current.type == PIECE_I) ? 7 : 4;

    for (int i = 0; i < tests; i++) {
        int ox = kickOffsets[i][0];
        int oy = kickOffsets[i][1];
        if (!CheckCollision(&game->board, &game->current, ox, oy, newRot)) {
            game->current.x += ox;
            game->current.y += oy;
            game->current.rotation = newRot;
            UpdateGhostPiece(game);
            return true;
        }
    }

    return false;
}

void LockCurrentPiece(TetrisGame *game)
{
    Color pieceColor = GetPieceColor(game->current.type);

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (GetPieceCell(game->current.type, game->current.rotation, r, c)) {
                int bx = game->current.x + c;
                int by = game->current.y + r;
                if (bx >= 0 && bx < BOARD_WIDTH && by >= 0 && by < BOARD_HEIGHT) {
                    game->board.grid[by][bx] = (int)game->current.type;
                    game->board.colors[by][bx] = pieceColor;

                    // Particle on impact
                    float px = BOARD_OFFSET_X + bx * CELL_SIZE + CELL_SIZE * 0.5f;
                    float py = BOARD_OFFSET_Y + by * CELL_SIZE + CELL_SIZE * 0.5f;
                    AddParticles(game, px, py, pieceColor, 4);
                }
            }
        }
    }

    // Check for completed lines
    int cleared = FindFullLines(&game->board);
    if (cleared > 0) {
        game->board.clearAnimTimer = 0.22f; // Trigger flash animation
        PlaySoundLineClear(cleared, game->soundEnabled);
    } else {
        PlaySoundDrop(game->soundEnabled);
        SpawnPiece(game);
    }
}

int FindFullLines(TetrisBoard *board)
{
    board->linesClearingCount = 0;

    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board->grid[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            board->linesClearing[board->linesClearingCount++] = y;
        }
    }
    return board->linesClearingCount;
}

void RemoveFullLines(TetrisBoard *board)
{
    if (board->linesClearingCount == 0) return;

    for (int i = 0; i < board->linesClearingCount; i++) {
        int clearedRow = board->linesClearing[i];

        // Shift everything above down
        for (int y = clearedRow; y > 0; y--) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board->grid[y][x] = board->grid[y - 1][x];
                board->colors[y][x] = board->colors[y - 1][x];
            }
        }

        // Top row becomes empty
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board->grid[0][x] = 0;
        }

        // Adjust remaining indices in linesClearing because rows shifted down
        for (int j = i + 1; j < board->linesClearingCount; j++) {
            if (board->linesClearing[j] < clearedRow) {
                board->linesClearing[j]++;
            }
        }
    }

    board->linesClearingCount = 0;
}
