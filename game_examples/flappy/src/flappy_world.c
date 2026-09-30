#include "flappy_world.h"
#include "flappy_assets.h"
#include "flappy_audio.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define GRAVITY 1500.0f
#define FLAP_VELOCITY -410.0f
#define MAX_FALL_SPEED 640.0f

// Toạ độ mũi nhọn trong sprite đá 108x239 (đo từ kênh alpha của ảnh).
#define ROCK_TIP_X_UP 66.0f
#define ROCK_TIP_X_DOWN 65.0f
#define ROCK_BASE_L 2.0f
#define ROCK_BASE_R 106.0f
#define ROCK_TIP_FORGIVE 4.0f   // Nới mũi đá vài pixel để va chạm "công bằng" với mắt

#define TOP_TIP_MIN 48.0f                // Mũi đá treo không được cao hơn mức này
#define BOTTOM_TIP_MAX (GROUND_Y - 42.0f)

static const DifficultyDef s_difficulties[FLAPPY_DIFF_COUNT] = {
    [FLAPPY_DIFF_EASY] = {
        .name = "Dễ", .scrollSpeed = 150.0f, .speedPerPoint = 2.0f, .maxSpeedBonus = 70.0f,
        .gap = 168.0f, .minGap = 140.0f, .gapShrinkPerPoint = 0.6f,
        .spacing = 300.0f, .maxGapDelta = 130.0f,
        .accent = {96, 190, 92, 255}
    },
    [FLAPPY_DIFF_NORMAL] = {
        .name = "Thường", .scrollSpeed = 175.0f, .speedPerPoint = 2.5f, .maxSpeedBonus = 95.0f,
        .gap = 148.0f, .minGap = 122.0f, .gapShrinkPerPoint = 0.7f,
        .spacing = 270.0f, .maxGapDelta = 150.0f,
        .accent = {242, 176, 48, 255}
    },
    [FLAPPY_DIFF_HARD] = {
        .name = "Khó", .scrollSpeed = 205.0f, .speedPerPoint = 3.0f, .maxSpeedBonus = 110.0f,
        .gap = 130.0f, .minGap = 110.0f, .gapShrinkPerPoint = 0.6f,
        .spacing = 245.0f, .maxGapDelta = 170.0f,
        .accent = {232, 76, 61, 255}
    },
};

const DifficultyDef *FlappyDifficultyGet(FlappyDifficulty diff)
{
    if (diff < 0 || diff >= FLAPPY_DIFF_COUNT) diff = FLAPPY_DIFF_NORMAL;
    return &s_difficulties[diff];
}

