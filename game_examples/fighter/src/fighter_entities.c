// ============================================================================
// FIGHTER ENTITIES - Xử lý Fighter, Projectile, Particle
// ============================================================================
#include "raylib.h"
#include "raymath.h"
#include "fighter_types.h"
#include "fighter_entities.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// FIGHTER - Khởi tạo
// ============================================================================
void InitFighter(Fighter *f, CharacterType charType, float x, bool facingRight)
{
    memset(f, 0, sizeof(Fighter));
    f->character = charType;
    f->position = (Vector2){x, GROUND_Y};
    f->velocity = (Vector2){0, 0};
    f->facingRight = facingRight;
    f->onGround = true;
    f->health = 1000;
    f->maxHealth = 1000;
    f->superMeter = 0.0f;
    f->state = FIGHTER_IDLE;
    f->stateTimer = 0.0f;
    f->attackTimer = 0.0f;
    f->hitStun = 0.0f;
    f->blockStun = 0.0f;
    f->comboCount = 0;
    f->comboTimer = 0.0f;
    f->animFrame = 0.0f;
    f->animSpeed = 8.0f;
    f->currentFrame = 0;
    f->width = 80.0f;
    f->height = 140.0f;
    f->roundsWon = 0;
    f->specialCooldown = 0.0f;
    f->hasHitThisAttack = false;
    f->flashTimer = 0.0f;
    f->shakeOffset = 0.0f;

    // Hurtbox mặc định
    f->hurtbox = (Rectangle){-30, -130, 60, 130};

    // Attack hitbox (inactive mặc định)
    f->attackHitbox.active = false;
    f->attackHitbox.damage = 0;
    f->attackHitbox.knockback = 0;
}

// ============================================================================
// FIGHTER - Đặt trạng thái
// ============================================================================
void FighterSetState(Fighter *f, FighterState state)
{
    if (f->state == state) return;
    f->state = state;
    f->stateTimer = 0.0f;
    f->animFrame = 0.0f;
    f->currentFrame = 0;
    f->attackHitbox.active = false;
    f->hasHitThisAttack = false;

    // Thiết lập thông số cho từng trạng thái
    switch (state) {
        case FIGHTER_PUNCH:
            f->attackTimer = 0.25f;
            f->attackHitbox.damage = 50;
            f->attackHitbox.knockback = 150.0f;
            break;
        case FIGHTER_KICK:
            f->attackTimer = 0.35f;
            f->attackHitbox.damage = 70;
            f->attackHitbox.knockback = 200.0f;
            break;
        case FIGHTER_SPECIAL:
            f->attackTimer = 0.5f;
            f->attackHitbox.damage = 0; // Projectile sẽ gây sát thương
            f->attackHitbox.knockback = 0;
            break;
        case FIGHTER_HIT:
            f->stateTimer = 0.3f;
            f->hitStun = 0.3f;
            break;
        case FIGHTER_KNOCKDOWN:
            f->stateTimer = 1.0f;
            break;
        case FIGHTER_BLOCK:
            f->blockStun = 0.2f;
            break;
        case FIGHTER_VICTORY:
        case FIGHTER_DEFEAT:
            f->stateTimer = 3.0f;
            break;
        default:
            break;
    }
}

// ============================================================================
// FIGHTER - Nhận sát thương
// ============================================================================
void FighterTakeDamage(Fighter *f, int damage, float knockback, bool fromRight)
{
    f->health -= damage;
    if (f->health < 0) f->health = 0;

    // Tăng super meter khi bị đánh
    f->superMeter += damage * 0.05f;
    if (f->superMeter > 100.0f) f->superMeter = 100.0f;

    // Knockback
    float dir = fromRight ? -1.0f : 1.0f;
    f->velocity.x = dir * knockback;

    // Flash effect
    f->flashTimer = 0.15f;
    f->shakeOffset = 5.0f;

    if (f->health <= 0) {
        FighterSetState(f, FIGHTER_KNOCKDOWN);
        f->velocity.x = dir * knockback * 1.5f;
        f->velocity.y = -400.0f;
        f->onGround = false;
    } else {
        FighterSetState(f, FIGHTER_HIT);
    }
}

