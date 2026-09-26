// ============================================================================
// FIGHTER GAME - Logic chính của game đối kháng
// ============================================================================
#include "raylib.h"
#include "raymath.h"
#include "fighter_types.h"
#include "fighter_game.h"
#include "fighter_entities.h"
#include "fighter_draw.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
static void HandleInput_P1(FighterGame *game, float dt);
static void HandleInput_P2(FighterGame *game, float dt);
static void HandleAI(FighterGame *game, float dt);
static void CheckCollisions(FighterGame *game);
static void CheckProjectileCollisions(FighterGame *game);
static void StartRound(FighterGame *game);
static void EndRound(FighterGame *game);

// ============================================================================
// KHỞI TẠO GAME
// ============================================================================
void InitFighterGame(FighterGame *game)
{
    memset(game, 0, sizeof(FighterGame));
    game->state = GAME_STATE_TITLE;
    game->stateTimer = 0;
    game->currentRound = 1;
    game->roundTimer = ROUND_TIME;
    game->p2IsAI = true;
    game->aiReactionTime = 0.3f;
    game->aiThinkTimer = 0;
    game->hitPauseTimer = 0;
    game->slowMotion = 1.0f;
    game->screenShake = 0;
    game->shakeOffset = (Vector2){0, 0};
    game->bgScrollX = 0;
    game->cloudOffset = 0;

    InitFighter(&game->player1, CHAR_RYU, 250.0f, true);
    InitFighter(&game->player2, CHAR_KEN, 774.0f, false);
}

void CloseFighterGame(FighterGame *game)
{
    (void)game;
}

// ============================================================================
// BẮT ĐẦU ROUND MỚI
// ============================================================================
static void StartRound(FighterGame *game)
{
    game->roundTimer = ROUND_TIME;
    game->hitPauseTimer = 0;
    game->slowMotion = 1.0f;
    game->screenShake = 0;

    // Reset fighters nhưng giữ rounds won
    int p1Wins = game->player1.roundsWon;
    int p2Wins = game->player2.roundsWon;
    InitFighter(&game->player1, CHAR_RYU, 250.0f, true);
    InitFighter(&game->player2, CHAR_KEN, 774.0f, false);
    game->player1.roundsWon = p1Wins;
    game->player2.roundsWon = p2Wins;

    // Clear projectiles & particles
    for (int i = 0; i < MAX_PROJECTILES; i++) game->projectiles[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) game->particles[i].active = false;
}

// ============================================================================
// KẾT THÚC ROUND
// ============================================================================
static void EndRound(FighterGame *game)
{
    // Xác định người thắng round
    if (game->player1.health <= 0) {
        game->player2.roundsWon++;
        FighterSetState(&game->player1, FIGHTER_DEFEAT);
        FighterSetState(&game->player2, FIGHTER_VICTORY);
    } else if (game->player2.health <= 0) {
        game->player1.roundsWon++;
        FighterSetState(&game->player1, FIGHTER_VICTORY);
        FighterSetState(&game->player2, FIGHTER_DEFEAT);
    } else {
        // Hết giờ - ai nhiều máu hơn thắng
        if (game->player1.health >= game->player2.health) {
            game->player1.roundsWon++;
            FighterSetState(&game->player1, FIGHTER_VICTORY);
            FighterSetState(&game->player2, FIGHTER_DEFEAT);
        } else {
            game->player2.roundsWon++;
            FighterSetState(&game->player1, FIGHTER_DEFEAT);
            FighterSetState(&game->player2, FIGHTER_VICTORY);
        }
    }

    game->state = GAME_STATE_ROUND_END;
    game->stateTimer = 0;
    game->slowMotion = 0.3f;
    game->screenShake = 15.0f;
}

