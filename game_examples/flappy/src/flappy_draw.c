#include "flappy_draw.h"
#include "flappy_assets.h"
#include "flappy_world.h"
#include "font_vn.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Bảng màu khớp với tông hoạt hình của bộ Tappy Plane.
#define INK        (Color){ 92,  64,  40, 255}
#define INK_SOFT   (Color){150, 118,  82, 255}
#define CARD_BG    (Color){246, 236, 210, 255}
#define CARD_EDGE  (Color){204, 178, 134, 255}
#define GOLD_TEXT  (Color){255, 206,  64, 255}
#define WHITE_SOFT (Color){255, 255, 255, 235}

// ---------------------------------------------------------------------------
// Bố cục (toạ độ canvas 1280x768)
// ---------------------------------------------------------------------------

static const Rectangle MENU_PANEL = {240.0f, 186.0f, 800.0f, 446.0f};

Rectangle FlappyMenuPlaneCard(int index)
{
    // Chiều rộng thẻ co giãn theo số lượng máy bay: thêm skin mới không cần sửa UI.
    int count = FlappyPlaneSkinCount();
    float gap = 22.0f;
    float avail = MENU_PANEL.width - 80.0f;
    float w = fminf(160.0f, (avail - gap * (float)(count - 1)) / (float)count);
    float total = w * (float)count + gap * (float)(count - 1);
    float x0 = MENU_PANEL.x + (MENU_PANEL.width - total) * 0.5f;
    return (Rectangle){x0 + (float)index * (w + gap), MENU_PANEL.y + 62.0f, w, 138.0f};
}

Rectangle FlappyMenuDifficultyChip(int index)
{
    float w = 200.0f, h = 66.0f, gap = 24.0f;
    float total = w * FLAPPY_DIFF_COUNT + gap * (FLAPPY_DIFF_COUNT - 1);
    float x0 = MENU_PANEL.x + (MENU_PANEL.width - total) * 0.5f;
    return (Rectangle){x0 + (float)index * (w + gap), MENU_PANEL.y + 262.0f, w, h};
}

Rectangle FlappyMenuStartButton(void)
{
    float w = 250.0f, h = 76.0f;
    return (Rectangle){MENU_PANEL.x + (MENU_PANEL.width - w) * 0.5f, MENU_PANEL.y + 352.0f, w, h};
}

Rectangle FlappyGameOverButton(int index)
{
    float w = 220.0f, h = 72.0f, gap = 28.0f;
    float x0 = (CANVAS_W - (w * 2.0f + gap)) * 0.5f;
    return (Rectangle){x0 + (float)index * (w + gap), 548.0f, w, h};
}

// ---------------------------------------------------------------------------
// Tiện ích vẽ
// ---------------------------------------------------------------------------

static Vector2 WorldToCanvas(Vector2 p)
{
    return (Vector2){p.x * WORLD_ZOOM, p.y * WORLD_ZOOM};
}

static float EaseOutBack(float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

static float EaseOutCubic(float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

static void DrawSpriteCentered(Texture2D tex, Vector2 center, float scale, float rotation, Color tint)
{
    Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
    Rectangle dst = {center.x, center.y, tex.width * scale, tex.height * scale};
    DrawTexturePro(tex, src, dst, (Vector2){dst.width * 0.5f, dst.height * 0.5f}, rotation, tint);
}

// Tấm nền UIbg của Kenney kéo giãn kiểu 9-slice để góc bo không bị méo.
static void DrawPanel(Rectangle r, Color tint)
{
    const FlappyAssets *a = FlappyAssetsGet();
    NPatchInfo info = {
        .source = {0.0f, 0.0f, (float)a->uiPanel.width, (float)a->uiPanel.height},
        .left = 18, .top = 18, .right = 18, .bottom = 18,
        .layout = NPATCH_NINE_PATCH
    };
    DrawTextureNPatch(a->uiPanel, info, r, (Vector2){0.0f, 0.0f}, 0.0f, tint);
}

static void DrawTextCentered(const char *text, float cx, float y, float size, Color color, bool bold)
{
    Vector2 m = bold ? MeasureTextVNBoldPro(text, size, 1.0f) : MeasureTextVNPro(text, size, 1.0f);
    Vector2 pos = {roundf(cx - m.x * 0.5f), roundf(y)};
    if (bold) DrawTextVNBoldPro(text, pos, size, 1.0f, color);
    else      DrawTextVNPro(text, pos, size, 1.0f, color);
}

// Chữ có viền dày - dùng cho chữ nằm trực tiếp trên nền trời.
static void DrawTextOutlined(const char *text, Vector2 pos, float size, Color fill, Color outline, float thick)
{
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            DrawTextVNBoldPro(text, (Vector2){pos.x + dx * thick, pos.y + dy * thick}, size, 1.0f, outline);
        }
    }
    DrawTextVNBoldPro(text, pos, size, 1.0f, fill);
}