// ============================================================================
// FIGHTER - Lấy hurtbox (world space)
// ============================================================================
Rectangle GetFighterHurtbox(Fighter *f)
{
    float crouchMod = (f->state == FIGHTER_CROUCH) ? 0.6f : 1.0f;
    Rectangle r;
    r.x = f->position.x + f->hurtbox.x;
    r.y = f->position.y + f->hurtbox.y * crouchMod;
    r.width = f->hurtbox.width;
    r.height = f->hurtbox.height * crouchMod;
    return r;
}

// ============================================================================
// FIGHTER - Lấy attack hitbox (world space)
// ============================================================================
Hitbox GetFighterAttackHitbox(Fighter *f)
{
    Hitbox h = f->attackHitbox;
    if (!h.active) return h;

    float dir = f->facingRight ? 1.0f : -1.0f;
    h.rect.x = f->position.x + (f->facingRight ? 30.0f : -30.0f - h.rect.width);
    h.rect.y = f->position.y - 100.0f;

    if (f->state == FIGHTER_KICK) {
        h.rect.y = f->position.y - 70.0f;
        h.rect.width = 70.0f;
        h.rect.height = 40.0f;
    } else {
        h.rect.width = 60.0f;
        h.rect.height = 30.0f;
    }
    (void)dir;
    return h;
}

// ============================================================================
// FIGHTER - Cập nhật
// ============================================================================
void UpdateFighter(Fighter *f, float dt)
{
    // Timers
    f->stateTimer += dt;
    f->flashTimer -= dt;
    if (f->flashTimer < 0) f->flashTimer = 0;
    f->shakeOffset *= 0.9f;

    if (f->specialCooldown > 0) f->specialCooldown -= dt;
    if (f->specialCooldown < 0) f->specialCooldown = 0;

    if (f->hitStun > 0) {
        f->hitStun -= dt;
        if (f->hitStun <= 0) {
            f->hitStun = 0;
            if (f->state == FIGHTER_HIT) FighterSetState(f, FIGHTER_IDLE);
        }
    }

    if (f->blockStun > 0) {
        f->blockStun -= dt;
        if (f->blockStun <= 0) {
            f->blockStun = 0;
            if (f->state == FIGHTER_BLOCK) FighterSetState(f, FIGHTER_IDLE);
        }
    }

    if (f->comboTimer > 0) {
        f->comboTimer -= dt;
        if (f->comboTimer <= 0) {
            f->comboCount = 0;
            f->comboTimer = 0;
        }
    }

    // Trọng lực
    if (!f->onGround) {
        f->velocity.y += GRAVITY * dt;
        f->position.y += f->velocity.y * dt;
        if (f->position.y >= GROUND_Y) {
            f->position.y = GROUND_Y;
            f->velocity.y = 0;
            f->onGround = true;
            if (f->state == FIGHTER_JUMP) FighterSetState(f, FIGHTER_IDLE);
            if (f->state == FIGHTER_KNOCKDOWN && f->health <= 0) {
                // Nằm trên mặt đất
            }
        }
    }

    // Di chuyển ngang (ma sát)
    if (f->onGround && f->state != FIGHTER_WALK_FORWARD && f->state != FIGHTER_WALK_BACKWARD) {
        f->velocity.x *= 0.85f;
    }
    f->position.x += f->velocity.x * dt;

    // Giới hạn trong sân đấu
    if (f->position.x < 50) f->position.x = 50;
    if (f->position.x > SCREEN_WIDTH - 50) f->position.x = SCREEN_WIDTH - 50;

    // Xử lý attack timer
    if (f->state == FIGHTER_PUNCH || f->state == FIGHTER_KICK) {
        f->attackTimer -= dt;

        // Active hitbox trong khoảng giữa animation
        float totalTime = (f->state == FIGHTER_PUNCH) ? 0.25f : 0.35f;
        float elapsed = totalTime - f->attackTimer;
        if (elapsed > totalTime * 0.2f && elapsed < totalTime * 0.6f) {
            f->attackHitbox.active = true;
        } else {
            f->attackHitbox.active = false;
        }

        if (f->attackTimer <= 0) {
            FighterSetState(f, FIGHTER_IDLE);
        }
    }

    if (f->state == FIGHTER_SPECIAL) {
        f->attackTimer -= dt;
        if (f->attackTimer <= 0) {
            FighterSetState(f, FIGHTER_IDLE);
        }
    }

    if (f->state == FIGHTER_KNOCKDOWN && f->onGround) {
        f->stateTimer += dt;
        // Tự động đứng dậy sau 1 giây nếu còn máu
        if (f->health > 0 && f->stateTimer > 1.5f) {
            FighterSetState(f, FIGHTER_IDLE);
        }
    }

    // Animation
    f->animFrame += f->animSpeed * dt;
    f->currentFrame = (int)f->animFrame % 4;
}

