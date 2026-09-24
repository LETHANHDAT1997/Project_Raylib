#include "space_game.h"
#include "space_player.h"
#include "space_aliens.h"
#include "space_bunker.h"
#include "space_audio.h"
#include <stdio.h>
#include <math.h>

void AddSpaceParticles(SpaceGame *game, float x, float y, Color color, int count)
{
    for (int i = 0; i < count; i++) {
        if (game->particleCount >= MAX_PARTICLES) break;
        SpaceParticle *p = &game->particles[game->particleCount++];
        p->x = x;
        p->y = y;
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(50, 240);
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->color = color;
        p->maxLife = (float)GetRandomValue(20, 50) / 100.0f;
        p->life = p->maxLife;
        p->size = (float)GetRandomValue(3, 6);
    }
}

void UpdateSpaceParticles(SpaceGame *game, float dt)
{
    for (int i = 0; i < game->particleCount; i++) {
        SpaceParticle *p = &game->particles[i];
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->life -= dt;

        if (p->life <= 0.0f) {
            game->particles[i] = game->particles[game->particleCount - 1];
            game->particleCount--;
            i--;
        }
    }
}

void DrawSpaceParticles(const SpaceGame *game)
{
    for (int i = 0; i < game->particleCount; i++) {
        const SpaceParticle *p = &game->particles[i];
        float alpha = p->life / p->maxLife;
        Color c = ColorAlpha(p->color, alpha);
        DrawRectangle((int)(p->x - p->size * 0.5f), (int)(p->y - p->size * 0.5f), (int)p->size, (int)p->size, c);
    }
}

static void InitStars(SpaceStar stars[MAX_STARS])
{
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = (float)GetRandomValue(0, SCREEN_WIDTH);
        stars[i].y = (float)GetRandomValue(0, SCREEN_HEIGHT);
        stars[i].speed = (float)GetRandomValue(20, 90);
        stars[i].brightness = (float)GetRandomValue(40, 100) / 100.0f;
    }
}

static void UpdateStars(SpaceStar stars[MAX_STARS], float dt)
{
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].y += stars[i].speed * dt;
        if (stars[i].y > SCREEN_HEIGHT) {
            stars[i].y = 0.0f;
            stars[i].x = (float)GetRandomValue(0, SCREEN_WIDTH);
        }
    }
}

static void DrawStars(const SpaceStar stars[MAX_STARS])
{
    for (int i = 0; i < MAX_STARS; i++) {
        Color starCol = ColorAlpha(RAYWHITE, stars[i].brightness * 0.5f);
        DrawPixel((int)stars[i].x, (int)stars[i].y, starCol);
    }
}

void ResetSpaceGame(SpaceGame *game, bool resetScore)
{
    if (resetScore) {
        game->score = 0;
        game->wave = 1;
    }
    InitPlayer(&game->player);

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) game->playerBullets[i].active = false;
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) game->alienBullets[i].active = false;

    InitAliens(game);
    InitBunkers(game->bunkers);
    InitUfo(&game->ufo);

    game->particleCount = 0;
    game->state = SPACE_STATE_PLAYING;
    game->stateTimer = 0.0f;
}

void StartNextWave(SpaceGame *game)
{
    game->wave++;
    InitAliens(game);
    InitBunkers(game->bunkers);
    InitUfo(&game->ufo);

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) game->playerBullets[i].active = false;
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) game->alienBullets[i].active = false;

    game->state = SPACE_STATE_PLAYING;
    game->stateTimer = 0.0f;
}

void InitSpaceGame(SpaceGame *game)
{
    game->score = 0;
    game->highScore = 0;
    game->wave = 1;
    game->soundEnabled = true;
    game->state = SPACE_STATE_MENU;
    InitStars(game->stars);
    ResetSpaceGame(game, true);
    game->state = SPACE_STATE_MENU; // Ensure menu is initial state
}