// ============================================================================
// XỬ LÝ INPUT PLAYER 1 (WASD + JKL)
// ============================================================================
static void HandleInput_P1(FighterGame *game, float dt)
{
    Fighter *f = &game->player1;
    if (f->hitStun > 0 || f->blockStun > 0) return;
    if (f->state == FIGHTER_KNOCKDOWN || f->state == FIGHTER_VICTORY || f->state == FIGHTER_DEFEAT) return;
    if (f->state == FIGHTER_PUNCH || f->state == FIGHTER_KICK || f->state == FIGHTER_SPECIAL) return;

    // Block
    if (IsKeyDown(KEY_SPACE)) {
        if (f->state != FIGHTER_BLOCK && f->onGround) {
            FighterSetState(f, FIGHTER_BLOCK);
        }
        return;
    } else if (f->state == FIGHTER_BLOCK) {
        FighterSetState(f, FIGHTER_IDLE);
    }

    // Special move (L key) - Hadouken
    if (IsKeyPressed(KEY_L) && f->specialCooldown <= 0 && f->superMeter >= 20.0f) {
        FighterSetState(f, FIGHTER_SPECIAL);
        f->specialCooldown = 1.0f;
        f->superMeter -= 20.0f;

        // Spawn projectile after small delay (handled in update)
        float dir = f->facingRight ? 1.0f : -1.0f;
        Vector2 projPos = {f->position.x + dir * 40, f->position.y - 80};
        Vector2 projVel = {dir * 500.0f, 0};
        FighterSpawnProjectile(game, 0, projPos, projVel, 80, (Color){100, 180, 255, 255});
        FighterSpawnHitSparks(game, projPos, 8);
        return;
    }

    // Punch (J key)
    if (IsKeyPressed(KEY_J)) {
        FighterSetState(f, FIGHTER_PUNCH);
        f->superMeter += 3.0f;
        if (f->superMeter > 100.0f) f->superMeter = 100.0f;
        return;
    }

    // Kick (K key)
    if (IsKeyPressed(KEY_K)) {
        FighterSetState(f, FIGHTER_KICK);
        f->superMeter += 4.0f;
        if (f->superMeter > 100.0f) f->superMeter = 100.0f;
        return;
    }

    // Jump (W key)
    if (IsKeyPressed(KEY_W) && f->onGround) {
        FighterSetState(f, FIGHTER_JUMP);
        f->velocity.y = -650.0f;
        f->onGround = false;
        FighterSpawnDust(game, (Vector2){f->position.x, f->position.y}, 3);
        return;
    }

    // Crouch (S key)
    if (IsKeyDown(KEY_S) && f->onGround) {
        if (f->state != FIGHTER_CROUCH) FighterSetState(f, FIGHTER_CROUCH);
    } else if (f->state == FIGHTER_CROUCH) {
        FighterSetState(f, FIGHTER_IDLE);
    }

    // Move
    float speed = 250.0f;
    if (IsKeyDown(KEY_D)) {
        f->velocity.x = speed;
        if (f->onGround && f->state != FIGHTER_CROUCH) FighterSetState(f, FIGHTER_WALK_FORWARD);
    } else if (IsKeyDown(KEY_A)) {
        f->velocity.x = -speed;
        if (f->onGround && f->state != FIGHTER_CROUCH) FighterSetState(f, FIGHTER_WALK_BACKWARD);
    } else if (f->onGround && (f->state == FIGHTER_WALK_FORWARD || f->state == FIGHTER_WALK_BACKWARD)) {
        FighterSetState(f, FIGHTER_IDLE);
    }
}