static float RandRange(float lo, float hi)
{
    return lo + (hi - lo) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

// ---------------------------------------------------------------------------
// Máy bay
// ---------------------------------------------------------------------------

void ResetPlane(Plane *plane)
{
    plane->pos = (Vector2){PLANE_X, 205.0f};
    plane->vy = 0.0f;
    plane->angle = 0.0f;
    plane->propTimer = 0.0f;
    plane->propFrame = 0;
    plane->trailTimer = 0.0f;
    plane->spinSpeed = 0.0f;
    plane->alive = true;
}

Vector2 PlaneExhaustPos(const Plane *plane)
{
    // Ống xả nằm phía sau thân (bên trái, vì máy bay luôn bay sang phải).
    Vector2 local = {-38.0f * PLANE_SCALE, 6.0f * PLANE_SCALE};
    Vector2 rotated = Vector2Rotate(local, plane->angle * DEG2RAD);
    return Vector2Add(plane->pos, rotated);
}

void AnimatePlane(Plane *plane, float dt, float rate)
{
    plane->propTimer += dt * rate;
    while (plane->propTimer >= 1.0f) {
        plane->propTimer -= 1.0f;
        plane->propFrame = (plane->propFrame + 1) % PLANE_FRAMES;
    }
}

void FlapPlane(FlappyGame *game)
{
    Plane *p = &game->plane;
    p->vy = FLAP_VELOCITY;
    p->propTimer += 0.5f;   // Cánh quạt quay dồn một nhịp cho cảm giác "tăng ga"

    Vector2 exhaust = PlaneExhaustPos(p);
    EmitPuff(game, exhaust, (Vector2){-60.0f, 30.0f}, true);
    EmitPuff(game, Vector2Add(exhaust, (Vector2){-6.0f, 8.0f}), (Vector2){-40.0f, 60.0f}, false);
    PlayFlappySfx(FLAPPY_SFX_FLAP, game->soundEnabled, 0.08f);
}

void UpdatePlaneFlight(FlappyGame *game, float dt)
{
    Plane *p = &game->plane;

    p->vy += GRAVITY * dt;
    if (p->vy > MAX_FALL_SPEED) p->vy = MAX_FALL_SPEED;
    p->pos.y += p->vy * dt;

    // Trần: không cho bay khuất khỏi màn hình. Đá treo đã kéo dài quá mép trên,
    // nên không thể lách qua trên đỉnh.
    if (p->pos.y < 4.0f) {
        p->pos.y = 4.0f;
        if (p->vy < 0.0f) p->vy = 0.0f;
    }

    // Ngóc mũi lên khi vừa vỗ, chúc dần xuống khi rơi - giống Flappy Bird.
    float target;
    if (p->vy < 0.0f) {
        target = -22.0f;
    } else {
        float t = Clamp((p->vy - 80.0f) / 560.0f, 0.0f, 1.0f);
        target = Lerp(-22.0f, 70.0f, t * t);
    }
    float follow = (p->vy < 0.0f) ? 16.0f : 5.0f;
    p->angle += (target - p->angle) * fminf(1.0f, dt * follow);

    AnimatePlane(p, dt, p->vy < 0.0f ? 30.0f : 18.0f);

    // Vệt khói mỏng phía sau
    p->trailTimer -= dt;
    if (p->trailTimer <= 0.0f) {
        p->trailTimer = 0.085f;
        EmitPuff(game, PlaneExhaustPos(p), (Vector2){-30.0f, RandRange(-12.0f, 12.0f)}, false);
    }
}

void UpdatePlaneHover(FlappyGame *game, float dt)
{
    Plane *p = &game->plane;
    p->pos.y = 205.0f + sinf(game->stateTime * 3.2f) * 9.0f;
    p->angle = sinf(game->stateTime * 3.2f + 1.2f) * 4.0f;
    p->vy = 0.0f;
    AnimatePlane(p, dt, 18.0f);

    p->trailTimer -= dt;
    if (p->trailTimer <= 0.0f) {
        p->trailTimer = 0.12f;
        EmitPuff(game, PlaneExhaustPos(p), (Vector2){-40.0f, RandRange(-10.0f, 10.0f)}, false);
    }
}

bool UpdatePlaneFalling(FlappyGame *game, float dt)
{
    Plane *p = &game->plane;
    p->vy += GRAVITY * dt;
    if (p->vy > MAX_FALL_SPEED * 1.2f) p->vy = MAX_FALL_SPEED * 1.2f;
    p->pos.y += p->vy * dt;
    p->angle += p->spinSpeed * dt;
    AnimatePlane(p, dt, 6.0f);

    // Khói đen bốc lên từ máy bay hỏng
    p->trailTimer -= dt;
    if (p->trailTimer <= 0.0f) {
        p->trailTimer = 0.05f;
        FlappyParticle *smoke = EmitPuff(game, p->pos, (Vector2){RandRange(-20.0f, 20.0f), -50.0f},
                                         GetRandomValue(0, 2) == 0);
        if (smoke) smoke->color = (Color){96, 96, 104, 255};
    }

    if (PlaneHitsGround(game)) {
        // Đặt máy bay nằm trên mặt đất thay vì lún xuống
        p->pos.y = GroundTopUnder(game, p->pos.x) - PLANE_RADIUS + 3.0f;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Địa hình
// ---------------------------------------------------------------------------

static float CurrentGap(const FlappyGame *game)
{
    const DifficultyDef *d = FlappyDifficultyGet(game->difficulty);
    return fmaxf(d->minGap, d->gap - (float)game->score * d->gapShrinkPerPoint);
}

static float CurrentSpeed(const FlappyGame *game)
{
    const DifficultyDef *d = FlappyDifficultyGet(game->difficulty);
    return d->scrollSpeed + fminf(d->maxSpeedBonus, (float)game->score * d->speedPerPoint);
}

void ResetWorld(FlappyGame *game)
{
    for (int i = 0; i < MAX_ROCK_PAIRS; i++) game->rocks[i].active = false;
    for (int i = 0; i < MAX_STARS; i++) game->stars[i].active = false;

    game->biome = 0;
    game->biomeBanner = 0.0f;
    for (int i = 0; i < GROUND_TILES; i++) {
        game->ground[i].x = (float)(i * GROUND_TEX_W);
        game->ground[i].biome = 0;
    }
    game->scrollSpeed = FlappyDifficultyGet(game->difficulty)->scrollSpeed;
    game->distanceToNextRock = 0.0f;
    game->lastGapCenter = (GROUND_Y) * 0.5f;
}

float GroundTopUnder(const FlappyGame *game, float worldX)
{
    for (int i = 0; i < GROUND_TILES; i++) {
        const GroundTile *t = &game->ground[i];
        if (worldX >= t->x && worldX < t->x + GROUND_TEX_W) {
            return FlappyGroundTopAt(t->biome, t->x, worldX);
        }
    }
    return (float)GROUND_Y + 36.0f;
}

void ScrollWorld(FlappyGame *game, float dt)
{
    float dx = game->scrollSpeed * dt;

    // Nền trời trôi chậm hơn đất -> hiệu ứng thị sai (parallax).
    game->bgOffset += dx * 0.28f;
    if (game->bgOffset > (float)WORLD_W) game->bgOffset -= (float)WORLD_W;

    for (int i = 0; i < GROUND_TILES; i++) {
        game->ground[i].x -= dx;
    }
    for (int i = 0; i < GROUND_TILES; i++) {
        GroundTile *t = &game->ground[i];
        if (t->x + GROUND_TEX_W <= 0.0f) {
            // Ghép ô đất vừa trôi khỏi màn hình vào sau ô xa nhất, mang theo vùng hiện tại.
            float farthest = t->x;
            for (int j = 0; j < GROUND_TILES; j++) {
                if (game->ground[j].x > farthest) farthest = game->ground[j].x;
            }
            t->x = farthest + GROUND_TEX_W;
            t->biome = game->biome;
        }
    }

    // Sắc trời chuyển mượt sang tông của vùng mới.
    Color target = FlappyBiome(game->biome)->sky;
    float k = fminf(1.0f, dt * 1.5f);
    game->skyTint.x = Lerp(game->skyTint.x, (float)target.r, k);
    game->skyTint.y = Lerp(game->skyTint.y, (float)target.g, k);
    game->skyTint.z = Lerp(game->skyTint.z, (float)target.b, k);
}

static void SpawnStar(FlappyGame *game, Vector2 pos)
{
    for (int i = 0; i < MAX_STARS; i++) {
        Star *s = &game->stars[i];
        if (s->active) continue;

        int roll = GetRandomValue(0, 99);
        s->kind = (roll < 8) ? STAR_GOLD : (roll < 30) ? STAR_SILVER : STAR_BRONZE;
        s->pos = pos;
        s->phase = RandRange(0.0f, 6.28f);
        s->active = true;
        return;
    }
}

static void SpawnRockPair(FlappyGame *game)
{
    RockPair *r = NULL;
    for (int i = 0; i < MAX_ROCK_PAIRS; i++) {
        if (!game->rocks[i].active) { r = &game->rocks[i]; break; }
    }
    if (!r) return;

    const DifficultyDef *d = FlappyDifficultyGet(game->difficulty);
    float gap = CurrentGap(game);

    float minCenter = TOP_TIP_MIN + gap * 0.5f;
    float maxCenter = BOTTOM_TIP_MAX - gap * 0.5f;
    float lo = fmaxf(minCenter, game->lastGapCenter - d->maxGapDelta);
    float hi = fminf(maxCenter, game->lastGapCenter + d->maxGapDelta);
    float center = RandRange(lo, hi);

    r->x = (float)WORLD_W + 20.0f;
    r->gap = gap;
    r->gapCenter = center;
    r->biome = game->biome;
    r->passed = false;
    r->active = true;
    r->hasTop = true;
    r->hasBottom = true;

    // Sau vài điểm đầu, thỉnh thoảng chỉ có một mũi đá cao vút để đổi nhịp.
    if (game->score >= 6 && GetRandomValue(0, 99) < 14) {
        if (GetRandomValue(0, 1) == 0) {
            r->hasTop = false;
            r->gapCenter = fminf(center + gap * 0.25f, maxCenter);
        } else {
            r->hasBottom = false;
            r->gapCenter = fmaxf(center - gap * 0.25f, minCenter);
        }
    }

    float topTip = r->gapCenter - gap * 0.5f;
    float bottomTip = r->gapCenter + gap * 0.5f;
    // Kéo dài sprite khi cần để đá treo luôn chạm trần và đá mọc luôn cắm xuống đất.
    r->topH = fmaxf((float)ROCK_H, topTip + 40.0f);
    r->bottomH = fmaxf((float)ROCK_H, (float)WORLD_H + 10.0f - bottomTip);

    // Sao nằm giữa cặp đá trước và cặp đá này: muốn ăn phải chấp nhận rủi ro.
    if (GetRandomValue(0, 99) < 35) {
        float sy = (game->lastGapCenter + center) * 0.5f + RandRange(-40.0f, 40.0f);
        sy = Clamp(sy, 60.0f, (float)GROUND_Y - 70.0f);
        SpawnStar(game, (Vector2){r->x - d->spacing * 0.5f + ROCK_TIP_X_UP, sy});
    }

    game->lastGapCenter = r->gapCenter;
}

int StarValue(StarKind kind)
{
    switch (kind) {
        case STAR_GOLD:   return 3;
        case STAR_SILVER: return 2;
        default:          return 1;
    }
}

Color StarColor(StarKind kind)
{
    switch (kind) {
        case STAR_GOLD:   return (Color){255, 214, 64, 255};
        case STAR_SILVER: return (Color){214, 226, 240, 255};
        default:          return (Color){226, 150, 92, 255};
    }
}

MedalKind MedalForScore(int score)
{
    if (score >= 50) return MEDAL_GOLD;
    if (score >= 25) return MEDAL_SILVER;
    if (score >= 10) return MEDAL_BRONZE;
    return MEDAL_NONE;
}

void UpdateObstacles(FlappyGame *game, float dt)
{
    game->scrollSpeed = CurrentSpeed(game);
    float dx = game->scrollSpeed * dt;

    game->distanceToNextRock -= dx;
    if (game->distanceToNextRock <= 0.0f) {
        SpawnRockPair(game);
        game->distanceToNextRock += FlappyDifficultyGet(game->difficulty)->spacing;
    }

    for (int i = 0; i < MAX_ROCK_PAIRS; i++) {
        RockPair *r = &game->rocks[i];
        if (!r->active) continue;
        r->x -= dx;

        if (!r->passed && r->x + ROCK_TIP_X_UP < game->plane.pos.x) {
            r->passed = true;
            game->score++;
            game->rocksPassed++;
            PlayFlappySfx(FLAPPY_SFX_SCORE, game->soundEnabled, 0.0f);
        }
        if (r->x + ROCK_W < -20.0f) r->active = false;
    }

    // Nhặt sao
    Vector2 pc = game->plane.pos;
    for (int i = 0; i < MAX_STARS; i++) {
        Star *s = &game->stars[i];
        if (!s->active) continue;
        s->pos.x -= dx;
        s->phase += dt;

        Vector2 sp = {s->pos.x, s->pos.y + sinf(s->phase * 3.0f) * 6.0f};
        if (CheckCollisionCircles(pc, PLANE_RADIUS + 6.0f, sp, 17.0f)) {
            s->active = false;
            int value = StarValue(s->kind);
            game->score += value;
            game->starsCollected++;
            EmitSparkles(game, sp, StarColor(s->kind), 14 + value * 3);
            AddFloatText(game, TextFormat("+%d", value), (Vector2){sp.x, sp.y - 20.0f}, StarColor(s->kind));
            PlayFlappySfx(FLAPPY_SFX_STAR, game->soundEnabled, 0.04f);
            continue;
        }
        if (s->pos.x < -40.0f) s->active = false;
    }
}

// ---------------------------------------------------------------------------
// Va chạm
// ---------------------------------------------------------------------------

static float SegmentDistSq(Vector2 p, Vector2 a, Vector2 b)
{
    Vector2 ab = Vector2Subtract(b, a);
    float len2 = Vector2LengthSqr(ab);
    float t = (len2 > 0.0f) ? Clamp(Vector2DotProduct(Vector2Subtract(p, a), ab) / len2, 0.0f, 1.0f) : 0.0f;
    Vector2 closest = Vector2Add(a, Vector2Scale(ab, t));
    return Vector2DistanceSqr(p, closest);
}

static bool CircleTriangle(Vector2 c, float r, Vector2 a, Vector2 b, Vector2 d)
{
    if (CheckCollisionPointTriangle(c, a, b, d)) return true;
    float r2 = r * r;
    return SegmentDistSq(c, a, b) < r2 || SegmentDistSq(c, b, d) < r2 || SegmentDistSq(c, d, a) < r2;
}

bool PlaneHitsRock(const FlappyGame *game)
{
    Vector2 c = game->plane.pos;
    float r = PLANE_RADIUS;

    for (int i = 0; i < MAX_ROCK_PAIRS; i++) {
        const RockPair *rp = &game->rocks[i];
        if (!rp->active) continue;
        if (c.x + r < rp->x || c.x - r > rp->x + ROCK_W) continue;

        float topTip = rp->gapCenter - rp->gap * 0.5f;
        float bottomTip = rp->gapCenter + rp->gap * 0.5f;

        // Hitbox tam giác bám theo hình mũi đá (thay vì hình chữ nhật như ống nước).
        if (rp->hasBottom) {
            Vector2 tip = {rp->x + ROCK_TIP_X_UP, bottomTip + ROCK_TIP_FORGIVE};
            Vector2 bl = {rp->x + ROCK_BASE_L, bottomTip + rp->bottomH};
            Vector2 br = {rp->x + ROCK_BASE_R, bottomTip + rp->bottomH};
            if (CircleTriangle(c, r, tip, bl, br)) return true;
        }
        if (rp->hasTop) {
            Vector2 tip = {rp->x + ROCK_TIP_X_DOWN, topTip - ROCK_TIP_FORGIVE};
            Vector2 tl = {rp->x + ROCK_BASE_L, topTip - rp->topH};
            Vector2 tr = {rp->x + ROCK_BASE_R, topTip - rp->topH};
            if (CircleTriangle(c, r, tl, tr, tip)) return true;
        }
    }
    return false;
}

bool PlaneHitsGround(const FlappyGame *game)
{
    Vector2 c = game->plane.pos;
    float r = PLANE_RADIUS;
    // Lấy mẫu dọc nửa dưới của vòng tròn hitbox so với đường viền đồi.
    for (float dx = -r; dx <= r; dx += 3.0f) {
        float bottom = c.y + sqrtf(fmaxf(0.0f, r * r - dx * dx));
        if (bottom >= GroundTopUnder(game, c.x + dx) + 3.0f) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Hiệu ứng hạt & chữ nổi
// ---------------------------------------------------------------------------

static FlappyParticle *AllocParticle(FlappyGame *game)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!game->particles[i].active) return &game->particles[i];
    }
    return NULL;
}

void ClearEffects(FlappyGame *game)
{
    for (int i = 0; i < MAX_PARTICLES; i++) game->particles[i].active = false;
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) game->floatTexts[i].active = false;
}

FlappyParticle *EmitPuff(FlappyGame *game, Vector2 pos, Vector2 vel, bool large)
{
    FlappyParticle *p = AllocParticle(game);
    if (!p) return NULL;
    p->kind = PARTICLE_PUFF;
    p->pos = pos;
    p->vel = vel;
    p->color = WHITE;
    p->large = large;
    p->size = large ? RandRange(0.45f, 0.6f) : RandRange(0.35f, 0.5f);
    p->rotation = RandRange(0.0f, 360.0f);
    p->spin = RandRange(-60.0f, 60.0f);
    p->maxLife = p->life = large ? 0.75f : 0.55f;
    p->active = true;
    return p;
}

void EmitSparkles(FlappyGame *game, Vector2 pos, Color color, int count)
{
    for (int i = 0; i < count; i++) {
        FlappyParticle *p = AllocParticle(game);
        if (!p) return;
        float a = RandRange(0.0f, 2.0f * PI);
        float s = RandRange(80.0f, 240.0f);
        p->kind = PARTICLE_SPARK;
        p->pos = pos;
        p->vel = (Vector2){cosf(a) * s, sinf(a) * s - 40.0f};
        p->color = (i % 3 == 0) ? WHITE : color;
        p->size = RandRange(3.0f, 6.0f);
        p->rotation = RandRange(0.0f, 90.0f);
        p->spin = RandRange(-360.0f, 360.0f);
        p->maxLife = p->life = RandRange(0.45f, 0.8f);
        p->large = false;
        p->active = true;
    }
}

void EmitCrash(FlappyGame *game, Vector2 pos)
{
    for (int i = 0; i < 6; i++) {
        float a = RandRange(0.0f, 2.0f * PI);
        EmitPuff(game, Vector2Add(pos, (Vector2){cosf(a) * 10.0f, sinf(a) * 10.0f}),
                 (Vector2){cosf(a) * 90.0f, sinf(a) * 90.0f - 30.0f}, true);
    }
    static const Color debris[] = {
        {120, 92, 64, 255}, {160, 130, 96, 255}, {200, 200, 210, 255}, {90, 90, 100, 255}
    };
    for (int i = 0; i < 18; i++) {
        FlappyParticle *p = AllocParticle(game);
        if (!p) return;
        float a = RandRange(-PI, 0.2f);
        float s = RandRange(120.0f, 320.0f);
        p->kind = PARTICLE_DEBRIS;
        p->pos = pos;
        p->vel = (Vector2){cosf(a) * s, sinf(a) * s};
        p->color = debris[i % 4];
        p->size = RandRange(3.0f, 7.0f);
        p->rotation = RandRange(0.0f, 360.0f);
        p->spin = RandRange(-540.0f, 540.0f);
        p->maxLife = p->life = RandRange(0.7f, 1.3f);
        p->large = false;
        p->active = true;
    }
}

void AddFloatText(FlappyGame *game, const char *text, Vector2 pos, Color color)
{
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        FlappyFloatText *f = &game->floatTexts[i];
        if (f->active) continue;
        strncpy(f->text, text, sizeof(f->text) - 1);
        f->text[sizeof(f->text) - 1] = '\0';
        f->pos = pos;
        f->color = color;
        f->maxLife = f->life = 0.9f;
        f->active = true;
        return;
    }
}