static void DrawTextOutlinedCentered(const char *text, float cx, float y, float size, Color fill, Color outline, float thick)
{
    Vector2 m = MeasureTextVNBoldPro(text, size, 1.0f);
    DrawTextOutlined(text, (Vector2){roundf(cx - m.x * 0.5f), roundf(y)}, size, fill, outline, thick);
}

// Vẽ số bằng bộ chữ số sprite của Kenney. Trả về chiều rộng đã vẽ.
static float MeasureNumberSprites(int value, float scale)
{
    const FlappyAssets *a = FlappyAssetsGet();
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    float w = 0.0f;
    for (int i = 0; buf[i]; i++) w += (a->numbers[buf[i] - '0'].width - 6.0f) * scale;
    return w;
}

static void DrawNumberSprites(int value, float cx, float top, float scale, Color tint)
{
    const FlappyAssets *a = FlappyAssetsGet();
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    float x = cx - MeasureNumberSprites(value, scale) * 0.5f;
    for (int i = 0; buf[i]; i++) {
        Texture2D t = a->numbers[buf[i] - '0'];
        Rectangle src = {0.0f, 0.0f, (float)t.width, (float)t.height};
        Rectangle dst = {x - 3.0f * scale, top, t.width * scale, t.height * scale};
        DrawTexturePro(t, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, tint);
        x += (t.width - 6.0f) * scale;
    }
}

// Tiêu đề ghép từ các chữ cái sprite, mỗi chữ nhún nhảy lệch pha.
static void DrawLetterTitle(const char *text, float cx, float y, float scale, float time)
{
    const FlappyAssets *a = FlappyAssetsGet();
    float spaceW = 28.0f * scale;
    float total = 0.0f;
    for (int i = 0; text[i]; i++) {
        if (text[i] == ' ') { total += spaceW; continue; }
        total += (a->letters[text[i] - 'A'].width - 4.0f) * scale;
    }

    float x = cx - total * 0.5f;
    for (int i = 0; text[i]; i++) {
        if (text[i] == ' ') { x += spaceW; continue; }
        Texture2D t = a->letters[text[i] - 'A'];
        float bob = sinf(time * 3.0f + (float)i * 0.45f) * 6.0f;
        float rot = sinf(time * 2.2f + (float)i * 0.7f) * 4.0f;
        Vector2 c = {x + t.width * scale * 0.5f, y + t.height * scale * 0.5f + bob};
        DrawSpriteCentered(t, (Vector2){c.x + 3.0f, c.y + 5.0f}, scale, rot, (Color){0, 0, 0, 50});
        DrawSpriteCentered(t, c, scale, rot, WHITE);
        x += (t.width - 4.0f) * scale;
    }
}

static void DrawPlaneSprite(int skin, int frame, Vector2 center, float scale, float angle, Color tint)
{
    const FlappyAssets *a = FlappyAssetsGet();
    int s = skin % FlappyPlaneSkinCount();
    DrawSpriteCentered(a->plane[s][frame % PLANE_FRAMES], center, scale, angle, tint);
}

// Nút vàng của Kenney (buttonLarge) kéo giãn 9-slice + nhãn chữ.
static void DrawKenneyButton(Rectangle r, const char *label, bool hovered, bool primary)
{
    const FlappyAssets *a = FlappyAssetsGet();
    float lift = hovered ? -3.0f : 0.0f;
    Rectangle dst = {r.x, r.y + lift, r.width, r.height};

    DrawRectangleRounded((Rectangle){r.x + 4.0f, r.y + 8.0f, r.width, r.height}, 0.3f, 8, (Color){0, 0, 0, 45});
    if (primary) {
        NPatchInfo info = {
            .source = {0.0f, 0.0f, (float)a->buttonLarge.width, (float)a->buttonLarge.height},
            .left = 16, .top = 14, .right = 16, .bottom = 18,
            .layout = NPATCH_NINE_PATCH
        };
        DrawTextureNPatch(a->buttonLarge, info, dst, (Vector2){0.0f, 0.0f}, 0.0f,
                          hovered ? WHITE : (Color){238, 232, 222, 255});
    } else {
        DrawPanel(dst, hovered ? WHITE : (Color){236, 228, 214, 255});
    }

    float fs = r.height * 0.36f;
    DrawTextCentered(label, r.x + r.width * 0.5f, dst.y + (r.height - fs) * 0.5f - 4.0f, fs, INK, true);
}