// ============================================================================
// FIGHTER - Vẽ nhân vật (sử dụng hình học đơn giản tạo hình fighter)
// ============================================================================
static void DrawFighterBody(Fighter *f, Color bodyColor, Color pantsColor,
                            Color skinColor, Color hairColor, Color beltColor,
                            float ox, float oy)
{
    float x = f->position.x + ox + f->shakeOffset * sinf(f->stateTimer * 60.0f);
    float y = f->position.y + oy;
    float dir = f->facingRight ? 1.0f : -1.0f;
    int frame = f->currentFrame;

    // Flash khi bị đánh
    Color bc = bodyColor, pc = pantsColor, sc = skinColor, hc = hairColor;
    if (f->flashTimer > 0) {
        float t = f->flashTimer / 0.15f;
        bc = ColorAlpha(WHITE, t);
        pc = ColorAlpha(WHITE, t);
        sc = ColorAlpha(WHITE, t);
        hc = ColorAlpha(WHITE, t);
    }

    float bobY = 0;
    float armAngle = 0;
    float legSpread = 0;
    float leanX = 0;

    switch (f->state) {
        case FIGHTER_IDLE:
            bobY = sinf(f->stateTimer * 4.0f) * 3.0f;
            armAngle = sinf(f->stateTimer * 2.0f) * 5.0f;
            break;
        case FIGHTER_WALK_FORWARD:
        case FIGHTER_WALK_BACKWARD:
            bobY = sinf(f->stateTimer * 10.0f) * 4.0f;
            legSpread = sinf(f->stateTimer * 10.0f) * 15.0f;
            break;
        case FIGHTER_JUMP:
            bobY = -10.0f;
            break;
        case FIGHTER_CROUCH:
            bobY = 30.0f;
            break;
        case FIGHTER_PUNCH:
            leanX = dir * 15.0f;
            break;
        case FIGHTER_KICK:
            leanX = dir * 10.0f;
            break;
        case FIGHTER_HIT:
            leanX = -dir * 10.0f;
            bobY = sinf(f->stateTimer * 30.0f) * 5.0f;
            break;
        case FIGHTER_KNOCKDOWN:
            if (!f->onGround) {
                // Bay lên
            } else {
                bobY = 60.0f;
            }
            break;
        case FIGHTER_BLOCK:
            leanX = -dir * 5.0f;
            break;
        case FIGHTER_SPECIAL:
            leanX = dir * 20.0f;
            break;
        default:
            break;
    }

    float cx = x + leanX;
    float cy = y + bobY;

    // === CHÂN (2 chân) ===
    float legY = cy;
    float legW = 14, legH = 45;
    float legOff = 12.0f + legSpread;
    float legOff2 = 12.0f - legSpread;

    if (f->state == FIGHTER_KNOCKDOWN && f->onGround) {
        // Nằm ngang
        DrawRectanglePro((Rectangle){cx - 40, cy - 10, 80, 20}, (Vector2){40, 10}, 0, pc);
        DrawRectanglePro((Rectangle){cx - 35, cy - 20, 70, 15}, (Vector2){35, 7}, 0, bc);
        // Đầu
        DrawCircle((int)(cx + dir * 35), (int)(cy - 20), 18, sc);
        DrawCircle((int)(cx + dir * 40), (int)(cy - 28), 14, hc);
        return;
    }

    if (f->state == FIGHTER_KICK) {
        // Chân đá
        float kickExtend = 0;
        float totalT = 0.35f;
        float elapsed = totalT - f->attackTimer;
        if (elapsed < totalT * 0.5f) {
            kickExtend = (elapsed / (totalT * 0.5f)) * 60.0f;
        } else {
            kickExtend = (1.0f - (elapsed - totalT * 0.5f) / (totalT * 0.5f)) * 60.0f;
        }
        // Chân sau
        DrawRectangle((int)(cx - legOff2 * dir - legW/2), (int)(legY - legH), (int)legW, (int)legH, pc);
        DrawRectangle((int)(cx - legOff2 * dir - 7), (int)(legY - 5), 14, 8, sc); // bàn chân
        // Chân đá (ngang)
        DrawRectangle((int)(cx + dir * 5), (int)(legY - 40), (int)(kickExtend * dir), 16, pc);
        DrawCircle((int)(cx + dir * (5 + kickExtend)), (int)(legY - 32), 8, sc); // bàn chân
    } else if (f->state == FIGHTER_CROUCH) {
        // Ngồi
        DrawRectangle((int)(cx - 18), (int)(legY - 20), 36, 20, pc);
        DrawRectangle((int)(cx - 20), (int)(legY - 5), 14, 8, sc);
        DrawRectangle((int)(cx + 6), (int)(legY - 5), 14, 8, sc);
    } else {
        // Chân thường
        DrawRectangle((int)(cx + legOff * dir - legW/2), (int)(legY - legH), (int)legW, (int)legH, pc);
        DrawRectangle((int)(cx - legOff2 * dir - legW/2), (int)(legY - legH), (int)legW, (int)legH, pc);
        // Bàn chân
        DrawRectangle((int)(cx + legOff * dir - 7), (int)(legY - 5), 14, 8, sc);
        DrawRectangle((int)(cx - legOff2 * dir - 7), (int)(legY - 5), 14, 8, sc);
    }

    // === THÂN ===
    float bodyY = cy - legH - 5;
    float bodyH = 45;
    float bodyW = 44;

    if (f->state != FIGHTER_CROUCH) {
        DrawRectangle((int)(cx - bodyW/2 + leanX * 0.3f), (int)(bodyY - bodyH), (int)bodyW, (int)bodyH, bc);
        // Đai lưng
        DrawRectangle((int)(cx - bodyW/2 + leanX * 0.3f), (int)(bodyY - 5), (int)bodyW, 6, beltColor);
    } else {
        DrawRectangle((int)(cx - bodyW/2), (int)(cy - 45), (int)bodyW, 30, bc);
        DrawRectangle((int)(cx - bodyW/2), (int)(cy - 20), (int)bodyW, 5, beltColor);
        bodyY = cy - 15;
        bodyH = 25;
    }

    // === TAY ===
    float armY = bodyY - bodyH + 10;
    float armW = 12, armH = 35;

    if (f->state == FIGHTER_PUNCH) {
        // Tay đấm
        float punchExtend = 0;
        float totalT = 0.25f;
        float elapsed = totalT - f->attackTimer;
        if (elapsed < totalT * 0.3f) {
            punchExtend = (elapsed / (totalT * 0.3f)) * 55.0f;
        } else {
            punchExtend = (1.0f - (elapsed - totalT * 0.3f) / (totalT * 0.7f)) * 55.0f;
        }
        // Tay sau
        DrawRectangle((int)(cx - dir * 15 - armW/2), (int)(armY), (int)armW, (int)armH, sc);
        // Tay đấm (ngang)
        DrawRectangle((int)(cx + dir * 15), (int)(armY + 5), (int)(punchExtend * dir), 14, sc);
        // Nắm đấm
        DrawRectangle((int)(cx + dir * (15 + punchExtend) - 6), (int)(armY + 2), 14, 18, sc);

        // Hiệu ứng speed lines
        if (punchExtend > 30) {
            Color lineCol = ColorAlpha(WHITE, 0.5f);
            for (int i = 0; i < 3; i++) {
                float ly = armY + 5 + i * 4;
                DrawLine((int)(cx + dir * 15), (int)ly,
                         (int)(cx + dir * (15 + punchExtend * 0.6f)), (int)ly, lineCol);
            }
        }
    } else if (f->state == FIGHTER_SPECIAL) {
        // Tay hadouken pose
        float elapsed = 0.5f - f->attackTimer;
        float extendT = (elapsed < 0.3f) ? elapsed / 0.3f : 1.0f;
        DrawRectangle((int)(cx + dir * 10 - 6), (int)(armY + 5), (int)(30 * extendT * dir), 14, sc);
        DrawRectangle((int)(cx - dir * 10 - 6), (int)(armY + 5), (int)(30 * extendT * dir), 14, sc);
        // Energy glow
        if (extendT > 0.5f) {
            float glowR = 15.0f + sinf(f->stateTimer * 20.0f) * 5.0f;
            Color glowCol = (f->character == CHAR_RYU) ?
                (Color){100, 180, 255, 150} : (Color){255, 100, 50, 150};
            DrawCircle((int)(cx + dir * 35), (int)(armY + 12), glowR, glowCol);
        }
    } else if (f->state == FIGHTER_BLOCK) {
        // Tay chắn
        DrawRectangle((int)(cx + dir * 5 - 6), (int)(armY - 5), 14, 40, sc);
        DrawRectangle((int)(cx + dir * 12 - 6), (int)(armY + 0), 14, 35, sc);
    } else {
        // Tay bình thường (fighting stance)
        float aOff = sinf(f->stateTimer * 3.0f + armAngle * 0.1f) * 3.0f;
        // Tay trước (cao hơn, sẵn sàng)
        DrawRectangle((int)(cx + dir * 18 - armW/2), (int)(armY - 5 + aOff), (int)armW, (int)(armH * 0.8f), sc);
        // Tay sau
        DrawRectangle((int)(cx - dir * 12 - armW/2), (int)(armY + 5 - aOff), (int)armW, (int)(armH * 0.7f), sc);
        // Nắm đấm
        DrawCircle((int)(cx + dir * 18), (int)(armY - 5 + aOff + armH * 0.8f), 7, sc);
        DrawCircle((int)(cx - dir * 12), (int)(armY + 5 - aOff + armH * 0.7f), 6, sc);
    }

    // === ĐẦU ===
    float headY = bodyY - bodyH - 5;
    float headR = 20.0f;
    if (f->state == FIGHTER_CROUCH) {
        headY = cy - 55;
    }
    DrawCircle((int)(cx + leanX * 0.3f), (int)(headY), (int)headR, sc);

    // Tóc
    if (f->character == CHAR_RYU) {
        // Ryu: tóc ngắn đen + bandana đỏ
        DrawCircle((int)(cx + leanX * 0.3f), (int)(headY - 8), 18, hc);
        // Bandana
        DrawRectangle((int)(cx + leanX * 0.3f - 22), (int)(headY - 10), 44, 7, RED);
        // Dải bandana phía sau
        float bandanaWave = sinf(f->stateTimer * 5.0f) * 5.0f;
        DrawLineEx(
            (Vector2){cx + leanX * 0.3f - dir * 22, headY - 7},
            (Vector2){cx + leanX * 0.3f - dir * 40, headY - 5 + bandanaWave},
            3.0f, RED
        );
    } else {
        // Ken: tóc vàng dài
        DrawEllipse((int)(cx + leanX * 0.3f), (int)(headY - 10), 20, 16, hc);
        // Tóc phía sau
        float hairWave = sinf(f->stateTimer * 4.0f) * 3.0f;
        for (int i = 0; i < 5; i++) {
            float hw = sinf(f->stateTimer * 3.0f + i * 0.5f) * 3.0f;
            DrawCircle((int)(cx + leanX * 0.3f - dir * (10 + i * 4)),
                       (int)(headY - 5 + i * 3 + hw), 6 - i * 0.5f, hc);
        }
    }

    // Mắt
    float eyeX = cx + leanX * 0.3f + dir * 8;
    float eyeY = headY - 2;
    DrawRectangle((int)(eyeX - 3), (int)(eyeY - 2), 6, 4, WHITE);
    DrawRectangle((int)(eyeX - 1 + dir * 1), (int)(eyeY - 1), 3, 3, BLACK);

    // Miệng
    if (f->state == FIGHTER_HIT || f->state == FIGHTER_KNOCKDOWN) {
        DrawEllipse((int)(cx + leanX * 0.3f + dir * 5), (int)(headY + 10), 5, 4, (Color){60, 0, 0, 255});
    } else if (f->state == FIGHTER_PUNCH || f->state == FIGHTER_KICK || f->state == FIGHTER_SPECIAL) {
        DrawEllipse((int)(cx + leanX * 0.3f + dir * 5), (int)(headY + 8), 4, 5, (Color){60, 0, 0, 255});
    }

    // Cơ bắp (chi tiết nhỏ trên thân)
    Color muscleCol = ColorAlpha(ColorBrightness(sc, -0.15f), 0.5f);
    if (f->state != FIGHTER_CROUCH && f->state != FIGHTER_KNOCKDOWN) {
        // Ngực
        DrawLine((int)(cx - 5), (int)(bodyY - bodyH + 15), (int)(cx - 5), (int)(bodyY - bodyH + 30), muscleCol);
        DrawLine((int)(cx + 5), (int)(bodyY - bodyH + 15), (int)(cx + 5), (int)(bodyY - bodyH + 30), muscleCol);
    }
}