void UpdateEffects(FlappyGame *game, float dt, float worldScroll)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        FlappyParticle *p = &game->particles[i];
        if (!p->active) continue;

        p->life -= dt;
        if (p->life <= 0.0f) { p->active = false; continue; }

        // Hạt thuộc về thế giới nên trôi theo nhịp cuộn của mặt đất.
        p->pos.x += (p->vel.x - worldScroll) * dt;
        p->pos.y += p->vel.y * dt;
        p->rotation += p->spin * dt;

        switch (p->kind) {
            case PARTICLE_PUFF:
                p->vel = Vector2Scale(p->vel, 1.0f - fminf(1.0f, dt * 2.5f));
                p->vel.y -= 20.0f * dt;
                p->size += dt * (p->large ? 0.9f : 0.6f);
                break;
            case PARTICLE_SPARK:
                p->vel.y += 380.0f * dt;
                p->vel = Vector2Scale(p->vel, 1.0f - fminf(1.0f, dt * 1.5f));
                break;
            case PARTICLE_DEBRIS: {
                p->vel.y += 900.0f * dt;
                float ground = GroundTopUnder(game, p->pos.x) + 6.0f;
                if (p->pos.y > ground) {
                    p->pos.y = ground;
                    p->vel.y *= -0.35f;
                    p->vel.x *= 0.6f;
                    p->spin *= 0.5f;
                }
                break;
            }
        }
    }

    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        FlappyFloatText *f = &game->floatTexts[i];
        if (!f->active) continue;
        f->life -= dt;
        f->pos.y -= 46.0f * dt;
        f->pos.x -= worldScroll * dt * 0.5f;
        if (f->life <= 0.0f) f->active = false;
    }
}