// ============================================================================
// XỬ LÝ INPUT PLAYER 2 (Arrow keys + Numpad)
// ============================================================================
static void HandleInput_P2(FighterGame *game, float dt)
{
    Fighter *f = &game->player2;
    if (f->hitStun > 0 || f->blockStun > 0) return;
    if (f->state == FIGHTER_KNOCKDOWN || f->state == FIGHTER_VICTORY || f->state == FIGHTER_DEFEAT) return;
    if (f->state == FIGHTER_PUNCH || f->state == FIGHTER_KICK || f->state == FIGHTER_SPECIAL) return;

    // Block
    if (IsKeyDown(KEY_KP_0)) {
        if (f->state != FIGHTER_BLOCK && f->onGround) {
            FighterSetState(f, FIGHTER_BLOCK);
        }
        return;
    } else if (f->state == FIGHTER_BLOCK) {
        FighterSetState(f, FIGHTER_IDLE);
    }

    // Special (Numpad 3)
    if (IsKeyPressed(KEY_KP_3) && f->specialCooldown <= 0 && f->superMeter >= 20.0f) {
        FighterSetState(f, FIGHTER_SPECIAL);
        f->specialCooldown = 1.0f;
        f->superMeter -= 20.0f;

        float dir = f->facingRight ? 1.0f : -1.0f;
        Vector2 projPos = {f->position.x + dir * 40, f->position.y - 80};
        Vector2 projVel = {dir * 500.0f, 0};
        FighterSpawnProjectile(game, 1, projPos, projVel, 80, (Color){255, 100, 50, 255});
        FighterSpawnHitSparks(game, projPos, 8);
        return;
    }

    // Punch (Numpad 1)
    if (IsKeyPressed(KEY_KP_1)) {
        FighterSetState(f, FIGHTER_PUNCH);
        f->superMeter += 3.0f;
        if (f->superMeter > 100.0f) f->superMeter = 100.0f;
        return;
    }

    // Kick (Numpad 2)
    if (IsKeyPressed(KEY_KP_2)) {
        FighterSetState(f, FIGHTER_KICK);
        f->superMeter += 4.0f;
        if (f->superMeter > 100.0f) f->superMeter = 100.0f;
        return;
    }

    // Jump (Up arrow)
    if (IsKeyPressed(KEY_UP) && f->onGround) {
        FighterSetState(f, FIGHTER_JUMP);
        f->velocity.y = -650.0f;
        f->onGround = false;
        FighterSpawnDust(game, (Vector2){f->position.x, f->position.y}, 3);
        return;
    }

    // Crouch (Down arrow)
    if (IsKeyDown(KEY_DOWN) && f->onGround) {
        if (f->state != FIGHTER_CROUCH) FighterSetState(f, FIGHTER_CROUCH);
    } else if (f->state == FIGHTER_CROUCH) {
        FighterSetState(f, FIGHTER_IDLE);
    }

    // Move
    float speed = 250.0f;
    if (IsKeyDown(KEY_RIGHT)) {
        f->velocity.x = speed;
        if (f->onGround && f->state != FIGHTER_CROUCH) FighterSetState(f, FIGHTER_WALK_FORWARD);
    } else if (IsKeyDown(KEY_LEFT)) {
        f->velocity.x = -speed;
        if (f->onGround && f->state != FIGHTER_CROUCH) FighterSetState(f, FIGHTER_WALK_BACKWARD);
    } else if (f->onGround && (f->state == FIGHTER_WALK_FORWARD || f->state == FIGHTER_WALK_BACKWARD)) {
        FighterSetState(f, FIGHTER_IDLE);
    }

    (void)dt;
}

