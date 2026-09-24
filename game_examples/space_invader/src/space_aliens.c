#include "space_aliens.h"
#include "space_audio.h"
#include "space_game.h"
#include <stdlib.h>
#include <math.h>

// Pixel art bitmaps (1 = pixel, 0 = transparent)
// Squid (8 x 8)
static const unsigned char SQUID_F0[8] = {
    0b00011000,
    0b00111100,
    0b01111110,
    0b11011011,
    0b11111111,
    0b00100100,
    0b01011010,
    0b10100101
};
static const unsigned char SQUID_F1[8] = {
    0b00011000,
    0b00111100,
    0b01111110,
    0b11011011,
    0b11111111,
    0b01011010,
    0b10000001,
    0b01000010
};

// Crab (11 x 8)
static const unsigned short CRAB_F0[8] = {
    0b00100000100,
    0b00010001000,
    0b00111111100,
    0b01101110110,
    0b11111111111,
    0b10111111101,
    0b10100000101,
    0b00011011000
};
static const unsigned short CRAB_F1[8] = {
    0b00100000100,
    0b10010001001,
    0b10111111101,
    0b11101110111,
    0b11111111111,
    0b00111111100,
    0b00100000100,
    0b01000000010
};

// Octopus (12 x 8)
static const unsigned short OCTOPUS_F0[8] = {
    0b000011110000,
    0b011111111110,
    0b111111111111,
    0b111001100111,
    0b111111111111,
    0b000110011000,
    0b001101101100,
    0b110000000011
};
static const unsigned short OCTOPUS_F1[8] = {
    0b000011110000,
    0b011111111110,
    0b111111111111,
    0b111001100111,
    0b111111111111,
    0b001100001100,
    0b011001100110,
    0b001100001100
};

// UFO (16 x 8)
static const unsigned short UFO_SPRITE[8] = {
    0b0000011111100000,
    0b0001111111111000,
    0b0011111111111100,
    0b0110110110110110,
    0b1111111111111111,
    0b0001110001110000,
    0b0011000000001100,
    0b0000000000000000
};

void DrawAlien(AlienType type, int frame, float x, float y, Color color)
{
    const int pxSize = 3;

    if (type == ALIEN_SQUID) {
        const unsigned char *bmp = (frame == 0) ? SQUID_F0 : SQUID_F1;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if ((bmp[r] >> (7 - c)) & 1) {
                    DrawRectangle((int)(x + c * pxSize), (int)(y + r * pxSize), pxSize, pxSize, color);
                }
            }
        }
    } else if (type == ALIEN_CRAB) {
        const unsigned short *bmp = (frame == 0) ? CRAB_F0 : CRAB_F1;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 11; c++) {
                if ((bmp[r] >> (10 - c)) & 1) {
                    DrawRectangle((int)(x + c * pxSize), (int)(y + r * pxSize), pxSize, pxSize, color);
                }
            }
        }
    } else if (type == ALIEN_OCTOPUS) {
        const unsigned short *bmp = (frame == 0) ? OCTOPUS_F0 : OCTOPUS_F1;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 12; c++) {
                if ((bmp[r] >> (11 - c)) & 1) {
                    DrawRectangle((int)(x + c * pxSize), (int)(y + r * pxSize), pxSize, pxSize, color);
                }
            }
        }
    }
}

void DrawAliens(const SpaceGame *game)
{
    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            const SpaceAlien *a = &game->aliens[r][c];
            if (!a->alive) continue;

            Color color = WHITE;
            if (a->type == ALIEN_SQUID) color = (Color){ 241, 196, 15, 255 };    // Yellow/Gold
            else if (a->type == ALIEN_CRAB) color = (Color){ 0, 215, 255, 255 }; // Cyan
            else if (a->type == ALIEN_OCTOPUS) color = (Color){ 231, 76, 60, 255 }; // Crimson Pink

            DrawAlien(a->type, a->animFrame, a->x, a->y, color);
        }
    }
}

void InitAliens(SpaceGame *game)
{
    float startX = 80.0f;
    float startY = 120.0f + (game->wave - 1) * 20.0f; // Each wave starts a bit lower!
    if (startY > 240.0f) startY = 240.0f;

    float spacingX = 52.0f;
    float spacingY = 42.0f;

    for (int r = 0; r < ALIEN_ROWS; r++) {
        AlienType type = ALIEN_OCTOPUS;
        if (r == 0) type = ALIEN_SQUID;
        else if (r == 1 || r == 2) type = ALIEN_CRAB;

        for (int c = 0; c < ALIEN_COLS; c++) {
            SpaceAlien *a = &game->aliens[r][c];
            a->type = type;
            a->alive = true;
            a->animFrame = 0;
            a->x = startX + c * spacingX;
            a->y = startY + r * spacingY;
        }
    }

    game->alienDir = 1;
    game->alienBeatIndex = 0;
    game->aliensRemaining = TOTAL_ALIENS;
    game->alienStepInterval = 0.65f;
    game->alienStepTimer = game->alienStepInterval;
    game->alienShootTimer = 1.2f;
}