void DrawFighter(Fighter *f, float shakeX, float shakeY)
{
    Color bodyColor, pantsColor, skinColor, hairColor, beltColor;

    if (f->character == CHAR_RYU) {
        // Ryu: gi trắng, đai đen, da sáng, tóc nâu đậm
        bodyColor = (Color){240, 235, 220, 255};   // Gi trắng
        pantsColor = (Color){230, 225, 210, 255};   // Quần trắng
        skinColor = (Color){230, 190, 150, 255};    // Da
        hairColor = (Color){60, 40, 20, 255};       // Tóc nâu đậm
        beltColor = (Color){30, 30, 30, 255};       // Đai đen
    } else {
        // Ken: gi đỏ, đai đen, da sáng, tóc vàng
        bodyColor = (Color){200, 50, 40, 255};      // Gi đỏ
        pantsColor = (Color){180, 40, 30, 255};     // Quần đỏ
        skinColor = (Color){235, 195, 155, 255};    // Da
        hairColor = (Color){230, 200, 50, 255};     // Tóc vàng
        beltColor = (Color){30, 30, 30, 255};       // Đai đen
    }

    // Shadow
    float shadowAlpha = f->onGround ? 0.35f : 0.15f;
    DrawEllipse((int)(f->position.x + shakeX), (int)(GROUND_Y + 3),
                (int)(f->width * 0.4f), 6, ColorAlpha(BLACK, shadowAlpha));

    DrawFighterBody(f, bodyColor, pantsColor, skinColor, hairColor, beltColor, shakeX, shakeY);
}