// ============================================================================
// AI CHO PLAYER 2
// ============================================================================
static void HandleAI(FighterGame *game, float dt)
{
    Fighter *ai = &game->player2;
    Fighter *target = &game->player1;

    if (ai->hitStun > 0 || ai->blockStun > 0) return;
    if (ai->state == FIGHTER_KNOCKDOWN || ai->state == FIGHTER_VICTORY || ai->state == FIGHTER_DEFEAT) return;
    if (ai->state == FIGHTER_PUNCH || ai->state == FIGHTER_KICK || ai->state == FIGHTER_SPECIAL) return;

    game->aiThinkTimer -= dt;
    if (game->aiThinkTimer > 0) return;
    game->aiThinkTimer = game->aiReactionTime + (float)GetRandomValue(0, 200) / 1000.0f;

    float dist = fabsf(ai->position.x - target->position.x);

    // Update facing
    ai->facingRight = (target->position.x > ai->position.x);

    // Có projectile đang bay tới?
    bool incomingProjectile = false;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &game->projectiles[i];
        if (!p->active || p->owner == 1) continue;
        float projDist = fabsf(p->position.x - ai->position.x);
        if (projDist < 200 && ((p->velocity.x > 0 && p->position.x < ai->position.x) ||
            (p->velocity.x < 0 && p->position.x > ai->position.x))) {
            incomingProjectile = true;
            break;
        }
    }

    // Block incoming projectile
    if (incomingProjectile && ai->onGround) {
        if (GetRandomValue(0, 100) < 70) {
            FighterSetState(ai, FIGHTER_BLOCK);
            return;
        } else if (ai->onGround) {
            FighterSetState(ai, FIGHTER_JUMP);
            ai->velocity.y = -650.0f;
            ai->onGround = false;
            return;
        }
    }

    // Nếu block, có 30% bỏ block mỗi frame AI think
    if (ai->state == FIGHTER_BLOCK && !incomingProjectile) {
        if (GetRandomValue(0, 100) < 30) {
            FighterSetState(ai, FIGHTER_IDLE);
        }
        return;
    }

    // Block khi bị tấn công ở gần
    if (dist < 120 && (target->state == FIGHTER_PUNCH || target->state == FIGHTER_KICK)) {
        if (GetRandomValue(0, 100) < 50) {
            FighterSetState(ai, FIGHTER_BLOCK);
            return;
        }
    }

    // Khoảng cách tấn công
    if (dist < 100) {
        // Close range - đánh
        int action = GetRandomValue(0, 100);
        if (action < 40) {
            FighterSetState(ai, FIGHTER_PUNCH);
            ai->superMeter += 3.0f;
        } else if (action < 70) {
            FighterSetState(ai, FIGHTER_KICK);
            ai->superMeter += 4.0f;
        } else if (action < 85 && ai->onGround) {
            // Nhảy lùi
            FighterSetState(ai, FIGHTER_JUMP);
            ai->velocity.y = -600.0f;
            ai->velocity.x = (ai->facingRight ? -1 : 1) * 150.0f;
            ai->onGround = false;
        } else {
            FighterSetState(ai, FIGHTER_BLOCK);
        }
    } else if (dist < 300) {
        // Mid range
        int action = GetRandomValue(0, 100);
        if (action < 30) {
            // Tiến tới
            float dir = (target->position.x > ai->position.x) ? 1.0f : -1.0f;
            ai->velocity.x = dir * 250.0f;
            FighterSetState(ai, FIGHTER_WALK_FORWARD);
        } else if (action < 50 && ai->specialCooldown <= 0 && ai->superMeter >= 20.0f) {
            // Hadouken
            FighterSetState(ai, FIGHTER_SPECIAL);
            ai->specialCooldown = 1.5f;
            ai->superMeter -= 20.0f;
            float dir = ai->facingRight ? 1.0f : -1.0f;
            Vector2 projPos = {ai->position.x + dir * 40, ai->position.y - 80};
            Vector2 projVel = {dir * 500.0f, 0};
            FighterSpawnProjectile(game, 1, projPos, projVel, 80, (Color){255, 100, 50, 255});
            FighterSpawnHitSparks(game, projPos, 8);
        } else if (action < 70 && ai->onGround) {
            // Jump forward
            FighterSetState(ai, FIGHTER_JUMP);
            float dir = (target->position.x > ai->position.x) ? 1.0f : -1.0f;
            ai->velocity.y = -650.0f;
            ai->velocity.x = dir * 200.0f;
            ai->onGround = false;
        } else {
            // Wait / idle
            FighterSetState(ai, FIGHTER_IDLE);
        }
    } else {
        // Far range - tiến tới hoặc bắn
        int action = GetRandomValue(0, 100);
        if (action < 50) {
            float dir = (target->position.x > ai->position.x) ? 1.0f : -1.0f;
            ai->velocity.x = dir * 250.0f;
            FighterSetState(ai, FIGHTER_WALK_FORWARD);
        } else if (action < 80 && ai->specialCooldown <= 0 && ai->superMeter >= 20.0f) {
            FighterSetState(ai, FIGHTER_SPECIAL);
            ai->specialCooldown = 1.2f;
            ai->superMeter -= 20.0f;
            float dir = ai->facingRight ? 1.0f : -1.0f;
            Vector2 projPos = {ai->position.x + dir * 40, ai->position.y - 80};
            Vector2 projVel = {dir * 500.0f, 0};
            FighterSpawnProjectile(game, 1, projPos, projVel, 80, (Color){255, 100, 50, 255});
        } else {
            FighterSetState(ai, FIGHTER_IDLE);
        }
    }

    if (ai->superMeter > 100.0f) ai->superMeter = 100.0f;
}