static bool Hovered(Rectangle r)
{
    return CheckCollisionPointRec(GetMousePosition(), r);
}

// ---------------------------------------------------------------------------
// Thế giới
// ---------------------------------------------------------------------------

static void DrawBackground(const FlappyGame *game)
{
    const FlappyAssets *a = FlappyAssetsGet();
    Color tint = {(unsigned char)game->skyTint.x, (unsigned char)game->skyTint.y, (unsigned char)game->skyTint.z, 255};
    float x0 = -fmodf(game->bgOffset, (float)WORLD_W);
    // Ba bản sao liền nhau để lúc rung màn hình không lộ mép.
    for (int i = -1; i <= 1; i++) {
        Rectangle src = {0.0f, 0.0f, (float)a->background.width, (float)a->background.height};
        Rectangle dst = {x0 + (float)(i * WORLD_W), -8.0f, (float)WORLD_W + 1.0f, (float)WORLD_H + 8.0f};
        DrawTexturePro(a->background, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, tint);
    }
}

static void DrawRocks(const FlappyGame *game)
{
    const FlappyAssets *a = FlappyAssetsGet();
    for (int i = 0; i < MAX_ROCK_PAIRS; i++) {
        const RockPair *r = &game->rocks[i];
        if (!r->active) continue;
        int b = r->biome % FlappyBiomeCount();
        float topTip = r->gapCenter - r->gap * 0.5f;
        float bottomTip = r->gapCenter + r->gap * 0.5f;
        Rectangle src = {0.0f, 0.0f, (float)ROCK_W, (float)ROCK_H};

        if (r->hasTop) {
            Rectangle dst = {r->x, topTip - r->topH, (float)ROCK_W, r->topH};
            DrawTexturePro(a->rockDown[b], src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }
        if (r->hasBottom) {
            Rectangle dst = {r->x, bottomTip, (float)ROCK_W, r->bottomH};
            DrawTexturePro(a->rock[b], src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }
    }
}

static void DrawStars(const FlappyGame *game)
{
    const FlappyAssets *a = FlappyAssetsGet();
    for (int i = 0; i < MAX_STARS; i++) {
        const Star *s = &game->stars[i];
        if (!s->active) continue;

        Vector2 p = {s->pos.x, s->pos.y + sinf(s->phase * 3.0f) * 6.0f};
        float pulse = 0.5f + 0.5f * sinf(s->phase * 5.0f);
        Color glow = StarColor(s->kind);
        glow.a = (unsigned char)(50 + 40 * pulse);
        DrawCircleV(p, 22.0f + pulse * 4.0f, glow);

        // Giả lập xoay 3D bằng cách bóp chiều ngang theo cos
        Texture2D t = a->star[s->kind];
        float squash = fmaxf(0.25f, fabsf(cosf(s->phase * 2.4f)));
        Rectangle src = {0.0f, 0.0f, (float)t.width, (float)t.height};
        Rectangle dst = {p.x, p.y, t.width * squash, (float)t.height};
        DrawTexturePro(t, src, dst, (Vector2){dst.width * 0.5f, dst.height * 0.5f}, 0.0f, WHITE);
    }
}

static void DrawParticles(const FlappyGame *game, bool puffs)
{
    const FlappyAssets *a = FlappyAssetsGet();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const FlappyParticle *p = &game->particles[i];
        if (!p->active) continue;
        if ((p->kind == PARTICLE_PUFF) != puffs) continue;

        float t = p->life / p->maxLife;
        Color c = p->color;
        c.a = (unsigned char)(c.a * fminf(1.0f, t * 1.6f));

        switch (p->kind) {
            case PARTICLE_PUFF:
                DrawSpriteCentered(p->large ? a->puffLarge : a->puffSmall, p->pos, p->size, p->rotation, c);
                break;
            case PARTICLE_SPARK:
                DrawPoly(p->pos, 4, p->size * (0.4f + 0.6f * t), p->rotation, c);
                break;
            case PARTICLE_DEBRIS:
                DrawRectanglePro((Rectangle){p->pos.x, p->pos.y, p->size, p->size * 0.7f},
                                 (Vector2){p->size * 0.5f, p->size * 0.35f}, p->rotation, c);
                break;
        }
    }
}

static void DrawGround(const FlappyGame *game)
{
    const FlappyAssets *a = FlappyAssetsGet();
    for (int i = 0; i < GROUND_TILES; i++) {
        const GroundTile *t = &game->ground[i];
        int b = t->biome % FlappyBiomeCount();
        // Lấp dải màu đất phía dưới để rung màn hình không lộ viền đen.
        DrawRectangleRec((Rectangle){t->x, (float)WORLD_H - 2.0f, (float)GROUND_TEX_W + 1.0f, 24.0f}, a->groundBase[b]);
        // Rộng hơn 1.5px để hai ô liền kề luôn chồng mép, không lộ khe do sai số float.
        Rectangle src = {0.0f, 0.0f, (float)GROUND_TEX_W, (float)GROUND_TEX_H};
        Rectangle dst = {t->x, (float)GROUND_Y, (float)GROUND_TEX_W + 1.5f, (float)GROUND_TEX_H};
        DrawTexturePro(a->ground[b], src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    }
}

static void DrawWorld(const FlappyGame *game)
{
    DrawBackground(game);
    DrawRocks(game);
    DrawStars(game);
    DrawParticles(game, true);

    const Plane *p = &game->plane;
    bool showPlane = game->state != FLAPPY_STATE_MENU;
    if (showPlane) {
        DrawPlaneSprite(game->planeSkin, p->propFrame, p->pos, PLANE_SCALE, p->angle, WHITE);
    }

    DrawGround(game);
    DrawParticles(game, false);
}

static void DrawFloatTexts(const FlappyGame *game)
{
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        const FlappyFloatText *f = &game->floatTexts[i];
        if (!f->active) continue;
        float t = f->life / f->maxLife;
        Vector2 c = WorldToCanvas(f->pos);
        Color fill = f->color;
        Color outline = INK;
        fill.a = outline.a = (unsigned char)(255 * fminf(1.0f, t * 2.0f));
        float size = 30.0f + (1.0f - t) * 6.0f;
        DrawTextOutlinedCentered(f->text, c.x, c.y, size, fill, outline, 2.5f);
    }
}

// ---------------------------------------------------------------------------
// HUD trong lúc chơi
// ---------------------------------------------------------------------------

static void DrawInfoChip(float x, float y, const char *label, const char *value, Color valueColor, Texture2D *icon)
{
    float fs = 18.0f;
    float labelW = MeasureTextVNPro(label, 15.0f, 1.0f).x;
    float valueW = MeasureTextVNBoldPro(value, fs, 1.0f).x;
    float iconW = icon ? 30.0f : 0.0f;
    Rectangle r = {x, y, 24.0f + iconW + labelW + 10.0f + valueW, 40.0f};

    DrawRectangleRounded((Rectangle){r.x + 2.0f, r.y + 4.0f, r.width, r.height}, 0.5f, 8, (Color){0, 0, 0, 40});
    DrawRectangleRounded(r, 0.5f, 8, (Color){255, 250, 238, 230});
    DrawRectangleRoundedLinesEx(r, 0.5f, 8, 2.0f, CARD_EDGE);

    float cx = r.x + 12.0f;
    if (icon) {
        DrawSpriteCentered(*icon, (Vector2){cx + 12.0f, r.y + r.height * 0.5f}, 0.62f, 0.0f, WHITE);
        cx += iconW;
    }
    DrawTextVNPro(label, (Vector2){cx, r.y + 12.0f}, 15.0f, 1.0f, INK_SOFT);
    DrawTextVNBoldPro(value, (Vector2){cx + labelW + 10.0f, r.y + 10.0f}, fs, 1.0f, valueColor);
}

static void DrawPlayHud(const FlappyGame *game)
{
    const FlappyAssets *a = FlappyAssetsGet();

    // Điểm lớn ở giữa, nảy nhẹ mỗi khi tăng
    float pop = fmaxf(0.0f, game->scorePop);
    float scale = 0.95f + 0.22f * pop * pop;
    float h = 78.0f * scale;
    DrawNumberSprites(game->score, CANVAS_W * 0.5f + 3.0f, 30.0f + 5.0f - (h - 78.0f * 0.95f) * 0.5f, scale, (Color){0, 0, 0, 50});
    DrawNumberSprites(game->score, CANVAS_W * 0.5f, 30.0f - (h - 78.0f * 0.95f) * 0.5f, scale, WHITE);

    // Góc trái: sao đã nhặt, kỷ lục, vùng hiện tại
    Texture2D star = a->star[STAR_GOLD];
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", game->starsCollected);
    DrawInfoChip(20.0f, 20.0f, "Sao", buf, (Color){214, 150, 20, 255}, &star);

    // Kỷ lục nhảy theo điểm hiện tại ngay khi vượt qua, không đợi tới lúc thua.
    int best = game->best[game->difficulty];
    bool beating = game->score > best && best > 0;
    snprintf(buf, sizeof(buf), "%d", beating ? game->score : best);
    DrawInfoChip(20.0f, 70.0f, beating ? "Kỷ lục mới" : "Kỷ lục", buf,
                 beating ? (Color){232, 76, 61, 255} : INK, NULL);

    DrawInfoChip(20.0f, 120.0f, "Vùng", FlappyBiome(game->biome)->name,
                 FlappyDifficultyGet(game->difficulty)->accent, NULL);

    // Dải thông báo khi bay sang vùng mới
    if (game->biomeBanner > 0.0f) {
        float life = 2.6f - game->biomeBanner;
        float in = EaseOutBack(life / 0.45f);
        float out = Clamp(game->biomeBanner / 0.4f, 0.0f, 1.0f);
        float alpha = fminf(1.0f, out);
        const char *title = TextFormat("VÙNG MỚI · %s", FlappyBiome(game->biome)->name);
        float fs = 30.0f;
        float w = MeasureTextVNBoldPro(title, fs, 1.0f).x + 60.0f;
        float y = 150.0f;
        Rectangle r = {CANVAS_W * 0.5f - w * 0.5f * in, y, w * in, 58.0f};
        if (r.width > 4.0f) {
            Color accent = FlappyDifficultyGet(game->difficulty)->accent;
            DrawRectangleRounded((Rectangle){r.x + 3.0f, r.y + 6.0f, r.width, r.height}, 0.5f, 8, Fade(BLACK, 0.2f * alpha));
            DrawRectangleRounded(r, 0.5f, 8, Fade((Color){255, 250, 238, 255}, alpha));
            DrawRectangleRoundedLinesEx(r, 0.5f, 8, 3.0f, Fade(accent, alpha));
            if (in > 0.8f) DrawTextCentered(title, CANVAS_W * 0.5f, y + 13.0f, fs, Fade(INK, alpha), true);
        }
    }

    // Gợi ý phím ở mép dưới (nằm trên dải đất)
    const char *hint = TextFormat("Space / ↑ / Click  Vỗ cánh     P  Tạm dừng     M  Âm thanh: %s",
                                  game->soundEnabled ? "BẬT" : "TẮT");
    DrawTextOutlinedCentered(hint, CANVAS_W * 0.5f, CANVAS_H - 34.0f, 16.0f, WHITE_SOFT, (Color){70, 50, 30, 200}, 1.5f);
}

// ---------------------------------------------------------------------------
// Các màn hình
// ---------------------------------------------------------------------------

static void DrawMenu(const FlappyGame *game)
{
    DrawLetterTitle("FLAPPY PLANE", CANVAS_W * 0.5f, 52.0f, 1.0f, game->globalTime);
    DrawTextOutlinedCentered("Luồn lách qua những mỏm đá, nhặt sao và giành huy chương!",
                             CANVAS_W * 0.5f, 138.0f, 20.0f, WHITE, (Color){70, 90, 120, 220}, 2.0f);

    DrawPanel(MENU_PANEL, WHITE);

    // --- Chọn máy bay
    DrawTextCentered("CHỌN MÁY BAY", MENU_PANEL.x + MENU_PANEL.width * 0.5f, MENU_PANEL.y + 26.0f, 20.0f, INK_SOFT, true);
    int skinCount = FlappyPlaneSkinCount();
    for (int i = 0; i < skinCount; i++) {
        Rectangle r = FlappyMenuPlaneCard(i);
        bool selected = (i == game->planeSkin);
        bool hover = Hovered(r);
        const PlaneSkinDef *skin = FlappyPlaneSkin(i);

        if (selected) {
            DrawRectangleRounded((Rectangle){r.x - 5.0f, r.y - 5.0f, r.width + 10.0f, r.height + 10.0f}, 0.2f, 8, Fade(skin->accent, 0.35f));
        }
        DrawRectangleRounded(r, 0.18f, 8, selected ? (Color){255, 252, 240, 255} : CARD_BG);
        DrawRectangleRoundedLinesEx(r, 0.18f, 8, selected ? 3.5f : 2.0f,
                                    selected ? skin->accent : (hover ? INK_SOFT : CARD_EDGE));

        float bob = selected ? sinf(game->globalTime * 4.0f) * 5.0f : 0.0f;
        int frame = selected ? (int)(game->globalTime * 20.0f) : 0;
        float angle = selected ? sinf(game->globalTime * 3.0f) * 5.0f : 0.0f;
        float scale = fminf(1.0f, (r.width - 24.0f) / (float)PLANE_TEX_W) * (selected ? 1.0f : 0.85f);
        Color tint = (selected || hover) ? WHITE : (Color){225, 225, 225, 255};
        DrawPlaneSprite(i, frame, (Vector2){r.x + r.width * 0.5f, r.y + 56.0f + bob}, scale, angle, tint);

        DrawTextCentered(skin->name, r.x + r.width * 0.5f, r.y + r.height - 34.0f, 17.0f,
                         selected ? INK : INK_SOFT, selected);
    }

    // --- Chọn độ khó
    DrawTextCentered("ĐỘ KHÓ", MENU_PANEL.x + MENU_PANEL.width * 0.5f, MENU_PANEL.y + 228.0f, 20.0f, INK_SOFT, true);
    for (int i = 0; i < FLAPPY_DIFF_COUNT; i++) {
        Rectangle r = FlappyMenuDifficultyChip(i);
        const DifficultyDef *d = FlappyDifficultyGet((FlappyDifficulty)i);
        bool selected = (i == (int)game->difficulty);
        bool hover = Hovered(r);

        Color bg = selected ? d->accent : CARD_BG;
        DrawRectangleRounded((Rectangle){r.x + 2.0f, r.y + 5.0f, r.width, r.height}, 0.35f, 8, (Color){0, 0, 0, 35});
        DrawRectangleRounded(r, 0.35f, 8, bg);
        DrawRectangleRoundedLinesEx(r, 0.35f, 8, 2.0f, selected ? Fade(INK, 0.6f) : (hover ? INK_SOFT : CARD_EDGE));

        Color fg = selected ? WHITE : INK;
        DrawTextCentered(TextFormat("%d · %s", i + 1, d->name), r.x + r.width * 0.5f, r.y + 10.0f, 22.0f, fg, true);
        DrawTextCentered(TextFormat("Kỷ lục %d", game->best[i]), r.x + r.width * 0.5f, r.y + 40.0f, 15.0f,
                         selected ? Fade(WHITE, 0.9f) : INK_SOFT, false);
    }

    // --- Nút bắt đầu
    Rectangle start = FlappyMenuStartButton();
    float pulse = 1.0f + sinf(game->globalTime * 4.0f) * 0.02f;
    Rectangle pulsed = {
        start.x - start.width * (pulse - 1.0f) * 0.5f, start.y - start.height * (pulse - 1.0f) * 0.5f,
        start.width * pulse, start.height * pulse
    };
    DrawKenneyButton(pulsed, "BẮT ĐẦU", Hovered(start), true);

    const char *hint = TextFormat("← →  Chọn máy bay     ↑ ↓ / 1-3  Độ khó     Enter / Space  Bắt đầu     M  Âm thanh: %s",
                                  game->soundEnabled ? "BẬT" : "TẮT");
    DrawTextOutlinedCentered(hint, CANVAS_W * 0.5f, CANVAS_H - 58.0f, 16.0f, WHITE_SOFT, (Color){70, 50, 30, 200}, 1.5f);
}

static void DrawReady(const FlappyGame *game)
{
    const FlappyAssets *a = FlappyAssetsGet();
    DrawPlayHud(game);

    float appear = EaseOutBack(game->stateTime / 0.5f);
    DrawSpriteCentered(a->textGetReady, (Vector2){CANVAS_W * 0.5f, 190.0f}, 0.9f * appear, 0.0f, WHITE);

    // Mũi tên "TAP" chỉ vào máy bay + bàn tay nhấp nháy như ảnh mẫu của Kenney
    Vector2 plane = WorldToCanvas(game->plane.pos);
    float nudge = sinf(game->globalTime * 6.0f) * 6.0f;
    DrawSpriteCentered(a->tapRight, (Vector2){plane.x - 120.0f - nudge, plane.y}, 1.0f, 0.0f, WHITE);
    DrawSpriteCentered(a->tapLeft, (Vector2){plane.x + 120.0f + nudge, plane.y}, 1.0f, 0.0f, WHITE);
    float tapScale = 0.9f + 0.1f * sinf(game->globalTime * 8.0f);
    DrawSpriteCentered(a->tap, (Vector2){plane.x + 18.0f, plane.y + 86.0f}, tapScale, 0.0f, WHITE);

    const DifficultyDef *d = FlappyDifficultyGet(game->difficulty);
    DrawTextOutlinedCentered(TextFormat("Độ khó: %s  ·  Máy bay: %s", d->name, FlappyPlaneSkin(game->planeSkin)->name),
                             CANVAS_W * 0.5f, 250.0f, 20.0f, WHITE, (Color){70, 90, 120, 220}, 2.0f);
    DrawTextOutlinedCentered("Nhấn Space / ↑ / Click chuột để cất cánh", CANVAS_W * 0.5f, 520.0f, 24.0f,
                             GOLD_TEXT, INK, 2.5f);
}

static void DrawPaused(const FlappyGame *game)
{
    DrawPlayHud(game);
    DrawRectangle(0, 0, CANVAS_W, CANVAS_H, (Color){20, 30, 50, 120});

    Rectangle r = {CANVAS_W * 0.5f - 230.0f, 200.0f, 460.0f, 320.0f};
    DrawPanel(r, WHITE);
    DrawTextCentered("TẠM DỪNG", r.x + r.width * 0.5f, r.y + 34.0f, 40.0f, INK, true);

    const char *lines[] = {
        "P / Esc / Space   Tiếp tục",
        "R   Chơi lại từ đầu",
        "Q   Về menu chính",
        TextFormat("M   Âm thanh: %s", game->soundEnabled ? "BẬT" : "TẮT"),
    };
    for (int i = 0; i < 4; i++) {
        DrawTextCentered(lines[i], r.x + r.width * 0.5f, r.y + 118.0f + i * 44.0f, 21.0f, INK_SOFT, false);
    }
}

static void DrawGameOver(const FlappyGame *game)
{
    const FlappyAssets *a = FlappyAssetsGet();
    float t = game->gameOverAnim;

    // Chữ GAME OVER rơi xuống từ trên
    float drop = EaseOutBack(t / 0.55f);
    DrawSpriteCentered(a->textGameOver, (Vector2){CANVAS_W * 0.5f, Lerp(-80.0f, 118.0f, drop)}, 1.0f, 0.0f, WHITE);

    // Bảng điểm trượt lên
    float slide = EaseOutCubic((t - 0.25f) / 0.5f);
    if (slide <= 0.0f) return;
    Rectangle r = {CANVAS_W * 0.5f - 300.0f, Lerp(CANVAS_H + 20.0f, 196.0f, slide), 600.0f, 320.0f};
    DrawPanel(r, WHITE);

    // Huy chương bên trái
    Vector2 medalC = {r.x + 150.0f, r.y + 150.0f};
    DrawTextCentered("HUY CHƯƠNG", medalC.x, r.y + 30.0f, 18.0f, INK_SOFT, true);
    DrawCircleV(medalC, 66.0f, (Color){228, 214, 180, 255});
    DrawCircleLinesV(medalC, 66.0f, CARD_EDGE);
    MedalKind medal = MedalForScore(game->score);
    if (medal != MEDAL_NONE) {
        float pop = EaseOutBack((t - 0.9f) / 0.4f);
        if (pop > 0.0f) {
            float shine = sinf(game->globalTime * 2.0f) * 4.0f;
            DrawSpriteCentered(a->medal[medal], medalC, 1.0f * pop, shine, WHITE);
        }
    } else {
        DrawTextCentered("Chưa có", medalC.x, medalC.y - 20.0f, 18.0f, INK_SOFT, true);
        DrawTextCentered("Đạt 10 điểm để", medalC.x, medalC.y + 6.0f, 14.0f, INK_SOFT, false);
        DrawTextCentered("nhận đồng", medalC.x, medalC.y + 24.0f, 14.0f, INK_SOFT, false);
    }
    static const char *medalNames[] = {"Đồng · 10+", "Bạc · 25+", "Vàng · 50+"};
    DrawTextCentered(medal != MEDAL_NONE ? medalNames[medal] : "Đồng 10 · Bạc 25 · Vàng 50",
                     medalC.x, r.y + 240.0f, 15.0f, INK_SOFT, false);

    // Điểm & kỷ lục bên phải
    float colX = r.x + 400.0f;
    DrawTextCentered("ĐIỂM", colX, r.y + 30.0f, 18.0f, INK_SOFT, true);
    DrawNumberSprites(game->shownScore, colX, r.y + 58.0f, 0.8f, WHITE);

    DrawTextCentered("KỶ LỤC", colX, r.y + 138.0f, 18.0f, INK_SOFT, true);
    DrawTextCentered(TextFormat("%d", game->best[game->difficulty]), colX, r.y + 160.0f, 40.0f, INK, true);
    if (game->newBest && t > 1.0f) {
        float pulse = 1.0f + 0.06f * sinf(game->globalTime * 6.0f);
        float bw = 76.0f * pulse, bh = 30.0f * pulse;
        Vector2 bc = {colX + 100.0f, r.y + 180.0f};
        DrawRectangleRounded((Rectangle){bc.x - bw * 0.5f, bc.y - bh * 0.5f, bw, bh}, 0.5f, 8, (Color){232, 76, 61, 255});
        DrawTextCentered("MỚI!", bc.x, bc.y - 10.0f * pulse, 18.0f * pulse, WHITE, true);
    }

    Texture2D star = a->star[STAR_GOLD];
    const char *starsText = TextFormat("x %d", game->starsCollected);
    float sw = MeasureTextVNBoldPro(starsText, 22.0f, 1.0f).x;
    DrawSpriteCentered(star, (Vector2){colX - sw * 0.5f - 14.0f, r.y + 238.0f}, 0.75f, 0.0f, WHITE);
    DrawTextVNBoldPro(starsText, (Vector2){colX - sw * 0.5f + 12.0f, r.y + 226.0f}, 22.0f, 1.0f, INK);
    DrawTextCentered(TextFormat("Độ khó %s · %s", FlappyDifficultyGet(game->difficulty)->name,
                                FlappyBiome(game->biome)->name),
                     colX, r.y + 268.0f, 15.0f, INK_SOFT, false);

    if (slide >= 1.0f) {
        bool ready = game->stateTime >= 0.6f;
        Rectangle again = FlappyGameOverButton(0);
        Rectangle menu = FlappyGameOverButton(1);
        DrawKenneyButton(again, "CHƠI LẠI", ready && Hovered(again), true);
        DrawKenneyButton(menu, "MENU", ready && Hovered(menu), false);
        DrawTextOutlinedCentered("Space / Enter  Chơi lại      Esc  Về menu", CANVAS_W * 0.5f, 640.0f, 18.0f,
                                 WHITE, (Color){70, 50, 30, 200}, 1.5f);
    }
}

// ---------------------------------------------------------------------------

void DrawFlappyGame(const FlappyGame *game)
{
    ClearBackground((Color){208, 238, 248, 255});

    Vector2 shake = {0.0f, 0.0f};
    if (game->shakeTimer > 0.0f) {
        float m = game->shakeMagnitude * fminf(1.0f, game->shakeTimer / 0.3f);
        shake.x = (float)GetRandomValue(-100, 100) / 100.0f * m;
        shake.y = (float)GetRandomValue(-100, 100) / 100.0f * m;
    }

    Camera2D cam = {0};
    cam.offset = shake;
    cam.zoom = WORLD_ZOOM;
    BeginMode2D(cam);
        DrawWorld(game);
    EndMode2D();

    DrawFloatTexts(game);

    switch (game->state) {
        case FLAPPY_STATE_MENU:      DrawMenu(game); break;
        case FLAPPY_STATE_READY:     DrawReady(game); break;
        case FLAPPY_STATE_PLAYING:
        case FLAPPY_STATE_DYING:     DrawPlayHud(game); break;
        case FLAPPY_STATE_PAUSED:    DrawPaused(game); break;
        case FLAPPY_STATE_GAME_OVER: DrawGameOver(game); break;
    }

    if (game->flashTimer > 0.0f) {
        DrawRectangle(0, 0, CANVAS_W, CANVAS_H, Fade(WHITE, game->flashTimer / 0.25f * 0.7f));
    }
}