// ============================================================================
// PROJECTILE
// ============================================================================
void FighterSpawnProjectile(FighterGame *game, int owner, Vector2 pos, Vector2 vel, int damage, Color color)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!game->projectiles[i].active) {
            game->projectiles[i].active = true;
            game->projectiles[i].position = pos;
            game->projectiles[i].velocity = vel;
            game->projectiles[i].radius = 20.0f;
            game->projectiles[i].damage = damage;
            game->projectiles[i].owner = owner;
            game->projectiles[i].lifetime = 2.0f;
            game->projectiles[i].color = color;
            game->projectiles[i].animTimer = 0;
            return;
        }
    }
}

void FighterUpdateProjectiles(FighterGame *game, float dt)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) continue;

        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->lifetime -= dt;
        p->animTimer += dt;

        if (p->lifetime <= 0 || p->position.x < -50 || p->position.x > SCREEN_WIDTH + 50) {
            p->active = false;
        }
    }
}

void FighterDrawProjectiles(FighterGame *game)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) continue;

        float pulse = sinf(p->animTimer * 15.0f) * 5.0f;
        float r = p->radius + pulse;

        // Core glow
        DrawCircle((int)p->position.x, (int)p->position.y, r + 8, ColorAlpha(p->color, 0.2f));
        DrawCircle((int)p->position.x, (int)p->position.y, r + 4, ColorAlpha(p->color, 0.4f));
        DrawCircle((int)p->position.x, (int)p->position.y, r, p->color);
        DrawCircle((int)p->position.x, (int)p->position.y, r * 0.6f, WHITE);

        // Energy trails
        float trailDir = (p->velocity.x > 0) ? -1.0f : 1.0f;
        for (int t = 0; t < 5; t++) {
            float trailX = p->position.x + trailDir * (t * 8 + sinf(p->animTimer * 10.0f + t) * 3.0f);
            float trailY = p->position.y + sinf(p->animTimer * 8.0f + t * 1.5f) * 4.0f;
            float trailR = r * (1.0f - t * 0.18f);
            DrawCircle((int)trailX, (int)trailY, trailR, ColorAlpha(p->color, 0.3f - t * 0.05f));
        }
    }
}

