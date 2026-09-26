#include "tetris_game.h"
#include "tetris_board.h"
#include "tetris_piece.h"
#include "tetris_audio.h"
#include <stdio.h>
#include <math.h>

#define DAS_DELAY 0.18f
#define DAS_REPEAT 0.05f

static void DrawBeveledCell(int x, int y, int size, Color color, float alpha)
{
    Color fill = color;
    fill.a = (unsigned char)(color.a * alpha);

    Color light = ColorAlpha(RAYWHITE, 0.45f * alpha);
    Color dark = ColorAlpha(BLACK, 0.40f * alpha);

    // Main cell
    DrawRectangle(x, y, size, size, fill);

    // Bevel highlights
    DrawRectangle(x, y, size, 2, light);
    DrawRectangle(x, y, 2, size, light);
    DrawRectangle(x, y + size - 2, size, 2, dark);
    DrawRectangle(x + size - 2, y, 2, size, dark);
}

void AddTetrisParticles(TetrisGame *game, float x, float y, Color color, int count)
{
    for (int i = 0; i < count; i++) {
        if (game->particleCount >= MAX_PARTICLES) break;
        Particle *p = &game->particles[game->particleCount++];
        p->x = x;
        p->y = y;
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(60, 220);
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->color = color;
        p->maxLife = (float)GetRandomValue(20, 50) / 100.0f;
        p->life = p->maxLife;
        p->size = (float)GetRandomValue(3, 7);
    }
}

void UpdateTetrisParticles(TetrisGame *game, float dt)
{
    for (int i = 0; i < game->particleCount; i++) {
        Particle *p = &game->particles[i];
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->vy += 280.0f * dt; // Gravity
        p->life -= dt;

        if (p->life <= 0.0f) {
            // Remove particle by swapping with last
            game->particles[i] = game->particles[game->particleCount - 1];
            game->particleCount--;
            i--;
        }
    }
}

void DrawTetrisParticles(const TetrisGame *game)
{
    for (int i = 0; i < game->particleCount; i++) {
        const Particle *p = &game->particles[i];
        float alpha = p->life / p->maxLife;
        Color c = ColorAlpha(p->color, alpha);
        DrawRectangle((int)(p->x - p->size * 0.5f), (int)(p->y - p->size * 0.5f), (int)p->size, (int)p->size, c);
    }
}

void ResetGame(TetrisGame *game)
{
    int hs = game->highScore;
    InitBoard(&game->board);
    InitPieceSystem(game);

    game->current.type = PIECE_NONE;
    game->ghostY = 0;
    game->holdPiece = PIECE_NONE;
    game->canHold = true;

    game->score = 0;
    game->highScore = hs;
    game->lines = 0;
    game->level = 1;
    game->combo = 0;
    game->lastClearCount = 0;

    game->dropTimer = 0.0f;
    game->dropInterval = 0.8f;

    game->keyLeftTimer = 0.0f;
    game->keyRightTimer = 0.0f;
    game->keyDownTimer = 0.0f;

    game->particleCount = 0;
    game->state = STATE_PLAYING;

    SpawnPiece(game);
}

void InitGame(TetrisGame *game)
{
    game->highScore = 0;
    game->soundEnabled = true;
    ResetGame(game);
}

static void HandleHoldPiece(TetrisGame *game)
{
    if (!game->canHold) return;

    PieceType currentType = game->current.type;
    if (game->holdPiece == PIECE_NONE) {
        game->holdPiece = currentType;
        SpawnPiece(game);
    } else {
        PieceType temp = game->holdPiece;
        game->holdPiece = currentType;
        game->current.type = temp;
        game->current.rotation = 0;
        game->current.x = 3;
        game->current.y = 0;
        UpdateGhostPiece(game);
    }
    game->canHold = false;
    PlaySoundRotate(game->soundEnabled);
}

