#include "caro_draw.h"
#include "caro_rules.h"
#include "caro_assets.h"
#include "font_vn.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ------------------------------------------------------------------ bảng màu

static const Color COL_BG_TOP     = {26, 34, 44, 255};
static const Color COL_BG_BOTTOM  = {10, 13, 19, 255};
static const Color COL_PANEL      = {14, 18, 26, 200};
static const Color COL_PANEL_LINE = {255, 255, 255, 26};
static const Color COL_TEXT       = {238, 240, 245, 255};
static const Color COL_TEXT_DIM   = {150, 160, 176, 255};
static const Color COL_ACCENT     = {255, 198, 92, 255};
static const Color COL_GRID       = {92, 58, 30, 150};
static const Color COL_GRID_EDGE  = {84, 52, 26, 200};

static const Color X_COLOR = {236, 84, 84, 255};
static const Color X_EDGE  = {150, 28, 40, 255};
static const Color O_COLOR = {64, 142, 236, 255};
static const Color O_EDGE  = {22, 64, 150, 255};

static const char *DIFF_NAMES[CARO_DIFF_COUNT] = {"Dễ", "Thường", "Khó"};
static const char *RULE_NAMES[CARO_RULE_COUNT] = {"Tự do", "Chặn 2 đầu"};

// ------------------------------------------------------------------ bố cục

static const Rectangle BOARD_FRAME = {32.0f, 32.0f, 656.0f, 656.0f};
static const float     FRAME_BAND  = 30.0f;     // Viền gỗ chứa toạ độ
static const Rectangle PANEL       = {716.0f, 32.0f, 532.0f, 656.0f};

Rectangle CaroBoardArea(int n)
{
    (void)n;
    return (Rectangle){BOARD_FRAME.x + FRAME_BAND, BOARD_FRAME.y + FRAME_BAND,
                       BOARD_FRAME.width - FRAME_BAND * 2.0f, BOARD_FRAME.height - FRAME_BAND * 2.0f};
}

Rectangle CaroCellRect(int n, int cell)
{
    Rectangle a = CaroBoardArea(n);
    float cs = a.width / (float)n;
    int x = cell % n, y = cell / n;
    return (Rectangle){a.x + x * cs, a.y + y * cs, cs, cs};
}

int CaroCellAtPoint(int n, Vector2 p)
{
    Rectangle a = CaroBoardArea(n);
    if (!CheckCollisionPointRec(p, a)) return -1;
    float cs = a.width / (float)n;
    int x = (int)((p.x - a.x) / cs), y = (int)((p.y - a.y) / cs);
    if (x < 0 || y < 0 || x >= n || y >= n) return -1;
    return y * n + x;
}

#define MENU_ROW_Y0     206.0f
#define MENU_ROW_STEP   66.0f
#define MENU_LABEL_W    150.0f

Rectangle CaroMenuChipRect(CaroMenuOption row, int index)
{
    int count = CaroMenuOptionCount(row);
    float x0 = PANEL.x + 28.0f + MENU_LABEL_W;
    float total = PANEL.width - 28.0f * 2.0f - MENU_LABEL_W;
    float gap = 8.0f;
    float w = (total - gap * (count - 1)) / (float)count;
    return (Rectangle){x0 + index * (w + gap), MENU_ROW_Y0 + row * MENU_ROW_STEP, w, 46.0f};
}

Rectangle CaroMenuStartRect(void)
{
    return (Rectangle){PANEL.x + 28.0f, PANEL.y + PANEL.height - 100.0f, PANEL.width - 56.0f, 64.0f};
}

Rectangle CaroPanelButtonRect(CaroPanelButton b)
{
    float gap = 12.0f;
    float w = (PANEL.width - 48.0f - gap) * 0.5f;
    float h = 54.0f;
    float y0 = PANEL.y + PANEL.height - 24.0f - h * 2.0f - gap;
    int col = b % 2, row = b / 2;
    return (Rectangle){PANEL.x + 24.0f + col * (w + gap), y0 + row * (h + gap), w, h};
}

static const Rectangle PAUSE_CARD = {CARO_CANVAS_W * 0.5f - 220.0f, CARO_CANVAS_H * 0.5f - 190.0f, 440.0f, 380.0f};

Rectangle CaroPauseRowRect(int row)
{
    return (Rectangle){PAUSE_CARD.x + 40.0f, PAUSE_CARD.y + 104.0f + row * 62.0f, PAUSE_CARD.width - 80.0f, 50.0f};
}

// ------------------------------------------------------------------ tiện ích vẽ

static float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