// ============================================================================
// KIỂM TRA VA CHẠM ĐÒN ĐÁNH
// ============================================================================
static void CheckCollisions(FighterGame *game)
{
    Fighter *fighters[2] = {&game->player1, &game->player2};

    for (int i = 0; i < 2; i++) {
        int j = 1 - i;
        Fighter *attacker = fighters[i];
        Fighter *defender = fighters[j];

        if (attacker->hasHitThisAttack) continue;

        Hitbox atk = GetFighterAttackHitbox(attacker);
        if (!atk.active) continue;

        Rectangle defHurt = GetFighterHurtbox(defender);

        if (CheckCollisionRecs(atk.rect, defHurt)) {
            attacker->hasHitThisAttack = true;

            // Xác định điểm va chạm
            Vector2 hitPoint = {
                (atk.rect.x + atk.rect.width / 2 + defHurt.x + defHurt.width / 2) / 2,
                (atk.rect.y + atk.rect.height / 2 + defHurt.y + defHurt.height / 2) / 2
            };

            if (defender->state == FIGHTER_BLOCK) {
                // Blocked!
                int chipDamage = atk.damage / 10;
                defender->health -= chipDamage;
                if (defender->health < 0) defender->health = 0;
                defender->blockStun = 0.3f;
                defender->velocity.x = (attacker->facingRight ? 1 : -1) * atk.knockback * 0.3f;

                FighterSpawnBlockSparks(game, hitPoint, 10);
                game->hitPauseTimer = 0.03f;
                game->screenShake = 3.0f;
            } else {
                // Hit!
                bool fromRight = (attacker->position.x > defender->position.x);
                FighterTakeDamage(defender, atk.damage, atk.knockback, fromRight);

                // Combo
                attacker->comboCount++;
                attacker->comboTimer = 1.5f;

                // Super meter cho attacker
                attacker->superMeter += atk.damage * 0.08f;
                if (attacker->superMeter > 100.0f) attacker->superMeter = 100.0f;

                FighterSpawnHitSparks(game, hitPoint, 15);
                game->hitPauseTimer = 0.06f;
                game->screenShake = 8.0f;

                // Kiểm tra KO
                if (defender->health <= 0) {
                    game->screenShake = 20.0f;
                    game->hitPauseTimer = 0.15f;
                }
            }
        }
    }
}