void UpdateGame(TetrisGame *game, float dt)
{
    UpdateTetrisParticles(game, dt);

    // Mute sound toggle
    if (IsKeyPressed(KEY_M)) {
        game->soundEnabled = !game->soundEnabled;
    }

    // Game state transitions
    if (game->state == STATE_GAME_OVER) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R) || IsKeyPressed(KEY_SPACE)) {
            ResetGame(game);
        }
        return;
    }

    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
        if (game->state == STATE_PLAYING) {
            game->state = STATE_PAUSED;
        } else if (game->state == STATE_PAUSED) {
            game->state = STATE_PLAYING;
        }
    }

    if (game->state != STATE_PLAYING) return;

    // Handle line clear animation delay
    if (game->board.clearAnimTimer > 0.0f) {
        game->board.clearAnimTimer -= dt;
        if (game->board.clearAnimTimer <= 0.0f) {
            int lines = game->board.linesClearingCount;
            RemoveFullLines(&game->board);

            // Scoring system
            int baseScore = 0;
            switch (lines) {
                case 1: baseScore = 100; break;
                case 2: baseScore = 300; break;
                case 3: baseScore = 500; break;
                case 4: baseScore = 800; break; // TETRIS!
                default: break;
            }
            game->score += baseScore * game->level;
            game->lines += lines;
            game->combo++;

            if (game->combo > 1) {
                game->score += 50 * game->combo * game->level;
            }

            // Level progression
            game->level = 1 + (game->lines / 10);
            game->dropInterval = fmaxf(0.08f, 0.8f - (game->level - 1) * 0.07f);

            if (game->score > game->highScore) {
                game->highScore = game->score;
            }

            SpawnPiece(game);
        }
        return;
    }

    // Input: Hold piece
    if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) {
        HandleHoldPiece(game);
    }

    // Input: Rotations
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_X)) {
        if (TryRotatePiece(game, 1)) {
            PlaySoundRotate(game->soundEnabled);
        }
    }
    if (IsKeyPressed(KEY_Z)) {
        if (TryRotatePiece(game, -1)) {
            PlaySoundRotate(game->soundEnabled);
        }
    }

    // Input: Hard Drop
    if (IsKeyPressed(KEY_SPACE)) {
        int dropDist = game->ghostY - game->current.y;
        game->score += dropDist * 2;
        game->current.y = game->ghostY;
        PlaySoundHardDrop(game->soundEnabled);

        // Spawn particles along the bottom of the piece
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (GetPieceCell(game->current.type, game->current.rotation, r, c)) {
                    float px = BOARD_OFFSET_X + (game->current.x + c) * CELL_SIZE + CELL_SIZE * 0.5f;
                    float py = BOARD_OFFSET_Y + (game->current.y + r + 1) * CELL_SIZE;
                    AddTetrisParticles(game, px, py, GetPieceColor(game->current.type), 6);
                }
            }
        }

        LockCurrentPiece(game);
        game->dropTimer = 0.0f;
        return;
    }

    // Input: Horizontal movement with DAS (Delayed Auto Shift)
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            if (TryMovePiece(game, -1, 0)) PlaySoundMove(game->soundEnabled);
            game->keyLeftTimer = DAS_DELAY;
        } else {
            game->keyLeftTimer -= dt;
            if (game->keyLeftTimer <= 0.0f) {
                if (TryMovePiece(game, -1, 0)) PlaySoundMove(game->soundEnabled);
                game->keyLeftTimer = DAS_REPEAT;
            }
        }
    } else {
        game->keyLeftTimer = 0.0f;
    }

    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            if (TryMovePiece(game, 1, 0)) PlaySoundMove(game->soundEnabled);
            game->keyRightTimer = DAS_DELAY;
        } else {
            game->keyRightTimer -= dt;
            if (game->keyRightTimer <= 0.0f) {
                if (TryMovePiece(game, 1, 0)) PlaySoundMove(game->soundEnabled);
                game->keyRightTimer = DAS_REPEAT;
            }
        }
    } else {
        game->keyRightTimer = 0.0f;
    }

    // Input: Soft Drop
    bool softDrop = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    float currentInterval = softDrop ? fminf(game->dropInterval * 0.15f, 0.05f) : game->dropInterval;

    // Gravity step
    game->dropTimer += dt;
    if (game->dropTimer >= currentInterval) {
        game->dropTimer = 0.0f;
        if (!TryMovePiece(game, 0, 1)) {
            LockCurrentPiece(game);
        } else if (softDrop) {
            game->score += 1;
        }
    }
}