// ============================================================================
// PARTICLE
// ============================================================================
void FighterSpawnParticle(FighterGame *game, Vector2 pos, Vector2 vel, Color color, float size, float lifetime, int type)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!game->particles[i].active) {
            game->particles[i].active = true;
            game->particles[i].position = pos;
            game->particles[i].velocity = vel;
            game->particles[i].color = color;
            game->particles[i].size = size;
            game->particles[i].lifetime = lifetime;
            game->particles[i].maxLifetime = lifetime;
            game->particles[i].type = type;
            return;
        }
    }
}

void FighterSpawnHitSparks(FighterGame *game, Vector2 pos, int count)
{
    for (int i = 0; i < count; i++) {
        float angle = ((float)GetRandomValue(0, 360)) * DEG2RAD;
        float speed = (float)GetRandomValue(200, 600);
        Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed - 200};
        Color colors[] = {YELLOW, ORANGE, WHITE, RED};
        Color c = colors[GetRandomValue(0, 3)];
        float size = (float)GetRandomValue(2, 6);
        FighterSpawnParticle(game, pos, vel, c, size, 0.3f, 0);
    }
}

void FighterSpawnDust(FighterGame *game, Vector2 pos, int count)
{
    for (int i = 0; i < count; i++) {
        float angle = ((float)GetRandomValue(150, 210)) * DEG2RAD;
        float speed = (float)GetRandomValue(30, 100);
        Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed - 50};
        Color c = (Color){180, 160, 130, 200};
        float size = (float)GetRandomValue(3, 8);
        FighterSpawnParticle(game, pos, vel, c, size, 0.5f, 1);
    }
}