// ============================================================================
// KIỂM TRA VA CHẠM PROJECTILE
// ============================================================================
static void CheckProjectileCollisions(FighterGame *game)
{
    Fighter *fighters[2] = {&game->player1, &game->player2};

    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) continue;

        int targetIdx = 1 - p->owner;
        Fighter *target = fighters[targetIdx];

        Rectangle targetHurt = GetFighterHurtbox(target);

        if (CheckCollisionCircleRec(p->position, p->radius, targetHurt)) {
            Vector2 hitPoint = p->position;

            if (target->state == FIGHTER_BLOCK) {
                int chipDamage = p->damage / 8;
                target->health -= chipDamage;
                if (target->health < 0) target->health = 0;
                target->blockStun = 0.4f;
                target->velocity.x = (p->velocity.x > 0 ? 1 : -1) * 100.0f;

                FighterSpawnBlockSparks(game, hitPoint, 15);
                game->screenShake = 5.0f;
            } else {
                bool fromRight = (p->position.x > target->position.x);
                FighterTakeDamage(target, p->damage, 250.0f, fromRight);

                fighters[p->owner]->comboCount++;
                fighters[p->owner]->comboTimer = 1.5f;
                fighters[p->owner]->superMeter += p->damage * 0.06f;
                if (fighters[p->owner]->superMeter > 100.0f)
                    fighters[p->owner]->superMeter = 100.0f;

                FighterSpawnHitSparks(game, hitPoint, 20);
                game->screenShake = 12.0f;
                game->hitPauseTimer = 0.08f;
            }

            p->active = false;
        }

        // Projectile vs Projectile
        for (int j = i + 1; j < MAX_PROJECTILES; j++) {
            Projectile *p2 = &game->projectiles[j];
            if (!p2->active || p->owner == p2->owner) continue;

            if (CheckCollisionCircles(p->position, p->radius, p2->position, p2->radius)) {
                Vector2 midPoint = {
                    (p->position.x + p2->position.x) / 2,
                    (p->position.y + p2->position.y) / 2
                };
                FighterSpawnHitSparks(game, midPoint, 25);
                game->screenShake = 10.0f;
                p->active = false;
                p2->active = false;
            }
        }
    }
}

// ============================================================================
// CẬP NHẬT GAME
// ============================================================================
void UpdateFighterGame(FighterGame *game, float dt)
{
    game->stateTimer += dt;
    game->cloudOffset += dt * 15.0f;

    switch (game->state) {
        case GAME_STATE_TITLE:
        {
            if (IsKeyPressed(KEY_ENTER)) {
                game->state = GAME_STATE_INTRO;
                game->stateTimer = 0;
                game->currentRound = 1;
                game->player1.roundsWon = 0;
                game->player2.roundsWon = 0;
                StartRound(game);
            }
            if (IsKeyPressed(KEY_TAB)) {
                game->p2IsAI = !game->p2IsAI;
            }
            break;
        }

        case GAME_STATE_INTRO:
        {
            if (game->stateTimer > 2.8f) {
                game->state = GAME_STATE_PLAYING;
                game->stateTimer = 0;
            }
            break;
        }

        case GAME_STATE_PLAYING:
        {
            // Hit pause (hiệu ứng dừng sesaat)
            if (game->hitPauseTimer > 0) {
                game->hitPauseTimer -= dt;
                if (game->hitPauseTimer > 0) return; // Dừng mọi thứ
            }

            float gameDt = dt * game->slowMotion;

            // Input
            HandleInput_P1(game, gameDt);
            if (game->p2IsAI) {
                HandleAI(game, gameDt);
            } else {
                HandleInput_P2(game, gameDt);
            }

            // Update facing (luôn quay mặt về đối thủ)
            if (game->player1.state != FIGHTER_HIT && game->player1.state != FIGHTER_KNOCKDOWN) {
                game->player1.facingRight = (game->player2.position.x > game->player1.position.x);
            }
            if (game->player2.state != FIGHTER_HIT && game->player2.state != FIGHTER_KNOCKDOWN) {
                game->player2.facingRight = (game->player1.position.x > game->player2.position.x);
            }

            // Update fighters
            UpdateFighter(&game->player1, gameDt);
            UpdateFighter(&game->player2, gameDt);

            // Push-out collision (không cho 2 fighter chồng lên nhau)
            float minDist = 60.0f;
            float dx = game->player2.position.x - game->player1.position.x;
            if (fabsf(dx) < minDist) {
                float push = (minDist - fabsf(dx)) * 0.5f;
                float dir = (dx > 0) ? 1.0f : -1.0f;
                game->player1.position.x -= dir * push;
                game->player2.position.x += dir * push;
            }

            // Collisions
            CheckCollisions(game);
            FighterUpdateProjectiles(game, gameDt);
            CheckProjectileCollisions(game);
            FighterUpdateParticles(game, gameDt);

            // Super meter tự tăng nhẹ
            game->player1.superMeter += dt * 2.0f;
            if (game->player1.superMeter > 100.0f) game->player1.superMeter = 100.0f;
            game->player2.superMeter += dt * 2.0f;
            if (game->player2.superMeter > 100.0f) game->player2.superMeter = 100.0f;

            // Timer
            game->roundTimer -= dt;
            if (game->roundTimer <= 0) {
                game->roundTimer = 0;
                EndRound(game);
            }

            // KO check
            if (game->player1.health <= 0 || game->player2.health <= 0) {
                // Đợi animation knockdown xong
                Fighter *knocked = (game->player1.health <= 0) ? &game->player1 : &game->player2;
                if (knocked->onGround && knocked->stateTimer > 0.5f) {
                    EndRound(game);
                }
            }

            // Screen shake
            if (game->screenShake > 0) {
                game->screenShake *= 0.9f;
                game->shakeOffset.x = (float)GetRandomValue(-100, 100) / 100.0f * game->screenShake;
                game->shakeOffset.y = (float)GetRandomValue(-100, 100) / 100.0f * game->screenShake * 0.5f;
                if (game->screenShake < 0.5f) {
                    game->screenShake = 0;
                    game->shakeOffset = (Vector2){0, 0};
                }
            }

            break;
        }

        case GAME_STATE_ROUND_END:
        {
            FighterUpdateParticles(game, dt);
            FighterUpdateProjectiles(game, dt);
            UpdateFighter(&game->player1, dt * 0.5f);
            UpdateFighter(&game->player2, dt * 0.5f);

            if (game->screenShake > 0) {
                game->screenShake *= 0.95f;
                game->shakeOffset.x = (float)GetRandomValue(-100, 100) / 100.0f * game->screenShake;
                game->shakeOffset.y = (float)GetRandomValue(-100, 100) / 100.0f * game->screenShake * 0.5f;
                if (game->screenShake < 0.5f) {
                    game->screenShake = 0;
                    game->shakeOffset = (Vector2){0, 0};
                }
            }

            if (game->stateTimer > 3.0f) {
                // Kiểm tra match end
                if (game->player1.roundsWon >= WINS_NEEDED || game->player2.roundsWon >= WINS_NEEDED) {
                    game->state = GAME_STATE_MATCH_END;
                    game->stateTimer = 0;
                } else {
                    // Round mới
                    game->currentRound++;
                    game->state = GAME_STATE_INTRO;
                    game->stateTimer = 0;
                    StartRound(game);
                }
            }
            break;
        }

        case GAME_STATE_MATCH_END:
        {
            if (game->stateTimer > 2.0f && IsKeyPressed(KEY_ENTER)) {
                game->state = GAME_STATE_TITLE;
                game->stateTimer = 0;
                InitFighterGame(game);
            }
            break;
        }
    }
}