static void DrawMiniPiece(PieceType type, int centerX, int centerY, int cellSize)
{
    if (type == PIECE_NONE) return;

    Color color = GetPieceColor(type);
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (GetPieceCell(type, 0, r, c)) {
                int px = centerX + (c - 2) * cellSize;
                int py = centerY + (r - 2) * cellSize;
                DrawBeveledCell(px, py, cellSize, color, 1.0f);
            }
        }
    }
}

void DrawGame(const TetrisGame *game)
{
    // Background gradient
    ClearBackground((Color){ 14, 17, 24, 255 });

    // Header title
    DrawText("TETRIS", BOARD_OFFSET_X + 70, 12, 34, (Color){ 0, 215, 255, 255 });

    // Board Frame background & glow
    Rectangle boardRect = {
        BOARD_OFFSET_X - 4,
        BOARD_OFFSET_Y - 4,
        BOARD_WIDTH * CELL_SIZE + 8,
        BOARD_HEIGHT * CELL_SIZE + 8
    };
    DrawRectangleRec(boardRect, (Color){ 20, 24, 34, 255 });
    DrawRectangleLinesEx(boardRect, 2.0f, (Color){ 45, 55, 75, 255 });

    // Grid lines
    for (int x = 0; x <= BOARD_WIDTH; x++) {
        int gx = BOARD_OFFSET_X + x * CELL_SIZE;
        DrawLine(gx, BOARD_OFFSET_Y, gx, BOARD_OFFSET_Y + BOARD_HEIGHT * CELL_SIZE, (Color){ 28, 34, 48, 255 });
    }
    for (int y = 0; y <= BOARD_HEIGHT; y++) {
        int gy = BOARD_OFFSET_Y + y * CELL_SIZE;
        DrawLine(BOARD_OFFSET_X, gy, BOARD_OFFSET_X + BOARD_WIDTH * CELL_SIZE, gy, (Color){ 28, 34, 48, 255 });
    }

    // Locked grid cells
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (game->board.grid[y][x] != 0) {
                int px = BOARD_OFFSET_X + x * CELL_SIZE;
                int py = BOARD_OFFSET_Y + y * CELL_SIZE;
                DrawBeveledCell(px, py, CELL_SIZE, game->board.colors[y][x], 1.0f);
            }
        }
    }

    // Line clear animation flash
    if (game->board.clearAnimTimer > 0.0f) {
        float flash = sinf(game->board.clearAnimTimer * 40.0f) * 0.5f + 0.5f;
        Color flashColor = ColorAlpha(WHITE, 0.7f * flash);
        for (int i = 0; i < game->board.linesClearingCount; i++) {
            int row = game->board.linesClearing[i];
            DrawRectangle(BOARD_OFFSET_X, BOARD_OFFSET_Y + row * CELL_SIZE,
                           BOARD_WIDTH * CELL_SIZE, CELL_SIZE, flashColor);
        }
    }

    // Active piece & ghost piece (only when playing or paused)
    if (game->state == STATE_PLAYING || game->state == STATE_PAUSED) {
        // Ghost piece
        if (game->current.type != PIECE_NONE && game->ghostY != game->current.y) {
            Color ghostColor = GetPieceGhostColor(game->current.type);
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    if (GetPieceCell(game->current.type, game->current.rotation, r, c)) {
                        int gx = BOARD_OFFSET_X + (game->current.x + c) * CELL_SIZE;
                        int gy = BOARD_OFFSET_Y + (game->ghostY + r) * CELL_SIZE;
                        if (gy >= BOARD_OFFSET_Y) {
                            DrawRectangle(gx + 1, gy + 1, CELL_SIZE - 2, CELL_SIZE - 2, ghostColor);
                            DrawRectangleLines(gx + 1, gy + 1, CELL_SIZE - 2, CELL_SIZE - 2, ColorAlpha(GetPieceColor(game->current.type), 0.5f));
                        }
                    }
                }
            }
        }

        // Falling current piece
        if (game->current.type != PIECE_NONE) {
            Color pieceColor = GetPieceColor(game->current.type);
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    if (GetPieceCell(game->current.type, game->current.rotation, r, c)) {
                        int px = BOARD_OFFSET_X + (game->current.x + c) * CELL_SIZE;
                        int py = BOARD_OFFSET_Y + (game->current.y + r) * CELL_SIZE;
                        if (py >= BOARD_OFFSET_Y) {
                            DrawBeveledCell(px, py, CELL_SIZE, pieceColor, 1.0f);
                        }
                    }
                }
            }
        }
    }

    // Particles
    DrawTetrisParticles(game);

    // ==========================================
    // LEFT SIDEBAR (HOLD, LEVEL, LINES, COMBO)
    // ==========================================
    int leftX = 40;
    // HOLD Box
    DrawRectangleRounded((Rectangle){ leftX, BOARD_OFFSET_Y, 160, 130 }, 0.1f, 8, (Color){ 22, 28, 40, 255 });
    DrawRectangleRoundedLinesEx((Rectangle){ leftX, BOARD_OFFSET_Y, 160, 130 }, 0.1f, 8, 2.0f, (Color){ 45, 55, 75, 255 });
    DrawText("HOLD", leftX + 55, BOARD_OFFSET_Y + 12, 18, (Color){ 160, 180, 205, 255 });
    if (game->holdPiece != PIECE_NONE) {
        DrawMiniPiece(game->holdPiece, leftX + 85, BOARD_OFFSET_Y + 75, 22);
    }

    // STATS
    int statY = BOARD_OFFSET_Y + 155;
    DrawRectangleRounded((Rectangle){ leftX, statY, 160, 230 }, 0.1f, 8, (Color){ 22, 28, 40, 255 });
    DrawRectangleRoundedLinesEx((Rectangle){ leftX, statY, 160, 230 }, 0.1f, 8, 2.0f, (Color){ 45, 55, 75, 255 });

    DrawText("LEVEL", leftX + 16, statY + 16, 16, (Color){ 140, 160, 185, 255 });
    DrawText(TextFormat("%d", game->level), leftX + 16, statY + 38, 26, (Color){ 0, 215, 255, 255 });

    DrawText("LINES", leftX + 16, statY + 80, 16, (Color){ 140, 160, 185, 255 });
    DrawText(TextFormat("%d", game->lines), leftX + 16, statY + 102, 26, (Color){ 46, 204, 113, 255 });

    DrawText("COMBO", leftX + 16, statY + 144, 16, (Color){ 140, 160, 185, 255 });
    DrawText(TextFormat("x%d", game->combo), leftX + 16, statY + 166, 26, (Color){ 241, 196, 15, 255 });

    // Sound toggle badge
    DrawRectangleRounded((Rectangle){ leftX, statY + 250, 160, 45 }, 0.15f, 6, (Color){ 22, 28, 40, 255 });
    DrawText(TextFormat("SOUND: %s [M]", game->soundEnabled ? "ON" : "OFF"), leftX + 16, statY + 264, 14,
             game->soundEnabled ? (Color){ 46, 204, 113, 255 } : (Color){ 160, 160, 160, 255 });

    // ==========================================
    // RIGHT SIDEBAR (NEXT QUEUE, SCORE, HELP)
    // ==========================================
    int rightX = BOARD_OFFSET_X + BOARD_WIDTH * CELL_SIZE + 40;

    // NEXT Box (3 pieces)
    DrawRectangleRounded((Rectangle){ rightX, BOARD_OFFSET_Y, 160, 260 }, 0.08f, 8, (Color){ 22, 28, 40, 255 });
    DrawRectangleRoundedLinesEx((Rectangle){ rightX, BOARD_OFFSET_Y, 160, 260 }, 0.08f, 8, 2.0f, (Color){ 45, 55, 75, 255 });
    DrawText("NEXT", rightX + 55, BOARD_OFFSET_Y + 12, 18, (Color){ 160, 180, 205, 255 });

    for (int i = 0; i < PREVIEW_COUNT; i++) {
        DrawMiniPiece(game->nextPieces[i], rightX + 85, BOARD_OFFSET_Y + 65 + i * 70, 18);
    }

    // SCORE & HIGH SCORE
    int scoreY = BOARD_OFFSET_Y + 280;
    DrawRectangleRounded((Rectangle){ rightX, scoreY, 160, 145 }, 0.1f, 8, (Color){ 22, 28, 40, 255 });
    DrawRectangleRoundedLinesEx((Rectangle){ rightX, scoreY, 160, 145 }, 0.1f, 8, 2.0f, (Color){ 45, 55, 75, 255 });

    DrawText("SCORE", rightX + 16, scoreY + 14, 15, (Color){ 140, 160, 185, 255 });
    DrawText(TextFormat("%d", game->score), rightX + 16, scoreY + 34, 24, (Color){ 249, 115, 22, 255 });

    DrawText("HIGH SCORE", rightX + 16, scoreY + 76, 14, (Color){ 140, 160, 185, 255 });
    DrawText(TextFormat("%d", game->highScore), rightX + 16, scoreY + 96, 22, (Color){ 241, 196, 15, 255 });

    // CONTROLS BOX
    int helpY = scoreY + 165;
    DrawRectangleRounded((Rectangle){ rightX, helpY, 160, 180 }, 0.1f, 8, (Color){ 22, 28, 40, 255 });
    DrawText("CONTROLS", rightX + 35, helpY + 10, 14, (Color){ 180, 200, 225, 255 });
    DrawText("Left / Right: Move", rightX + 10, helpY + 34, 11, (Color){ 150, 165, 180, 255 });
    DrawText("Up / W / X: CW", rightX + 10, helpY + 54, 11, (Color){ 150, 165, 180, 255 });
    DrawText("Z: CCW Rotate", rightX + 10, helpY + 74, 11, (Color){ 150, 165, 180, 255 });
    DrawText("Down / S: Soft Drop", rightX + 10, helpY + 94, 11, (Color){ 150, 165, 180, 255 });
    DrawText("Space: Hard Drop", rightX + 10, helpY + 114, 11, (Color){ 150, 165, 180, 255 });
    DrawText("C / Shift: Hold", rightX + 10, helpY + 134, 11, (Color){ 150, 165, 180, 255 });
    DrawText("P: Pause | M: Mute", rightX + 10, helpY + 154, 11, (Color){ 150, 165, 180, 255 });

    // Overlays
    if (game->state == STATE_PAUSED) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 10, 12, 18, 200 });
        DrawText("PAUSED", SCREEN_WIDTH / 2 - MeasureText("PAUSED", 48) / 2, SCREEN_HEIGHT / 2 - 40, 48, (Color){ 0, 215, 255, 255 });
        DrawText("Press P or ESC to Resume", SCREEN_WIDTH / 2 - MeasureText("Press P or ESC to Resume", 20) / 2, SCREEN_HEIGHT / 2 + 25, 20, RAYWHITE);
    } else if (game->state == STATE_GAME_OVER) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 20, 8, 12, 220 });
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 52) / 2, SCREEN_HEIGHT / 2 - 70, 52, (Color){ 239, 68, 68, 255 });
        DrawText(TextFormat("Final Score: %d", game->score), SCREEN_WIDTH / 2 - MeasureText(TextFormat("Final Score: %d", game->score), 24) / 2, SCREEN_HEIGHT / 2, 24, RAYWHITE);
        DrawText("Press ENTER or R to Play Again", SCREEN_WIDTH / 2 - MeasureText("Press ENTER or R to Play Again", 18) / 2, SCREEN_HEIGHT / 2 + 50, 18, (Color){ 241, 196, 15, 255 });
    }
}