void UpdateSpaceGame(SpaceGame *game, float dt)
{
    UpdateStars(game->stars, dt);
    UpdateSpaceParticles(game, dt);

    // Mute toggle
    if (IsKeyPressed(KEY_M)) {
        game->soundEnabled = !game->soundEnabled;
    }

    if (game->state == SPACE_STATE_MENU) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            ResetSpaceGame(game, true);
        }
        return;
    }

    if (game->state == SPACE_STATE_GAME_OVER) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R) || IsKeyPressed(KEY_SPACE)) {
            ResetSpaceGame(game, true);
        }
        return;
    }

    if (game->state == SPACE_STATE_WAVE_CLEAR) {
        game->stateTimer += dt;
        if (game->stateTimer >= 1.5f) {
            StartNextWave(game);
        }
        return;
    }

    // STATE_PLAYING
    UpdatePlayer(game, dt);
    UpdateAliens(game, dt);
    UpdateUfo(game, dt);

    // Update Player Bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        SpaceBullet *b = &game->playerBullets[i];
        if (!b->active) continue;

        b->y += b->vy * dt;
        if (b->y < 35.0f) {
            b->active = false;
            continue;
        }

        // Bullet vs UFO
        if (game->ufo.active) {
            if (b->x >= game->ufo.x && b->x <= game->ufo.x + 48.0f &&
                b->y >= game->ufo.y && b->y <= game->ufo.y + 24.0f) {
                b->active = false;
                game->ufo.active = false;
                game->score += game->ufo.scoreValue;
                if (game->score > game->highScore) game->highScore = game->score;
                AddSpaceParticles(game, game->ufo.x + 24.0f, game->ufo.y + 12.0f, (Color){ 239, 68, 68, 255 }, 25);
                PlaySoundAlienHit(game->soundEnabled);
                continue;
            }
        }

        // Bullet vs Bunker
        if (CheckBunkerCollision(game->bunkers, b->x, b->y, 4.0f, game)) {
            b->active = false;
            continue;
        }

        // Bullet vs Alien Bullets (cancel each other!)
        for (int j = 0; j < MAX_ALIEN_BULLETS; j++) {
            SpaceBullet *ab = &game->alienBullets[j];
            if (ab->active && fabsf(b->x - ab->x) < 8.0f && fabsf(b->y - ab->y) < 12.0f) {
                b->active = false;
                ab->active = false;
                AddSpaceParticles(game, b->x, b->y, RAYWHITE, 6);
                break;
            }
        }
        if (!b->active) continue;

        // Bullet vs Aliens
        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                SpaceAlien *a = &game->aliens[r][c];
                if (!a->alive) continue;

                float alienW = 36.0f;
                float alienH = 24.0f;

                if (b->x >= a->x && b->x <= a->x + alienW &&
                    b->y >= a->y && b->y <= a->y + alienH) {
                    a->alive = false;
                    b->active = false;
                    game->aliensRemaining--;

                    int points = 10;
                    Color partColor = (Color){ 231, 76, 60, 255 };
                    if (a->type == ALIEN_SQUID) {
                        points = 30;
                        partColor = (Color){ 241, 196, 15, 255 };
                    } else if (a->type == ALIEN_CRAB) {
                        points = 20;
                        partColor = (Color){ 0, 215, 255, 255 };
                    }

                    game->score += points;
                    if (game->score > game->highScore) game->highScore = game->score;

                    AddSpaceParticles(game, a->x + alienW * 0.5f, a->y + alienH * 0.5f, partColor, 16);
                    PlaySoundAlienHit(game->soundEnabled);

                    if (game->aliensRemaining <= 0) {
                        game->state = SPACE_STATE_WAVE_CLEAR;
                        game->stateTimer = 0.0f;
                    }
                    goto next_player_bullet;
                }
            }
        }
        next_player_bullet:;
    }

    // Update Alien Bullets
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        SpaceBullet *b = &game->alienBullets[i];
        if (!b->active) continue;

        b->y += b->vy * dt;
        if (b->y > SCREEN_HEIGHT - 40.0f) {
            b->active = false;
            continue;
        }

        // Alien Bullet vs Bunker
        if (CheckBunkerCollision(game->bunkers, b->x, b->y, 4.0f, game)) {
            b->active = false;
            continue;
        }

        // Alien Bullet vs Player
        SpacePlayer *p = &game->player;
        if (p->isAlive && p->hitTimer <= 0.0f) {
            if (b->x >= p->x && b->x <= p->x + p->width &&
                b->y >= p->y && b->y <= p->y + p->height) {
                b->active = false;
                p->lives--;
                p->hitTimer = 2.0f;

                AddSpaceParticles(game, p->x + p->width * 0.5f, p->y + p->height * 0.5f, (Color){ 46, 204, 113, 255 }, 25);
                PlaySoundPlayerHit(game->soundEnabled);

                if (p->lives <= 0) {
                    p->isAlive = false;
                    game->state = SPACE_STATE_GAME_OVER;
                }
            }
        }
    }
}