// ============================================================================
// VẼ GAME
// ============================================================================
void DrawFighterGame(FighterGame *game)
{
    ClearBackground(BLACK);

    switch (game->state) {
        case GAME_STATE_TITLE:
            DrawTitleScreen(game);
            break;

        case GAME_STATE_INTRO:
            DrawIntroScreen(game);
            break;

        case GAME_STATE_PLAYING:
        {
            DrawBackground(game);
            FighterDrawParticles(game);
            FighterDrawProjectiles(game);

            // Vẽ fighter phía sau trước (dựa theo vị trí Y)
            if (game->player1.position.y <= game->player2.position.y) {
                DrawFighter(&game->player1, game->shakeOffset.x, game->shakeOffset.y);
                DrawFighter(&game->player2, game->shakeOffset.x, game->shakeOffset.y);
            } else {
                DrawFighter(&game->player2, game->shakeOffset.x, game->shakeOffset.y);
                DrawFighter(&game->player1, game->shakeOffset.x, game->shakeOffset.y);
            }

            DrawHUD(game);

            // Hit pause flash
            if (game->hitPauseTimer > 0) {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(WHITE, 0.15f));
            }
            break;
        }

        case GAME_STATE_ROUND_END:
            DrawRoundEndScreen(game);
            break;

        case GAME_STATE_MATCH_END:
            DrawMatchEndScreen(game);
            break;
    }
}