static float EaseOutBack(float t)
{
    t = Clamp01(t);
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

static float EaseOutCubic(float t)
{
    t = Clamp01(t);
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

static Color WithAlpha(Color c, float a)
{
    c.a = (unsigned char)(Clamp01(a) * (float)c.a);
    return c;
}

static Color StoneColor(CaroStone s) { return s == CARO_X ? X_COLOR : O_COLOR; }
static Color StoneEdge(CaroStone s)  { return s == CARO_X ? X_EDGE : O_EDGE; }
static CaroShape StoneShape(CaroStone s) { return s == CARO_X ? CARO_SHAPE_X : CARO_SHAPE_O; }

static void TextCentered(const char *text, Rectangle r, float size, Color color, bool bold)
{
    Vector2 m = bold ? MeasureTextVNBoldPro(text, size, 0.0f) : MeasureTextVNPro(text, size, 0.0f);
    Vector2 pos = {r.x + (r.width - m.x) * 0.5f, r.y + (r.height - m.y) * 0.5f};
    if (bold) DrawTextVNBoldPro(text, pos, size, 0.0f, color);
    else DrawTextVNPro(text, pos, size, 0.0f, color);
}

static void Text(const char *text, float x, float y, float size, Color color, bool bold)
{
    if (bold) DrawTextVNBoldPro(text, (Vector2){x, y}, size, 0.0f, color);
    else DrawTextVNPro(text, (Vector2){x, y}, size, 0.0f, color);
}

static float TextWidth(const char *text, float size, bool bold)
{
    return bold ? MeasureTextVNBoldPro(text, size, 0.0f).x : MeasureTextVNPro(text, size, 0.0f).x;
}

static void SoftShadow(Rectangle r, float roundness, float spread, float alpha)
{
    for (int i = 6; i >= 1; i--) {
        float g = spread * (float)i / 6.0f;
        Rectangle s = {r.x - g, r.y - g + spread * 0.5f, r.width + g * 2.0f, r.height + g * 2.0f};
        DrawRectangleRounded(s, roundness, 12, WithAlpha(BLACK, alpha / 6.0f));
    }
}

static void Panel(Rectangle r)
{
    SoftShadow(r, 0.05f, 18.0f, 0.55f);
    DrawRectangleRounded(r, 0.045f, 12, COL_PANEL);
    DrawRectangleRoundedLinesEx(r, 0.045f, 12, 1.0f, COL_PANEL_LINE);
}

static bool Hovered(Rectangle r)
{
    return CheckCollisionPointRec(GetMousePosition(), r);
}

static void Chip(Rectangle r, const char *label, bool selected, bool focused, bool enabled, float t)
{
    Color fill = selected ? COL_ACCENT : (Color){255, 255, 255, 18};
    Color text = selected ? (Color){40, 26, 8, 255} : COL_TEXT;
    if (!enabled) {
        fill = selected ? (Color){120, 110, 96, 120} : (Color){255, 255, 255, 8};
        text = WithAlpha(COL_TEXT_DIM, 0.6f);
    } else if (!selected && Hovered(r)) {
        fill = (Color){255, 255, 255, 34};
    }
    DrawRectangleRounded(r, 0.4f, 10, fill);
    if (selected && enabled) {
        DrawRectangleRounded((Rectangle){r.x, r.y, r.width, r.height * 0.5f}, 0.4f, 10, (Color){255, 255, 255, 40});
    }
    if (focused && enabled) {
        float pulse = 0.6f + 0.4f * sinf(t * 5.0f);
        DrawRectangleRoundedLinesEx((Rectangle){r.x - 3, r.y - 3, r.width + 6, r.height + 6}, 0.45f, 10, 2.0f,
                                    WithAlpha(COL_ACCENT, pulse));
    }
    TextCentered(label, r, 19.0f, text, selected);
}

static void Button(Rectangle r, const char *label, const char *key, bool primary, bool enabled)
{
    bool hover = enabled && Hovered(r);
    Color fill = primary ? COL_ACCENT : (Color){255, 255, 255, hover ? 36 : 20};
    if (primary && hover) fill = (Color){255, 212, 128, 255};
    if (!enabled) fill = (Color){255, 255, 255, 8};
    DrawRectangleRounded(r, 0.32f, 10, fill);
    if (primary && enabled) {
        DrawRectangleRounded((Rectangle){r.x + 2, r.y + 2, r.width - 4, r.height * 0.48f}, 0.32f, 10, (Color){255, 255, 255, 46});
    }
    Color tc = primary ? (Color){44, 28, 8, 255} : COL_TEXT;
    if (!enabled) tc = WithAlpha(COL_TEXT_DIM, 0.5f);

    float size = primary ? 24.0f : 19.0f;
    float lw = TextWidth(label, size, true);
    float kw = key ? TextWidth(key, 14.0f, true) + 14.0f : 0.0f;
    float total = lw + (key ? kw + 10.0f : 0.0f);
    float x = r.x + (r.width - total) * 0.5f;
    Text(label, x, r.y + (r.height - size) * 0.5f - 1.0f, size, tc, true);
    if (key) {
        Rectangle kr = {x + lw + 10.0f, r.y + (r.height - 22.0f) * 0.5f, kw, 22.0f};
        DrawRectangleRounded(kr, 0.35f, 8, primary ? (Color){0, 0, 0, 40} : (Color){255, 255, 255, 26});
        TextCentered(key, kr, 14.0f, WithAlpha(tc, 0.85f), true);
    }
}

// ------------------------------------------------------------------ nền

static void DrawBackdrop(float t)
{
    DrawRectangleGradientV(0, 0, CARO_CANVAS_W, CARO_CANVAS_H, COL_BG_TOP, COL_BG_BOTTOM);
    // Vài quầng sáng ấm / lạnh trôi rất chậm
    Vector2 a = {260.0f + sinf(t * 0.15f) * 60.0f, 180.0f + cosf(t * 0.12f) * 40.0f};
    Vector2 b = {1060.0f + cosf(t * 0.11f) * 70.0f, 560.0f + sinf(t * 0.13f) * 50.0f};
    DrawCircleGradient(a, 520.0f, (Color){255, 170, 90, 38}, (Color){255, 170, 90, 0});
    DrawCircleGradient(b, 560.0f, (Color){80, 150, 255, 34}, (Color){80, 150, 255, 0});
    // Hạt bụi lấp lánh
    for (int i = 0; i < 40; i++) {
        float x = fmodf((float)(i * 127 % 1280) + t * (6.0f + (float)(i % 5)), (float)CARO_CANVAS_W);
        float y = fmodf((float)(i * 311 % 720) - t * (4.0f + (float)(i % 3)) + 720.0f, (float)CARO_CANVAS_H);
        float tw = 0.5f + 0.5f * sinf(t * 1.7f + (float)i);
        DrawCircleV((Vector2){x, y}, 1.2f + (float)(i % 3) * 0.5f, WithAlpha(WHITE, 0.05f + 0.08f * tw));
    }
}

// ------------------------------------------------------------------ bàn cờ

static void DrawWood(Rectangle dst, float scale, Color tint)
{
    const CaroAssets *a = CaroAssetsGet();
    Rectangle src = {0.0f, 0.0f, dst.width * scale, dst.height * scale};
    DrawTexturePro(a->wood, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, tint);
}

static void DrawBoard(int n, bool coords)
{
    Rectangle f = BOARD_FRAME;
    Rectangle area = CaroBoardArea(n);

    SoftShadow(f, 0.02f, 26.0f, 0.75f);

    // Khung gỗ tối màu, mặt bàn gỗ sáng
    DrawWood(f, 1.6f, (Color){128, 82, 52, 255});
    DrawRectangleLinesEx(f, 2.0f, (Color){40, 22, 10, 200});
    DrawLineEx((Vector2){f.x + 2, f.y + 2}, (Vector2){f.x + f.width - 2, f.y + 2}, 2.0f, (Color){255, 230, 190, 70});
    DrawLineEx((Vector2){f.x + 2, f.y + 2}, (Vector2){f.x + 2, f.y + f.height - 2}, 2.0f, (Color){255, 230, 190, 50});

    Rectangle inner = {area.x - 6.0f, area.y - 6.0f, area.width + 12.0f, area.height + 12.0f};
    DrawRectangleRec((Rectangle){inner.x - 2, inner.y - 2, inner.width + 4, inner.height + 4}, (Color){50, 28, 12, 160});
    DrawWood(inner, 1.15f, WHITE);
    // Phủ một lớp kem mỏng: gỗ sáng hơn, vân dịu lại để quân cờ nổi bật
    DrawRectangleRec(inner, (Color){255, 234, 196, 92});

    // Viền tối nhẹ bên trong cho mặt bàn có chiều sâu
    for (int i = 0; i < 8; i++) {
        float a = 0.06f * (1.0f - (float)i / 8.0f);
        DrawRectangleLinesEx((Rectangle){inner.x + i, inner.y + i, inner.width - i * 2, inner.height - i * 2}, 1.0f,
                             WithAlpha((Color){60, 30, 10, 255}, a));
    }

    // Lưới ô
    float cs = area.width / (float)n;
    for (int i = 0; i <= n; i++) {
        bool edge = (i == 0 || i == n);
        float w = edge ? 2.0f : 1.2f;
        Color c = edge ? COL_GRID_EDGE : COL_GRID;
        DrawLineEx((Vector2){area.x + i * cs, area.y}, (Vector2){area.x + i * cs, area.y + area.height}, w, c);
        DrawLineEx((Vector2){area.x, area.y + i * cs}, (Vector2){area.x + area.width, area.y + i * cs}, w, c);
    }

    if (coords) {
        static const char *COLS = "ABCDEFGHJKLMNOPQRST";
        float size = n > 15 ? 13.0f : 15.0f;
        for (int i = 0; i < n; i++) {
            char s[4] = {COLS[i], 0, 0, 0};
            Rectangle top = {area.x + i * cs, f.y + 2.0f, cs, FRAME_BAND - 4.0f};
            Rectangle bottom = {area.x + i * cs, f.y + f.height - FRAME_BAND + 2.0f, cs, FRAME_BAND - 4.0f};
            TextCentered(s, top, size, (Color){255, 236, 206, 190}, true);
            TextCentered(s, bottom, size, (Color){255, 236, 206, 190}, true);
            char num[12];
            snprintf(num, sizeof(num), "%d", n - i);
            Rectangle left = {f.x + 2.0f, area.y + i * cs, FRAME_BAND - 4.0f, cs};
            Rectangle right = {f.x + f.width - FRAME_BAND + 2.0f, area.y + i * cs, FRAME_BAND - 4.0f, cs};
            TextCentered(num, left, size, (Color){255, 236, 206, 190}, true);
            TextCentered(num, right, size, (Color){255, 236, 206, 190}, true);
        }
    }
}

static Rectangle PieceRect(int n, int cell, float scale)
{
    Rectangle r = CaroCellRect(n, cell);
    float pad = r.width * 0.05f;
    float s = (r.width - pad * 2.0f) * scale;
    return (Rectangle){r.x + (r.width - s) * 0.5f, r.y + (r.height - s) * 0.5f, s, s};
}

static void DrawStone(int n, int cell, CaroStone s, float anim, float glow, float alpha)
{
    float scale = 0.85f + 0.15f * EaseOutBack(anim * 1.4f);
    Color col = WithAlpha(StoneColor(s), alpha);
    CaroDrawShape(StoneShape(s), PieceRect(n, cell, scale), 0.0f, col, StoneEdge(s),
                  EaseOutCubic(anim), glow, 1.0f);
}

static Vector2 CellCenter(int n, int cell)
{
    Rectangle r = CaroCellRect(n, cell);
    return (Vector2){r.x + r.width * 0.5f, r.y + r.height * 0.5f};
}

static void DrawWinLine(int n, int a, int b, float anim, float t)
{
    Vector2 pa = CellCenter(n, a), pb = CellCenter(n, b);
    Vector2 d = {pb.x - pa.x, pb.y - pa.y};
    float len = sqrtf(d.x * d.x + d.y * d.y);
    float cs = CaroBoardArea(n).width / (float)n;
    float thick = cs * 0.26f;
    float rot = atan2f(d.y, d.x) * RAD2DEG;
    Rectangle r = {(pa.x + pb.x) * 0.5f - (len + cs * 0.9f) * 0.5f, (pa.y + pb.y) * 0.5f - thick,
                   len + cs * 0.9f, thick * 2.0f};
    float pulse = 0.55f + 0.45f * sinf(t * 4.0f);
    CaroDrawShape(CARO_SHAPE_BAR, r, rot, (Color){255, 222, 120, 200}, (Color){214, 140, 40, 255},
                  EaseOutCubic(anim * 1.6f), pulse, 0.6f);
}

static void DrawParticles(const CaroGame *game)
{
    for (int i = 0; i < CARO_MAX_PARTICLES; i++) {
        const CaroParticle *p = &game->particles[i];
        if (!p->active) continue;
        float k = p->life / p->maxLife;
        if (p->size > 4.5f) {
            Rectangle r = {p->pos.x, p->pos.y, p->size, p->size * 0.55f};
            DrawRectanglePro(r, (Vector2){r.width * 0.5f, r.height * 0.5f}, p->rot,
                             WithAlpha(p->color, fminf(1.0f, k * 2.5f)));
        } else {
            DrawCircleV(p->pos, p->size * (0.4f + 0.6f * k), WithAlpha(p->color, k));
        }
    }
}

// Quân trên bàn + các lớp đánh dấu (nước cuối, gợi ý, con trỏ, rê chuột).
static void DrawBoardContents(const CaroGame *game)
{
    const CaroBoard *b = &game->board;
    int n = b->n;
    float t = game->globalTime;
    int last = b->moveCount > 0 ? b->moves[b->moveCount - 1] : -1;

    // Nước vừa đi: ô sáng nhẹ
    if (last >= 0) {
        Rectangle r = CaroCellRect(n, last);
        DrawRectangleRec((Rectangle){r.x + 1, r.y + 1, r.width - 2, r.height - 2}, (Color){255, 214, 120, 70});
    }

    bool playing = game->state == CARO_STATE_PLAYING;
    bool humanTurn = game->mode == CARO_MODE_TWO_PLAYERS || b->toMove == game->humanStone;

    // Bóng mờ quân sắp đặt dưới chuột
    if (playing && humanTurn && !game->aiThinking && !game->keyboardCursor &&
        game->hoverCell >= 0 && b->cells[game->hoverCell] == CARO_EMPTY) {
        Rectangle r = CaroCellRect(n, game->hoverCell);
        DrawRectangleRec((Rectangle){r.x + 1, r.y + 1, r.width - 2, r.height - 2}, WithAlpha(StoneColor(b->toMove), 0.12f));
        CaroDrawShape(StoneShape(b->toMove), PieceRect(n, game->hoverCell, 0.9f), 0.0f,
                      WithAlpha(StoneColor(b->toMove), 0.38f), StoneEdge(b->toMove), 1.0f, 0.0f, 0.0f);
    }

    // Gợi ý
    if (game->hintCell >= 0 && b->cells[game->hintCell] == CARO_EMPTY) {
        Rectangle r = CaroCellRect(n, game->hintCell);
        float pulse = 0.5f + 0.5f * sinf(game->hintTimer * 6.0f);
        DrawRectangleRounded((Rectangle){r.x + 2, r.y + 2, r.width - 4, r.height - 4}, 0.25f, 8,
                             (Color){110, 230, 150, (unsigned char)(50 + 60 * pulse)});
        CaroDrawShape(StoneShape(b->toMove), PieceRect(n, game->hintCell, 0.9f), 0.0f,
                      WithAlpha(StoneColor(b->toMove), 0.55f), StoneEdge(b->toMove), 1.0f, 0.4f * pulse, 0.0f);
    }

    // Quân cờ
    bool over = game->state == CARO_STATE_OVER && game->result != CARO_RESULT_DRAW;
    for (int i = 0; i < n * n; i++) {
        CaroStone s = (CaroStone)b->cells[i];
        if (s == CARO_EMPTY) continue;
        float glow = (i == last) ? 0.28f + 0.12f * sinf(t * 3.0f) : 0.0f;
        float alpha = 1.0f;
        if (over) alpha = 0.55f + 0.45f * (1.0f - Clamp01(game->winAnim * 1.5f));
        DrawStone(n, i, s, game->placeAnim[i], glow, alpha);
    }

    // Hàng thắng: quân sáng rực + vạch vàng
    if (over) {
        int a = game->winCells[0], e = game->winCells[1];
        int ax = a % n, ay = a / n, ex = e % n, ey = e / n;
        int steps = (abs(ex - ax) > abs(ey - ay)) ? abs(ex - ax) : abs(ey - ay);
        int sx = (ex > ax) - (ex < ax), sy = (ey > ay) - (ey < ay);
        for (int k = 0; k <= steps; k++) {
            int cell = (ay + sy * k) * n + (ax + sx * k);
            float glow = 0.6f + 0.4f * sinf(t * 5.0f - k * 0.6f);
            DrawStone(n, cell, (CaroStone)b->cells[cell], 1.0f, glow, 1.0f);
        }
        DrawWinLine(n, a, e, game->winAnim, t);
    }

    // Con trỏ bàn phím
    if (playing && game->keyboardCursor) {
        Rectangle r = CaroCellRect(n, game->cursor);
        float pulse = 0.6f + 0.4f * sinf(t * 6.0f);
        Color c = humanTurn ? StoneColor(b->toMove) : COL_TEXT_DIM;
        DrawRectangleRoundedLinesEx((Rectangle){r.x + 1, r.y + 1, r.width - 2, r.height - 2}, 0.2f, 8, 3.0f,
                                    WithAlpha(c, pulse));
        if (humanTurn && b->cells[game->cursor] == CARO_EMPTY) {
            CaroDrawShape(StoneShape(b->toMove), PieceRect(n, game->cursor, 0.9f), 0.0f,
                          WithAlpha(StoneColor(b->toMove), 0.35f), StoneEdge(b->toMove), 1.0f, 0.0f, 0.0f);
        }
    }
}

// ------------------------------------------------------------------ bảng thông tin

static const char *PlayerName(const CaroGame *game, CaroStone s)
{
    if (game->mode == CARO_MODE_TWO_PLAYERS) return s == CARO_X ? "Người chơi 1" : "Người chơi 2";
    return s == game->humanStone ? "Bạn" : "Máy";
}

static void DrawPlayerCard(const CaroGame *game, Rectangle r, CaroStone s)
{
    const CaroBoard *b = &game->board;
    bool active = game->state == CARO_STATE_PLAYING && b->toMove == s;
    bool isAI = game->mode == CARO_MODE_VS_AI && s != game->humanStone;
    float t = game->globalTime;

    DrawRectangleRounded(r, 0.22f, 10, active ? WithAlpha(StoneColor(s), 0.18f) : (Color){255, 255, 255, 12});
    if (active) {
        float pulse = 0.55f + 0.45f * sinf(t * 4.0f);
        DrawRectangleRoundedLinesEx(r, 0.22f, 10, 2.0f, WithAlpha(StoneColor(s), pulse));
    }

    Rectangle icon = {r.x + 14.0f, r.y + (r.height - 50.0f) * 0.5f, 50.0f, 50.0f};
    DrawRectangleRounded(icon, 0.3f, 8, (Color){255, 246, 232, 230});
    CaroDrawShape(StoneShape(s), (Rectangle){icon.x + 7, icon.y + 7, 36, 36}, 0.0f, StoneColor(s), StoneEdge(s),
                  1.0f, active ? 0.3f : 0.0f, 1.0f);

    Text(PlayerName(game, s), icon.x + 66.0f, r.y + 14.0f, 24.0f, COL_TEXT, true);
    char sub[64];
    if (isAI) snprintf(sub, sizeof(sub), "Máy · %s", DIFF_NAMES[game->difficulty]);
    else snprintf(sub, sizeof(sub), "%s", s == CARO_X ? "Cầm quân X · đi trước" : "Cầm quân O");
    Text(sub, icon.x + 66.0f, r.y + 44.0f, 16.0f, COL_TEXT_DIM, false);

    if (active) {
        char status[48];
        if (isAI && game->aiThinking) {
            int dots = (int)(t * 3.0f) % 4;
            snprintf(status, sizeof(status), "Đang nghĩ%.*s", dots, "...");
        } else {
            snprintf(status, sizeof(status), "Đến lượt");
        }
        float w = TextWidth(status, 16.0f, true);
        Rectangle pill = {r.x + r.width - w - 34.0f, r.y + (r.height - 28.0f) * 0.5f, w + 20.0f, 28.0f};
        DrawRectangleRounded(pill, 0.5f, 8, StoneColor(s));
        TextCentered(status, pill, 16.0f, WHITE, true);
    }
}

static void DrawResultBlock(const CaroGame *game, float y)
{
    const char *title;
    Color c = COL_ACCENT;
    if (game->result == CARO_RESULT_DRAW) {
        title = "Hoà cờ!";
    } else {
        CaroStone w = game->result == CARO_RESULT_X_WINS ? CARO_X : CARO_O;
        c = StoneColor(w);
        if (game->mode == CARO_MODE_VS_AI) title = (w == game->humanStone) ? "Bạn thắng!" : "Máy thắng!";
        else title = (w == CARO_X) ? "Người chơi 1 thắng!" : "Người chơi 2 thắng!";
    }
    float pop = EaseOutBack(game->resultTime * 2.5f);
    float size = 38.0f * (0.6f + 0.4f * pop);
    Rectangle r = {PANEL.x + 24.0f, y, PANEL.width - 48.0f, 60.0f};
    DrawRectangleRounded(r, 0.3f, 10, WithAlpha(c, 0.2f));
    TextCentered(title, r, size, c, true);
}

static void DrawSidePanel(const CaroGame *game)
{
    const CaroBoard *b = &game->board;
    Panel(PANEL);
    float x = PANEL.x + 24.0f;
    float y = PANEL.y + 22.0f;

    Text("Cờ Caro", x, y, 40.0f, COL_TEXT, true);
    char sub[96];
    snprintf(sub, sizeof(sub), "%d×%d · Luật %s%s%s", b->n, b->n, RULE_NAMES[b->rule],
             game->mode == CARO_MODE_VS_AI ? " · Độ khó " : "",
             game->mode == CARO_MODE_VS_AI ? DIFF_NAMES[game->difficulty] : "");
    Text(sub, x, y + 48.0f, 17.0f, COL_TEXT_DIM, false);
    y += 88.0f;

    DrawPlayerCard(game, (Rectangle){x, y, PANEL.width - 48.0f, 78.0f}, CARO_X);
    y += 90.0f;
    DrawPlayerCard(game, (Rectangle){x, y, PANEL.width - 48.0f, 78.0f}, CARO_O);
    y += 96.0f;

    // Tỉ số trong phiên + thông tin ván
    Rectangle score = {x, y, PANEL.width - 48.0f, 64.0f};
    DrawRectangleRounded(score, 0.25f, 10, (Color){0, 0, 0, 60});
    char sx[8], so[8], sd[24];
    snprintf(sx, sizeof(sx), "%d", game->sessionX);
    snprintf(so, sizeof(so), "%d", game->sessionO);
    snprintf(sd, sizeof(sd), "Hoà %d", game->sessionDraw);
    float third = score.width / 3.0f;
    TextCentered(sx, (Rectangle){score.x, score.y + 4, third, 40}, 34.0f, X_COLOR, true);
    TextCentered(so, (Rectangle){score.x + third * 2, score.y + 4, third, 40}, 34.0f, O_COLOR, true);
    TextCentered("Tỉ số", (Rectangle){score.x + third, score.y + 6, third, 30}, 18.0f, COL_TEXT, true);
    TextCentered(sd, (Rectangle){score.x + third, score.y + 34, third, 24}, 15.0f, COL_TEXT_DIM, false);
    y += 80.0f;

    int secs = (int)game->gameTime;
    char info[128];
    snprintf(info, sizeof(info), "Nước thứ %d   ·   %02d:%02d   ·   Nước cuối %s", b->moveCount,
             secs / 60, secs % 60, b->moveCount > 0 ? CaroCellName(b, b->moves[b->moveCount - 1]) : "—");
    Text(info, x, y, 17.0f, COL_TEXT_DIM, false);
    y += 34.0f;

    if (game->state == CARO_STATE_OVER) {
        DrawResultBlock(game, y);
    } else if (game->mode == CARO_MODE_VS_AI) {
        int d = game->difficulty;
        char rec[128];
        snprintf(rec, sizeof(rec), "Thành tích mức %s:  %d thắng · %d thua · %d hoà",
                 DIFF_NAMES[d], game->wins[d], game->losses[d], game->draws[d]);
        Text(rec, x, y + 18.0f, 17.0f, COL_TEXT, false);
    } else {
        Text("Hai người chơi chung một máy, luân phiên đặt quân.", x, y + 18.0f, 17.0f, COL_TEXT, false);
    }

    bool over = game->state == CARO_STATE_OVER;
    bool canHint = !over && !game->aiThinking &&
                   (game->mode == CARO_MODE_TWO_PLAYERS || b->toMove == game->humanStone);
    Button(CaroPanelButtonRect(CARO_BTN_UNDO), "Hoàn tác", "U", false, !over && b->moveCount > 0);
    Button(CaroPanelButtonRect(CARO_BTN_HINT), game->hintPending ? "Đang tìm…" : "Gợi ý", "H", false, canHint);
    Button(CaroPanelButtonRect(CARO_BTN_NEW), "Ván mới", over ? "Enter" : "N", over, true);
    Button(CaroPanelButtonRect(CARO_BTN_MENU), over ? "Về menu" : "Tạm dừng", "Esc", false, true);
}

// Dải chữ lớn chạy ngang bàn cờ khi ván vừa kết thúc.
static void DrawResultBanner(const CaroGame *game)
{
    float t = game->resultTime;
    if (t > 2.6f) return;
    float in = EaseOutCubic(t * 3.0f);
    float out = Clamp01((2.6f - t) * 2.5f);
    float a = in * out;

    const char *text;
    Color c = COL_ACCENT;
    if (game->result == CARO_RESULT_DRAW) {
        text = "HOÀ CỜ";
    } else {
        CaroStone w = game->result == CARO_RESULT_X_WINS ? CARO_X : CARO_O;
        c = StoneColor(w);
        if (game->mode == CARO_MODE_VS_AI) text = (w == game->humanStone) ? "CHIẾN THẮNG!" : "THUA RỒI!";
        else text = (w == CARO_X) ? "X THẮNG!" : "O THẮNG!";
    }
    Rectangle area = BOARD_FRAME;
    float h = 110.0f * in;
    Rectangle band = {area.x, area.y + area.height * 0.5f - h * 0.5f, area.width, h};
    DrawRectangleRec(band, WithAlpha((Color){8, 10, 16, 255}, 0.72f * a));
    DrawRectangleRec((Rectangle){band.x, band.y, band.width, 3}, WithAlpha(c, a));
    DrawRectangleRec((Rectangle){band.x, band.y + band.height - 3, band.width, 3}, WithAlpha(c, a));
    float size = 64.0f * (0.7f + 0.3f * EaseOutBack(t * 2.2f));
    TextCentered(text, band, size, WithAlpha(c, a), true);
}

// ------------------------------------------------------------------ menu

// Ván mẫu chạy vòng lặp trên bàn ở màn hình menu (X thắng chéo).
static const int DEMO_MOVES[][2] = {
    {6, 7}, {7, 7}, {7, 6}, {6, 6}, {5, 8}, {8, 8}, {8, 5}, {9, 4}, {4, 9}
};
#define DEMO_COUNT ((int)(sizeof(DEMO_MOVES) / sizeof(DEMO_MOVES[0])))
#define DEMO_STEP 0.55f
#define DEMO_HOLD 2.4f

static void DrawMenuDemo(const CaroGame *game)
{
    int n = game->sizeChoice == 0 ? 15 : 19;
    int off = (n - 15) / 2;
    float cycle = DEMO_COUNT * DEMO_STEP + DEMO_HOLD;
    float t = fmodf(game->globalTime, cycle);
    float fade = Clamp01((cycle - t) * 2.0f);

    DrawBoard(n, true);
    for (int i = 0; i < DEMO_COUNT; i++) {
        float appear = t - i * DEMO_STEP;
        if (appear < 0.0f) break;
        int cell = (DEMO_MOVES[i][1] + off) * n + (DEMO_MOVES[i][0] + off);
        CaroStone s = (i % 2 == 0) ? CARO_X : CARO_O;
        DrawStone(n, cell, s, appear * 4.0f, 0.0f, fade);
    }
    float done = t - DEMO_COUNT * DEMO_STEP;
    if (done > 0.0f) {
        // Hai đầu hàng thắng: nước cuối (4,9) và (8,5)
        int a = (DEMO_MOVES[8][1] + off) * n + (DEMO_MOVES[8][0] + off);
        int e = (DEMO_MOVES[6][1] + off) * n + (DEMO_MOVES[6][0] + off);
        DrawWinLine(n, a, e, done * fade, game->globalTime);
    }
    // Lớp phủ mờ để menu nổi bật hơn
    DrawRectangleRec(BOARD_FRAME, (Color){10, 12, 18, 28});
}

static const char *OptionLabel(CaroMenuOption row)
{
    switch (row) {
        case CARO_OPT_MODE:       return "Chế độ";
        case CARO_OPT_DIFFICULTY: return "Độ khó";
        case CARO_OPT_SIZE:       return "Bàn cờ";
        case CARO_OPT_RULE:       return "Luật thắng";
        case CARO_OPT_FIRST:      return "Đi trước";
        default:                  return "";
    }
}

static const char *OptionValue(CaroMenuOption row, int i)
{
    static const char *MODE[] = {"Đấu với máy", "2 người"};
    static const char *SIZE[] = {"15 × 15", "19 × 19"};
    static const char *FIRST[] = {"Bạn (X)", "Máy (X)"};
    switch (row) {
        case CARO_OPT_MODE:       return MODE[i];
        case CARO_OPT_DIFFICULTY: return DIFF_NAMES[i];
        case CARO_OPT_SIZE:       return SIZE[i];
        case CARO_OPT_RULE:       return RULE_NAMES[i];
        case CARO_OPT_FIRST:      return FIRST[i];
        default:                  return "";
    }
}

static int CurrentOption(const CaroGame *game, CaroMenuOption row)
{
    switch (row) {
        case CARO_OPT_MODE:       return game->mode;
        case CARO_OPT_DIFFICULTY: return game->difficulty;
        case CARO_OPT_SIZE:       return game->sizeChoice;
        case CARO_OPT_RULE:       return game->rule;
        case CARO_OPT_FIRST:      return game->humanFirst ? 0 : 1;
        default:                  return 0;
    }
}

static void DrawMenuPanel(const CaroGame *game)
{
    float t = game->globalTime;
    Panel(PANEL);
    float x = PANEL.x + 28.0f;

    // Tiêu đề kèm hai quân X / O nhỏ
    CaroDrawShape(CARO_SHAPE_X, (Rectangle){x, PANEL.y + 30.0f, 54, 54}, 0.0f, X_COLOR, X_EDGE, 1.0f,
                  0.25f + 0.15f * sinf(t * 2.0f), 1.0f);
    CaroDrawShape(CARO_SHAPE_O, (Rectangle){x + 58.0f, PANEL.y + 30.0f, 54, 54}, 0.0f, O_COLOR, O_EDGE, 1.0f,
                  0.25f + 0.15f * sinf(t * 2.0f + 1.5f), 1.0f);
    Text("CỜ CARO", x + 130.0f, PANEL.y + 26.0f, 54.0f, COL_TEXT, true);
    Text("Năm quân liền một hàng là thắng", x + 132.0f, PANEL.y + 86.0f, 18.0f, COL_TEXT_DIM, false);

    DrawLineEx((Vector2){x, PANEL.y + 140.0f}, (Vector2){PANEL.x + PANEL.width - 28.0f, PANEL.y + 140.0f}, 1.0f, COL_PANEL_LINE);

    for (int row = 0; row < CARO_OPT_COUNT; row++) {
        CaroMenuOption r = (CaroMenuOption)row;
        bool enabled = !(game->mode == CARO_MODE_TWO_PLAYERS && (r == CARO_OPT_DIFFICULTY || r == CARO_OPT_FIRST));
        bool focusedRow = game->menuRow == row;
        Rectangle first = CaroMenuChipRect(r, 0);
        Color lc = enabled ? (focusedRow ? COL_ACCENT : COL_TEXT) : WithAlpha(COL_TEXT_DIM, 0.6f);
        if (focusedRow && enabled) {
            DrawRectangleRounded((Rectangle){x - 12.0f, first.y - 7.0f, PANEL.width - 32.0f, first.height + 14.0f},
                                 0.3f, 10, (Color){255, 255, 255, 12});
        }
        Text(OptionLabel(r), x, first.y + 12.0f, 20.0f, lc, true);
        int cur = CurrentOption(game, r);
        for (int i = 0; i < CaroMenuOptionCount(r); i++) {
            Chip(CaroMenuChipRect(r, i), OptionValue(r, i), i == cur, focusedRow && i == cur, enabled, t);
        }
    }

    // Thành tích với máy ở độ khó đang chọn
    float y = MENU_ROW_Y0 + CARO_OPT_COUNT * MENU_ROW_STEP + 6.0f;
    if (game->mode == CARO_MODE_VS_AI) {
        int d = game->difficulty;
        char rec[128];
        snprintf(rec, sizeof(rec), "Thành tích mức %s:  %d thắng · %d thua · %d hoà",
                 DIFF_NAMES[d], game->wins[d], game->losses[d], game->draws[d]);
        Text(rec, x, y, 17.0f, COL_TEXT_DIM, false);
    } else {
        Text("Hai người luân phiên trên cùng một máy.", x, y, 17.0f, COL_TEXT_DIM, false);
    }
    const char *ruleHelp = game->rule == CARO_RULE_BLOCKED
        ? "Chặn 2 đầu: hàng 5 bị đối phương chặn kín hai đầu không tính."
        : "Tự do: cứ đủ 5 quân liền nhau (hoặc hơn) là thắng.";
    Text(ruleHelp, x, y + 26.0f, 15.0f, WithAlpha(COL_TEXT_DIM, 0.85f), false);

    Rectangle start = CaroMenuStartRect();
    bool startFocus = game->menuRow == CARO_MENU_START_ROW;
    if (startFocus) {
        float pulse = 0.5f + 0.5f * sinf(t * 5.0f);
        DrawRectangleRoundedLinesEx((Rectangle){start.x - 4, start.y - 4, start.width + 8, start.height + 8}, 0.36f, 10,
                                    2.5f, WithAlpha(COL_ACCENT, 0.4f + 0.6f * pulse));
    }
    Button(start, "Bắt đầu chơi", "Enter", true, true);
}

static void DrawMenuHints(const CaroGame *game)
{
    const char *hint = "↑ ↓ chọn dòng  ·  ← → đổi lựa chọn  ·  1-3 độ khó  ·  Enter bắt đầu  ·  M âm thanh";
    (void)game;
    float w = TextWidth(hint, 15.0f, false);
    Text(hint, (CARO_CANVAS_W - w) * 0.5f, CARO_CANVAS_H - 24.0f, 15.0f, WithAlpha(COL_TEXT_DIM, 0.8f), false);
}

// ------------------------------------------------------------------ tạm dừng

static void DrawPause(const CaroGame *game)
{
    DrawRectangle(0, 0, CARO_CANVAS_W, CARO_CANVAS_H, (Color){4, 6, 10, 170});
    float pop = EaseOutBack(game->stateTime * 4.0f);
    Rectangle card = PAUSE_CARD;
    float s = 0.9f + 0.1f * pop;
    card.x += card.width * (1.0f - s) * 0.5f;
    card.y += card.height * (1.0f - s) * 0.5f;
    card.width *= s;
    card.height *= s;
    Panel(card);
    TextCentered("Tạm dừng", (Rectangle){card.x, card.y + 26.0f, card.width, 50.0f}, 38.0f, COL_TEXT, true);

    const char *rows[CARO_PAUSE_ROWS] = {"Tiếp tục", "Ván mới", "Về menu", game->soundEnabled ? "Âm thanh: Bật" : "Âm thanh: Tắt"};
    for (int i = 0; i < CARO_PAUSE_ROWS; i++) {
        Rectangle r = CaroPauseRowRect(i);
        bool focus = game->pauseRow == i;
        bool hover = Hovered(r);
        DrawRectangleRounded(r, 0.3f, 10, focus ? COL_ACCENT : (Color){255, 255, 255, (unsigned char)(hover ? 34 : 16)});
        TextCentered(rows[i], r, 21.0f, focus ? (Color){44, 28, 8, 255} : COL_TEXT, true);
    }
}

// ------------------------------------------------------------------ tổng

void DrawCaroGame(const CaroGame *game)
{
    ClearBackground(COL_BG_BOTTOM);
    DrawBackdrop(game->globalTime);

    if (game->state == CARO_STATE_MENU) {
        DrawMenuDemo(game);
        DrawMenuPanel(game);
        DrawMenuHints(game);
        return;
    }

    // Rung nhẹ khi ván kết thúc
    float shake = game->shake > 0.0f ? game->shake * 10.0f : 0.0f;
    Camera2D cam = {0};
    cam.zoom = 1.0f;
    cam.offset = (Vector2){sinf(game->globalTime * 70.0f) * shake, cosf(game->globalTime * 53.0f) * shake};
    BeginMode2D(cam);
        DrawBoard(game->board.n, true);
        DrawBoardContents(game);
    EndMode2D();

    DrawParticles(game);
    DrawSidePanel(game);
    if (game->state == CARO_STATE_OVER) DrawResultBanner(game);
    if (game->state == CARO_STATE_PAUSED) DrawPause(game);
}