void UpdateAliens(SpaceGame *game, float dt)
{
    if (game->aliensRemaining <= 0) return;

    // Calculate step interval based on living aliens (dramatic acceleration)
    float ratio = (float)game->aliensRemaining / (float)TOTAL_ALIENS;
    game->alienStepInterval = 0.05f + 0.60f * (ratio * ratio);

    game->alienStepTimer -= dt;
    if (game->alienStepTimer <= 0.0f) {
        game->alienStepTimer = game->alienStepInterval;

        // Check if any alien will hit screen edge
        bool willHitEdge = false;
        float shiftAmount = 14.0f * game->alienDir;

        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (game->aliens[r][c].alive) {
                    float nextX = game->aliens[r][c].x + shiftAmount;
                    if (game->alienDir > 0 && nextX > SCREEN_WIDTH - 65.0f) {
                        willHitEdge = true;
                        break;
                    } else if (game->alienDir < 0 && nextX < 30.0f) {
                        willHitEdge = true;
                        break;
                    }
                }
            }
            if (willHitEdge) break;
        }

        if (willHitEdge) {
            // Reverse direction and step downwards
            game->alienDir = -game->alienDir;
            for (int r = 0; r < ALIEN_ROWS; r++) {
                for (int c = 0; c < ALIEN_COLS; c++) {
                    if (game->aliens[r][c].alive) {
                        game->aliens[r][c].y += 18.0f;
                        game->aliens[r][c].animFrame = 1 - game->aliens[r][c].animFrame;

                        // Check if invaders reached bottom / player line!
                        if (game->aliens[r][c].y >= game->player.y - 20.0f) {
                            game->state = SPACE_STATE_GAME_OVER;
                        }
                    }
                }
            }
        } else {
            // Step horizontally
            for (int r = 0; r < ALIEN_ROWS; r++) {
                for (int c = 0; c < ALIEN_COLS; c++) {
                    if (game->aliens[r][c].alive) {
                        game->aliens[r][c].x += shiftAmount;
                        game->aliens[r][c].animFrame = 1 - game->aliens[r][c].animFrame;
                    }
                }
            }
        }

        // Play 4-beat march rhythm
        PlaySoundAlienMarch(game->alienBeatIndex, game->soundEnabled);
        game->alienBeatIndex = (game->alienBeatIndex + 1) % 4;
    }

    // Alien Shooting
    game->alienShootTimer -= dt;
    if (game->alienShootTimer <= 0.0f) {
        float minInterval = 0.5f + 0.9f * ((float)game->aliensRemaining / TOTAL_ALIENS);
        game->alienShootTimer = (float)GetRandomValue((int)(minInterval * 100), (int)((minInterval + 0.8f) * 100)) / 100.0f;

        // Pick bottom-most alien in a random active column
        int activeCols[ALIEN_COLS];
        int activeCount = 0;
        for (int c = 0; c < ALIEN_COLS; c++) {
            for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
                if (game->aliens[r][c].alive) {
                    activeCols[activeCount++] = c;
                    break;
                }
            }
        }

        if (activeCount > 0) {
            int chosenCol = activeCols[GetRandomValue(0, activeCount - 1)];
            // Find lowest alien in that column
            for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
                if (game->aliens[r][chosenCol].alive) {
                    // Spawn alien bomb
                    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
                        if (!game->alienBullets[i].active) {
                            game->alienBullets[i].active = true;
                            game->alienBullets[i].isPlayer = false;
                            game->alienBullets[i].x = game->aliens[r][chosenCol].x + 18.0f;
                            game->alienBullets[i].y = game->aliens[r][chosenCol].y + 26.0f;
                            game->alienBullets[i].vy = 260.0f + game->wave * 20.0f;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

void InitUfo(SpaceUfo *ufo)
{
    ufo->active = false;
    ufo->x = -70.0f;
    ufo->y = 65.0f;
    ufo->speed = 180.0f;
    ufo->scoreValue = 100;
    ufo->spawnTimer = (float)GetRandomValue(15, 25);
}

void UpdateUfo(SpaceGame *game, float dt)
{
    SpaceUfo *u = &game->ufo;

    if (!u->active) {
        u->spawnTimer -= dt;
        if (u->spawnTimer <= 0.0f) {
            u->active = true;
            u->spawnTimer = (float)GetRandomValue(20, 35);
            int dir = GetRandomValue(0, 1);
            if (dir == 0) {
                u->x = -60.0f;
                u->speed = 170.0f;
            } else {
                u->x = SCREEN_WIDTH + 10.0f;
                u->speed = -170.0f;
            }
            const int bonusScores[] = { 50, 100, 150, 200, 300 };
            u->scoreValue = bonusScores[GetRandomValue(0, 4)];
            PlaySoundUfo(game->soundEnabled);
        }
    } else {
        u->x += u->speed * dt;
        if ((u->speed > 0 && u->x > SCREEN_WIDTH + 50.0f) ||
            (u->speed < 0 && u->x < -70.0f)) {
            u->active = false;
        }
    }
}

void DrawUfo(const SpaceUfo *ufo)
{
    if (!ufo->active) return;

    const int pxSize = 3;
    Color red = (Color){ 239, 68, 68, 255 };

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 16; c++) {
            if ((UFO_SPRITE[r] >> (15 - c)) & 1) {
                DrawRectangle((int)(ufo->x + c * pxSize), (int)(ufo->y + r * pxSize), pxSize, pxSize, red);
            }
        }
    }
}