void DrawSpaceGame(const SpaceGame *game)
{
    ClearBackground((Color){ 10, 10, 16, 255 });

    DrawStars(game->stars);

    // ==========================================
    // TOP HUD
    // ==========================================
    DrawText("SCORE < 1 >", 60, 16, 16, (Color){ 170, 185, 205, 255 });
    DrawText(TextFormat("%05d", game->score), 60, 36, 22, (Color){ 46, 204, 113, 255 });

    DrawText("HI-SCORE", SCREEN_WIDTH / 2 - MeasureText("HI-SCORE", 16) / 2, 16, 16, (Color){ 170, 185, 205, 255 });
    DrawText(TextFormat("%05d", game->highScore), SCREEN_WIDTH / 2 - MeasureText(TextFormat("%05d", game->highScore), 22) / 2, 36, 22, (Color){ 241, 196, 15, 255 });

    DrawText("WAVE", SCREEN_WIDTH - 150, 16, 16, (Color){ 170, 185, 205, 255 });
    DrawText(TextFormat("%d", game->wave), SCREEN_WIDTH - 150, 36, 22, (Color){ 0, 215, 255, 255 });

    // Boundary Line Top
    DrawLine(30, 60, SCREEN_WIDTH - 30, 60, (Color){ 30, 35, 50, 255 });

    // UFO
    DrawUfo(&game->ufo);

    // Aliens
    DrawAliens(game);

    // Destructible Bunkers
    DrawBunkers(game->bunkers);

    // Player Bullets (bright green neon laser)
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (game->playerBullets[i].active) {
            float bx = game->playerBullets[i].x;
            float by = game->playerBullets[i].y;
            DrawRectangle((int)(bx - 2), (int)(by - 10), 4, 14, (Color){ 46, 204, 113, 255 });
            DrawRectangle((int)(bx - 1), (int)(by - 8), 2, 10, WHITE);
        }
    }

    // Alien Bullets (zig-zag / pulsing crimson missile)
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (game->alienBullets[i].active) {
            float bx = game->alienBullets[i].x;
            float by = game->alienBullets[i].y;
            DrawRectangle((int)(bx - 2), (int)(by - 6), 4, 12, (Color){ 239, 68, 68, 255 });
            DrawRectangle((int)(bx - 1), (int)(by - 4), 2, 8, (Color){ 254, 202, 202, 255 });
        }
    }

    // Player
    DrawPlayer(&game->player);

    // Particles
    DrawSpaceParticles(game);

    // ==========================================
    // BOTTOM BAR & LIVES
    // ==========================================
    int bottomY = SCREEN_HEIGHT - 45;
    DrawLine(30, bottomY - 10, SCREEN_WIDTH - 30, bottomY - 10, (Color){ 46, 204, 113, 255 });

    DrawText("LIVES:", 40, bottomY, 14, (Color){ 160, 180, 200, 255 });
    DrawLives(game->player.lives, 100, bottomY);

    DrawText(TextFormat("SOUND: %s [M]", game->soundEnabled ? "ON" : "OFF"), SCREEN_WIDTH - 180, bottomY, 14,
             game->soundEnabled ? (Color){ 46, 204, 113, 255 } : (Color){ 140, 140, 140, 255 });

    DrawText("Move: [A/D or Arrows]  Fire: [SPACE / W]", SCREEN_WIDTH / 2 - 130, bottomY, 13, (Color){ 120, 140, 160, 255 });

    // ==========================================
    // CRT SCANLINE OVERLAY
    // ==========================================
    for (int y = 0; y < SCREEN_HEIGHT; y += 3) {
        DrawLine(0, y, SCREEN_WIDTH, y, (Color){ 0, 0, 0, 18 });
    }

    // ==========================================
    // OVERLAYS (MENU, GAME OVER, WAVE CLEAR)
    // ==========================================
    if (game->state == SPACE_STATE_MENU) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 8, 10, 16, 235 });

        int titleY = 120;
        DrawText("SPACE INVADERS", SCREEN_WIDTH / 2 - MeasureText("SPACE INVADERS", 44) / 2, titleY, 44, (Color){ 46, 204, 113, 255 });
        DrawText("RAYLIB RETRO EDITION", SCREEN_WIDTH / 2 - MeasureText("RAYLIB RETRO EDITION", 18) / 2, titleY + 52, 18, (Color){ 0, 215, 255, 255 });

        int tableY = 240;
        DrawText("* SCORE ADVANCE TABLE *", SCREEN_WIDTH / 2 - MeasureText("* SCORE ADVANCE TABLE *", 20) / 2, tableY, 20, RAYWHITE);

        // Score Table Items
        DrawUfo(&(SpaceUfo){ .active = true, .x = SCREEN_WIDTH / 2 - 110, .y = tableY + 50 });
        DrawText("=  ?  MYSTERY", SCREEN_WIDTH / 2 - 30, tableY + 52, 18, (Color){ 239, 68, 68, 255 });

        DrawAlien(ALIEN_SQUID, 0, SCREEN_WIDTH / 2 - 100, tableY + 95, (Color){ 241, 196, 15, 255 });
        DrawText("=  30 POINTS", SCREEN_WIDTH / 2 - 30, tableY + 98, 18, (Color){ 241, 196, 15, 255 });

        DrawAlien(ALIEN_CRAB, 0, SCREEN_WIDTH / 2 - 105, tableY + 140, (Color){ 0, 215, 255, 255 });
        DrawText("=  20 POINTS", SCREEN_WIDTH / 2 - 30, tableY + 144, 18, (Color){ 0, 215, 255, 255 });

        DrawAlien(ALIEN_OCTOPUS, 0, SCREEN_WIDTH / 2 - 107, tableY + 185, (Color){ 231, 76, 60, 255 });
        DrawText("=  10 POINTS", SCREEN_WIDTH / 2 - 30, tableY + 190, 18, (Color){ 231, 76, 60, 255 });

        // Bunker info
        DrawText("4 Destructible Shields protect your base", SCREEN_WIDTH / 2 - MeasureText("4 Destructible Shields protect your base", 16) / 2, tableY + 250, 16, (Color){ 160, 180, 205, 255 });

        // Start Prompt
        float blink = sinf((float)GetTime() * 5.0f);
        if (blink > 0.0f) {
            DrawText("PRESS SPACE OR ENTER TO PLAY", SCREEN_WIDTH / 2 - MeasureText("PRESS SPACE OR ENTER TO PLAY", 22) / 2, tableY + 320, 22, (Color){ 241, 196, 15, 255 });
        }
    } else if (game->state == SPACE_STATE_GAME_OVER) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 15, 6, 8, 230 });
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 48) / 2, SCREEN_HEIGHT / 2 - 80, 48, (Color){ 239, 68, 68, 255 });
        DrawText(TextFormat("Final Score: %d", game->score), SCREEN_WIDTH / 2 - MeasureText(TextFormat("Final Score: %d", game->score), 24) / 2, SCREEN_HEIGHT / 2 - 10, 24, RAYWHITE);
        DrawText(TextFormat("High Score: %d", game->highScore), SCREEN_WIDTH / 2 - MeasureText(TextFormat("High Score: %d", game->highScore), 20) / 2, SCREEN_HEIGHT / 2 + 30, 20, (Color){ 241, 196, 15, 255 });
        DrawText("Press ENTER or R to Play Again", SCREEN_WIDTH / 2 - MeasureText("Press ENTER or R to Play Again", 20) / 2, SCREEN_HEIGHT / 2 + 80, 20, (Color){ 0, 215, 255, 255 });
    } else if (game->state == SPACE_STATE_WAVE_CLEAR) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 8, 20, 14, 180 });
        DrawText("WAVE CLEARED!", SCREEN_WIDTH / 2 - MeasureText("WAVE CLEARED!", 42) / 2, SCREEN_HEIGHT / 2 - 30, 42, (Color){ 46, 204, 113, 255 });
        DrawText("Prepare for next assault...", SCREEN_WIDTH / 2 - MeasureText("Prepare for next assault...", 20) / 2, SCREEN_HEIGHT / 2 + 25, 20, RAYWHITE);
    }
}