void FighterSpawnBlockSparks(FighterGame *game, Vector2 pos, int count)
{
    for (int i = 0; i < count; i++) {
        float angle = ((float)GetRandomValue(0, 360)) * DEG2RAD;
        float speed = (float)GetRandomValue(100, 300);
        Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed};
        Color c = (Color){200, 200, 255, 220};
        float size = (float)GetRandomValue(2, 5);
        FighterSpawnParticle(game, pos, vel, c, size, 0.2f, 2);
    }
}

void FighterUpdateParticles(FighterGame *game, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &game->particles[i];
        if (!p->active) continue;

        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->velocity.y += 500.0f * dt; // Gravity trên particles
        p->lifetime -= dt;

        if (p->lifetime <= 0) {
            p->active = false;
        }
    }
}

void FighterDrawParticles(FighterGame *game)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &game->particles[i];
        if (!p->active) continue;

        float t = p->lifetime / p->maxLifetime;
        Color c = ColorAlpha(p->color, t);
        float s = p->size * t;

        if (p->type == 0) {
            // Spark: hình tròn nhỏ sáng
            DrawCircle((int)p->position.x, (int)p->position.y, s, c);
            DrawCircle((int)p->position.x, (int)p->position.y, s * 0.5f, ColorAlpha(WHITE, t * 0.8f));
        } else if (p->type == 1) {
            // Dust: hình tròn mờ lớn
            DrawCircle((int)p->position.x, (int)p->position.y, s * 1.5f, c);
        } else {
            // Energy: hình sao
            DrawPoly((Vector2){p->position.x, p->position.y}, 5, s, p->lifetime * 360.0f, c);
        }
    }
}
