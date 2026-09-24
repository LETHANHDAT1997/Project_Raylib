#include "space_player.h"
#include "space_audio.h"
#include <math.h>

void InitPlayer(SpacePlayer *player)
{
    player->width = 44;
    player->height = 24;
    player->x = SCREEN_WIDTH * 0.5f - player->width * 0.5f;
    player->y = SCREEN_HEIGHT - 90;
    player->speed = 340.0f;
    player->lives = 3;
    player->hitTimer = 0.0f;
    player->isAlive = true;
    player->shootCooldown = 0.0f;
}

void UpdatePlayer(SpaceGame *game, float dt)
{
    SpacePlayer *p = &game->player;

    if (!p->isAlive) return;

    if (p->hitTimer > 0.0f) {
        p->hitTimer -= dt;
    }

    if (p->shootCooldown > 0.0f) {
        p->shootCooldown -= dt;
    }

    // Movement
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        p->x -= p->speed * dt;
        if (p->x < 24.0f) p->x = 24.0f;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        p->x += p->speed * dt;
        if (p->x > SCREEN_WIDTH - p->width - 24.0f) {
            p->x = SCREEN_WIDTH - p->width - 24.0f;
        }
    }

    // Shooting (Hỗ trợ nhấn giữ phím để tự động bắn liên tục)
    if ((IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) && p->shootCooldown <= 0.0f) {
        // Find inactive player bullet slot
        for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
            if (!game->playerBullets[i].active) {
                game->playerBullets[i].active = true;
                game->playerBullets[i].isPlayer = true;
                game->playerBullets[i].x = p->x + p->width * 0.5f;
                game->playerBullets[i].y = p->y - 6.0f;
                game->playerBullets[i].vy = -650.0f;
                p->shootCooldown = 0.18f; // Nhịp bắn khi nhấn giữ (~5.5 viên / giây)
                PlaySoundLaser(game->soundEnabled);
                break;
            }
        }
    }
}

void DrawPlayer(const SpacePlayer *player)
{
    if (!player->isAlive) return;

    // Flashing effect if recently hit
    if (player->hitTimer > 0.0f) {
        if ((int)(player->hitTimer * 15.0f) % 2 == 0) return;
    }

    Color green = (Color){ 46, 204, 113, 255 };
    Color darkGreen = (Color){ 39, 174, 96, 255 };

    float px = player->x;
    float py = player->y;

    // Cannon Turret
    DrawRectangle((int)(px + 19), (int)(py - 6), 6, 8, green);
    DrawRectangle((int)(px + 17), (int)(py + 2), 10, 6, green);

    // Main Body
    DrawRectangle((int)(px + 4), (int)(py + 8), 36, 10, green);

    // Tread Base
    DrawRectangle((int)px, (int)(py + 18), player->width, 6, darkGreen);
    DrawRectangle((int)(px + 2), (int)(py + 22), 4, 3, green);
    DrawRectangle((int)(px + player->width - 6), (int)(py + 22), 4, 3, green);
}

void DrawLives(int lives, int startX, int y)
{
    Color green = (Color){ 46, 204, 113, 255 };
    for (int i = 0; i < lives; i++) {
        int tx = startX + i * 36;
        // Mini tank icon
        DrawRectangle(tx + 9, y - 3, 4, 4, green);
        DrawRectangle(tx + 2, y + 1, 18, 6, green);
        DrawRectangle(tx, y + 7, 22, 4, green);
    }
}
