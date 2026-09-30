#include "chess_hud.h"
#include "chess_render.h"
#include "font_vn.h"
#include "perf_hint.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------------ bảng màu

static const Color COL_PANEL      = {12, 14, 20, 214};
static const Color COL_PANEL_LINE = {255, 255, 255, 28};
static const Color COL_TEXT       = {240, 238, 232, 255};
static const Color COL_TEXT_DIM   = {156, 158, 168, 255};
static const Color COL_ACCENT     = {236, 196, 120, 255};   // Vàng đồng - hợp tông đá cẩm thạch
static const Color COL_ACCENT_INK = {44, 30, 10, 255};
static const Color COL_CHECK      = {255, 96, 86, 255};
static const Color COL_GOOD       = {120, 226, 150, 255};

static const char *DIFF_NAMES[3] = {"Dễ", "Thường", "Khó"};

// Quân cờ Unicode (bộ đặc) - tô màu trắng/đen khi vẽ.
static const char *GLYPH[CHESS_PIECE_TYPES] = {"", "♟", "♞", "♝", "♜", "♛", "♚"};
static const char *PIECE_NAME[CHESS_PIECE_TYPES] = {"", "Tốt", "Mã", "Tượng", "Xe", "Hậu", "Vua"};
static const int PIECE_POINTS[CHESS_PIECE_TYPES] = {0, 1, 3, 3, 5, 9, 0};

const int CHESS_PROMOTION_TYPES[4] = {CHESS_QUEEN, CHESS_ROOK, CHESS_BISHOP, CHESS_KNIGHT};

// ------------------------------------------------------------------ bố cục

static const Rectangle PANEL = {1176.0f, 24.0f, 400.0f, 852.0f};
#define BOARD_CENTER_X 592.0f           // Tâm bàn cờ trên màn hình (theo độ dời ống kính)

#define MENU_ROW_Y0   262.0f
#define MENU_ROW_STEP 92.0f

int ChessMenuOptionCount(ChessMenuOption row);   // Định nghĩa ở chess_game.c

Rectangle ChessMenuChipRect(ChessMenuOption row, int index)
{
    int count = ChessMenuOptionCount(row);
    float x0 = PANEL.x + 26.0f;
    float total = PANEL.width - 52.0f;
    float gap = 8.0f;
    float w = (total - gap * (count - 1)) / (float)count;
    return (Rectangle){x0 + index * (w + gap), MENU_ROW_Y0 + row * MENU_ROW_STEP + 30.0f, w, 44.0f};
}

Rectangle ChessMenuStartRect(void)
{
    return (Rectangle){PANEL.x + 26.0f, PANEL.y + PANEL.height - 96.0f, PANEL.width - 52.0f, 64.0f};
}

Rectangle ChessPanelButtonRect(ChessPanelButton b)
{
    float gap = 10.0f;
    float w = (PANEL.width - 48.0f - gap) * 0.5f;
    float h = 48.0f;
    float y0 = PANEL.y + PANEL.height - 22.0f - h * 3.0f - gap * 2.0f;
    int col = b % 2, row = b / 2;
    return (Rectangle){PANEL.x + 24.0f + col * (w + gap), y0 + row * (h + gap), w, h};
}

static const Rectangle PROMO_CARD = {BOARD_CENTER_X - 280.0f, 318.0f, 560.0f, 250.0f};

Rectangle ChessPromotionRect(int index)
{
    float size = 112.0f, gap = 14.0f;
    float total = size * 4 + gap * 3;
    return (Rectangle){PROMO_CARD.x + (PROMO_CARD.width - total) * 0.5f + index * (size + gap),
                       PROMO_CARD.y + 84.0f, size, size + 28.0f};
}

static const Rectangle OVER_CARD = {BOARD_CENTER_X - 290.0f, 286.0f, 580.0f, 318.0f};

Rectangle ChessOverButtonRect(int index)
{
    float gap = 12.0f;
    float w = (OVER_CARD.width - 64.0f - gap * 2) / 3.0f;
    return (Rectangle){OVER_CARD.x + 32.0f + index * (w + gap), OVER_CARD.y + OVER_CARD.height - 84.0f, w, 56.0f};
}

static const Rectangle PAUSE_CARD = {CHESS_CANVAS_W * 0.5f - 220.0f, CHESS_CANVAS_H * 0.5f - 190.0f, 440.0f, 380.0f};

Rectangle ChessPauseRowRect(int row)
{
    return (Rectangle){PAUSE_CARD.x + 40.0f, PAUSE_CARD.y + 104.0f + row * 62.0f, PAUSE_CARD.width - 80.0f, 50.0f};
}

bool ChessPointInUi(const ChessGame *g, Vector2 p)
{
    if (CheckCollisionPointRec(p, PANEL)) return true;
    if (g->state == CHESS_STATE_PROMOTION && CheckCollisionPointRec(p, PROMO_CARD)) return true;
    if (g->state == CHESS_STATE_OVER && !g->overCardHidden && CheckCollisionPointRec(p, OVER_CARD)) return true;
    if (g->state == CHESS_STATE_PAUSED) return true;
    return false;
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

static Color WithAlpha(Color c, float a)
{
    c.a = (unsigned char)(Clamp01(a) * (float)c.a);
    return c;
}

static void Text(const char *t, float x, float y, float size, Color c, bool bold)
{
    if (bold) DrawTextVNBoldPro(t, (Vector2){x, y}, size, 0.0f, c);
    else DrawTextVNPro(t, (Vector2){x, y}, size, 0.0f, c);
}

static float TextWidth(const char *t, float size, bool bold)
{
    return bold ? MeasureTextVNBoldPro(t, size, 0.0f).x : MeasureTextVNPro(t, size, 0.0f).x;
}

static void TextCentered(const char *t, Rectangle r, float size, Color c, bool bold)
{
    Vector2 m = bold ? MeasureTextVNBoldPro(t, size, 0.0f) : MeasureTextVNPro(t, size, 0.0f);
    Text(t, r.x + (r.width - m.x) * 0.5f, r.y + (r.height - m.y) * 0.5f, size, c, bold);
}

static bool Hovered(Rectangle r)
{
    return CheckCollisionPointRec(GetMousePosition(), r);
}

static void SoftShadow(Rectangle r, float roundness, float spread, float alpha)
{
    if (PerfHintLite()) {
        // Máy yếu: một tầng thay cho sáu (mỗi tầng là một lớp phủ bán trong suốt).
        Rectangle s = {r.x - spread * 0.3f, r.y + spread * 0.2f, r.width + spread * 0.6f, r.height + spread * 0.5f};
        DrawRectangleRounded(s, roundness, 12, WithAlpha(BLACK, alpha * 0.5f));
        return;
    }
    for (int i = 6; i >= 1; i--) {
        float g = spread * (float)i / 6.0f;
        Rectangle s = {r.x - g, r.y - g + spread * 0.5f, r.width + g * 2.0f, r.height + g * 2.0f};
        DrawRectangleRounded(s, roundness, 12, WithAlpha(BLACK, alpha / 6.0f));
    }
}

static void Card(Rectangle r, float roundness)
{
    SoftShadow(r, roundness, 20.0f, 0.6f);
    DrawRectangleRounded(r, roundness, 12, COL_PANEL);
    DrawRectangleRoundedLinesEx(r, roundness, 12, 1.0f, COL_PANEL_LINE);
}

// Quân cờ Unicode có viền để đọc rõ trên mọi nền.
static void Glyph(int type, int color, Vector2 center, float size, float alpha)
{
    const char *g = GLYPH[type];
    Vector2 m = MeasureTextVNPro(g, size, 0.0f);
    Vector2 p = {center.x - m.x * 0.5f, center.y - m.y * 0.5f};
    Color fill = color == CHESS_WHITE ? (Color){248, 244, 234, 255} : (Color){34, 34, 40, 255};
    Color edge = color == CHESS_WHITE ? (Color){40, 36, 30, 255} : (Color){206, 204, 198, 255};
    float o = fmaxf(1.0f, size * 0.045f);
    for (int i = 0; i < 8; i++) {
        float a = (float)i * PI * 0.25f;
        DrawTextVNPro(g, (Vector2){p.x + cosf(a) * o, p.y + sinf(a) * o}, size, 0.0f, WithAlpha(edge, alpha));
    }
    DrawTextVNPro(g, p, size, 0.0f, WithAlpha(fill, alpha));
}

static void Chip(Rectangle r, const char *label, bool selected, bool focused, bool enabled, float t)
{
    Color fill = selected ? COL_ACCENT : (Color){255, 255, 255, 18};
    Color text = selected ? COL_ACCENT_INK : COL_TEXT;
    if (!enabled) {
        fill = selected ? (Color){120, 112, 100, 110} : (Color){255, 255, 255, 8};
        text = WithAlpha(COL_TEXT_DIM, 0.6f);
    } else if (!selected && Hovered(r)) {
        fill = (Color){255, 255, 255, 34};
    }
    DrawRectangleRounded(r, 0.4f, 10, fill);
    if (selected && enabled) DrawRectangleRounded((Rectangle){r.x, r.y, r.width, r.height * 0.5f}, 0.4f, 10, (Color){255, 255, 255, 40});
    if (focused && enabled) {
        float pulse = 0.6f + 0.4f * sinf(t * 5.0f);
        DrawRectangleRoundedLinesEx((Rectangle){r.x - 3, r.y - 3, r.width + 6, r.height + 6}, 0.45f, 10, 2.0f, WithAlpha(COL_ACCENT, pulse));
    }
    TextCentered(label, r, 18.0f, text, selected);
}

static void Button(Rectangle r, const char *label, const char *key, bool primary, bool enabled)
{
    bool hover = enabled && Hovered(r);
    Color fill = primary ? COL_ACCENT : (Color){255, 255, 255, hover ? 36 : 20};
    if (primary && hover) fill = (Color){246, 212, 146, 255};
    if (!enabled) fill = (Color){255, 255, 255, 8};
    DrawRectangleRounded(r, 0.32f, 10, fill);
    if (primary && enabled) DrawRectangleRounded((Rectangle){r.x + 2, r.y + 2, r.width - 4, r.height * 0.48f}, 0.32f, 10, (Color){255, 255, 255, 46});
    Color tc = primary ? COL_ACCENT_INK : COL_TEXT;
    if (!enabled) tc = WithAlpha(COL_TEXT_DIM, 0.5f);

    float size = primary ? 22.0f : 17.0f;
    float lw = TextWidth(label, size, true);
    float kw = key ? TextWidth(key, 13.0f, true) + 12.0f : 0.0f;
    float total = lw + (key ? kw + 8.0f : 0.0f);
    float x = r.x + (r.width - total) * 0.5f;
    Text(label, x, r.y + (r.height - size) * 0.5f - 1.0f, size, tc, true);
    if (key) {
        Rectangle kr = {x + lw + 8.0f, r.y + (r.height - 20.0f) * 0.5f, kw, 20.0f};
        DrawRectangleRounded(kr, 0.35f, 8, primary ? (Color){0, 0, 0, 40} : (Color){255, 255, 255, 26});
        TextCentered(key, kr, 13.0f, WithAlpha(tc, 0.85f), true);
    }
}

// ------------------------------------------------------------------ toạ độ trên viền bàn

static void DrawBoardCoords(const ChessGame *g)
{
    Camera3D cam = ChessCameraFromRig(&g->cam);
    float top = ChessBoardTop();
    // Chỉ ghi chữ ở hai cạnh gần người xem nhất
    float zEdge = cam.position.z < 0.0f ? -4.38f : 4.38f;
    float xEdge = cam.position.x < 0.0f ? -4.38f : 4.38f;
    for (int i = 0; i < 8; i++) {
        bool vis;
        char s[2] = {(char)('a' + i), 0};
        Vector2 p = ChessWorldToCanvas(g, (Vector3){3.5f - i, top, zEdge}, &vis);
        if (vis) {
            Vector2 m = MeasureTextVNBoldPro(s, 17.0f, 0.0f);
            Text(s, p.x - m.x * 0.5f, p.y - m.y * 0.5f, 17.0f, (Color){236, 228, 210, 150}, true);
        }
        char n[2] = {(char)('1' + i), 0};
        p = ChessWorldToCanvas(g, (Vector3){xEdge, top, i - 3.5f}, &vis);
        if (vis) {
            Vector2 m = MeasureTextVNBoldPro(n, 17.0f, 0.0f);
            Text(n, p.x - m.x * 0.5f, p.y - m.y * 0.5f, 17.0f, (Color){236, 228, 210, 150}, true);
        }
    }
}

// ------------------------------------------------------------------ bảng khi chơi

static const char *PlayerName(const ChessGame *g, int color)
{
    if (g->mode == CHESS_MODE_TWO_PLAYERS) return color == CHESS_WHITE ? "Bên Trắng" : "Bên Đen";
    return color == g->humanColor ? "Bạn" : "Máy";
}

// Quân đối phương mà `color` đã bắt được, và điểm vật chất.
static int CapturedBy(const ChessGame *g, int color, int *types, int maxTypes)
{
    int n = 0;
    for (int i = 0; i < g->board.ply && n < maxTypes; i++) {
        unsigned char cap = g->board.undo[i].captured;
        if (cap && CHESS_COLOR(cap) != color) types[n++] = CHESS_TYPE(cap);
    }
    // Sắp theo giá trị giảm dần cho gọn
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (PIECE_POINTS[types[j]] > PIECE_POINTS[types[i]]) { int t = types[i]; types[i] = types[j]; types[j] = t; }
    return n;
}

static int MaterialPoints(const ChessGame *g, int color)
{
    int pts = 0;
    for (int sq = 0; sq < 64; sq++) {
        unsigned char p = g->board.sq[sq];
        if (p && CHESS_COLOR(p) == color) pts += PIECE_POINTS[CHESS_TYPE(p)];
    }
    return pts;
}

static void DrawPlayerCard(const ChessGame *g, Rectangle r, int color)
{
    bool active = (g->state == CHESS_STATE_PLAYING || g->state == CHESS_STATE_PROMOTION) && g->board.side == color;
    bool isAI = g->mode == CHESS_MODE_VS_AI && color != g->humanColor;
    float t = g->globalTime;

    DrawRectangleRounded(r, 0.2f, 10, active ? (Color){236, 196, 120, 34} : (Color){255, 255, 255, 12});
    if (active) DrawRectangleRoundedLinesEx(r, 0.2f, 10, 2.0f, WithAlpha(COL_ACCENT, 0.55f + 0.45f * sinf(t * 4.0f)));

    Rectangle icon = {r.x + 12.0f, r.y + 12.0f, 50.0f, 50.0f};
    DrawRectangleRounded(icon, 0.3f, 8, color == CHESS_WHITE ? (Color){90, 88, 84, 255} : (Color){200, 196, 188, 255});
    Glyph(CHESS_KING, color, (Vector2){icon.x + 25.0f, icon.y + 25.0f}, 40.0f, 1.0f);

    Text(PlayerName(g, color), icon.x + 64.0f, r.y + 12.0f, 22.0f, COL_TEXT, true);
    char sub[64];
    if (isAI) snprintf(sub, sizeof(sub), "Máy · %s", DIFF_NAMES[g->difficulty]);
    else snprintf(sub, sizeof(sub), "%s", color == CHESS_WHITE ? "Quân trắng · đi trước" : "Quân đen");
    Text(sub, icon.x + 64.0f, r.y + 40.0f, 15.0f, COL_TEXT_DIM, false);

    // Quân đã ăn được + chênh lệch vật chất
    int types[16];
    int n = CapturedBy(g, color, types, 16);
    float x = icon.x + 2.0f;
    for (int i = 0; i < n; i++) {
        Glyph(types[i], color ^ 1, (Vector2){x + 9.0f, r.y + r.height - 16.0f}, 21.0f, 0.95f);
        x += (i + 1 < n && types[i + 1] == types[i]) ? 11.0f : 18.0f;
    }
    int diff = MaterialPoints(g, color) - MaterialPoints(g, color ^ 1);
    if (diff > 0) {
        char s[16];
        snprintf(s, sizeof(s), "+%d", diff);
        Text(s, x + 6.0f, r.y + r.height - 27.0f, 16.0f, COL_TEXT_DIM, true);
    }

    if (active) {
        char status[40];
        if (isAI && g->aiThinking) snprintf(status, sizeof(status), "Đang nghĩ%.*s", (int)(t * 3.0f) % 4, "...");
        else snprintf(status, sizeof(status), "Đến lượt");
        float w = TextWidth(status, 15.0f, true);
        Rectangle pill = {r.x + r.width - w - 32.0f, r.y + 14.0f, w + 20.0f, 26.0f};
        DrawRectangleRounded(pill, 0.5f, 8, COL_ACCENT);
        TextCentered(status, pill, 15.0f, COL_ACCENT_INK, true);
    }
}

static void DrawMoveList(const ChessGame *g, Rectangle area)
{
    DrawRectangleRounded(area, 0.06f, 10, (Color){0, 0, 0, 70});
    Text("Biên bản ván cờ", area.x + 14.0f, area.y + 10.0f, 16.0f, COL_TEXT_DIM, true);

    float rowH = 26.0f;
    float top = area.y + 38.0f;
    int visibleRows = (int)((area.height - 46.0f) / rowH);
    int totalRows = (g->sanCount + 1) / 2;
    if (totalRows == 0) {
        Text("Chưa có nước đi nào.", area.x + 14.0f, top + 4.0f, 16.0f, WithAlpha(COL_TEXT_DIM, 0.7f), false);
        return;
    }
    int maxScroll = totalRows > visibleRows ? totalRows - visibleRows : 0;
    int scroll = g->moveListScroll > maxScroll ? maxScroll : g->moveListScroll;
    int firstRow = totalRows - visibleRows - scroll;
    if (firstRow < 0) firstRow = 0;

    for (int row = firstRow; row < totalRows && row < firstRow + visibleRows; row++) {
        float y = top + (row - firstRow) * rowH;
        if (row % 2 == 0) DrawRectangle((int)area.x + 6, (int)y - 2, (int)area.width - 12, (int)rowH, (Color){255, 255, 255, 8});
        char num[16];
        snprintf(num, sizeof(num), "%d.", row + 1);
        Text(num, area.x + 14.0f, y + 2.0f, 16.0f, COL_TEXT_DIM, false);
        for (int k = 0; k < 2; k++) {
            int idx = row * 2 + k;
            if (idx >= g->sanCount) break;
            bool latest = idx == g->sanCount - 1;
            float x = area.x + 64.0f + k * 130.0f;
            if (latest) DrawRectangleRounded((Rectangle){x - 6.0f, y - 1.0f, 100.0f, rowH - 4.0f}, 0.3f, 6, WithAlpha(COL_ACCENT, 0.25f));
            Color c = strchr(g->san[idx], '#') ? COL_CHECK : (latest ? COL_ACCENT : COL_TEXT);
            Text(g->san[idx], x, y + 2.0f, 17.0f, c, latest);
        }
    }
    if (maxScroll > 0) {
        // Thanh cuộn mảnh
        float trackH = visibleRows * rowH;
        float thumbH = trackH * (float)visibleRows / (float)totalRows;
        float thumbY = top + (trackH - thumbH) * (1.0f - (float)scroll / (float)maxScroll);
        DrawRectangleRounded((Rectangle){area.x + area.width - 9.0f, thumbY, 4.0f, thumbH}, 1.0f, 4, (Color){255, 255, 255, 60});
    }
}

static void DrawStatusLine(const ChessGame *g, float x, float y)
{
    const char *msg = NULL;
    Color c = COL_TEXT_DIM;
    if (g->state == CHESS_STATE_OVER) {
        msg = g->result == CHESS_RESULT_DRAW ? "Ván cờ hoà" : "Ván cờ đã kết thúc";
    } else if (ChessInCheck(&g->board, g->board.side)) {
        msg = "Chiếu tướng!";
        c = COL_CHECK;
    } else if (g->hintPending) {
        msg = "Đang tìm nước gợi ý…";
        c = COL_GOOD;
    } else if (g->hintMove != CHESS_MOVE_NONE) {
        msg = "Gợi ý: đi theo ô sáng màu xanh";
        c = COL_GOOD;
    } else if (g->selected >= 0) {
        msg = "Chọn ô đích (chấm xanh), bấm lại để bỏ chọn";
    } else {
        msg = g->board.side == CHESS_WHITE ? "Lượt bên Trắng" : "Lượt bên Đen";
    }
    Text(msg, x, y, 17.0f, c, c.r == COL_CHECK.r);
}

static void DrawSidePanel(const ChessGame *g)
{
    Card(PANEL, 0.035f);
    float x = PANEL.x + 24.0f;
    float y = PANEL.y + 20.0f;
    float w = PANEL.width - 48.0f;

    Text("Cờ Vua 3D", x, y, 36.0f, COL_TEXT, true);
    char sub[80];
    if (g->mode == CHESS_MODE_VS_AI) snprintf(sub, sizeof(sub), "Đấu với máy · Độ khó %s", DIFF_NAMES[g->difficulty]);
    else snprintf(sub, sizeof(sub), "Hai người chơi trên một máy");
    Text(sub, x, y + 44.0f, 16.0f, COL_TEXT_DIM, false);
    y += 80.0f;

    // Đối thủ ở trên, người chơi ở dưới (theo hướng nhìn)
    int bottomColor = (g->mode == CHESS_MODE_VS_AI) ? g->humanColor : CHESS_WHITE;
    DrawPlayerCard(g, (Rectangle){x, y, w, 100.0f}, bottomColor ^ 1);
    y += 110.0f;
    DrawPlayerCard(g, (Rectangle){x, y, w, 100.0f}, bottomColor);
    y += 116.0f;

    DrawStatusLine(g, x + 2.0f, y);
    y += 32.0f;

    float listBottom = ChessPanelButtonRect(CHESS_BTN_UNDO).y - 16.0f;
    DrawMoveList(g, (Rectangle){x, y, w, listBottom - y});

    bool playing = g->state == CHESS_STATE_PLAYING;
    bool humanTurn = g->mode == CHESS_MODE_TWO_PLAYERS || g->board.side == g->humanColor;
    Button(ChessPanelButtonRect(CHESS_BTN_UNDO), "Hoàn tác", "U", false, playing && g->board.ply > 0);
    Button(ChessPanelButtonRect(CHESS_BTN_HINT), g->hintPending ? "Đang tìm…" : "Gợi ý", "H", false,
           playing && humanTurn && !g->aiThinking);
    Button(ChessPanelButtonRect(CHESS_BTN_FLIP), "Xoay bàn", "F", false, true);
    Button(ChessPanelButtonRect(CHESS_BTN_VIEW), g->cam.topDown ? "Góc 3D" : "Nhìn từ trên", "V", false, true);
    Button(ChessPanelButtonRect(CHESS_BTN_NEW), "Ván mới", "N", false, true);
    Button(ChessPanelButtonRect(CHESS_BTN_PAUSE), "Tạm dừng", "Esc", false, playing);
}

// ------------------------------------------------------------------ hộp thoại

static void DrawPromotion(const ChessGame *g)
{
    float pop = EaseOutBack(g->stateTime * 4.0f);
    Rectangle card = PROMO_CARD;
    float s = 0.9f + 0.1f * pop;
    card.x += card.width * (1.0f - s) * 0.5f;
    card.y += card.height * (1.0f - s) * 0.5f;
    card.width *= s;
    card.height *= s;
    Card(card, 0.08f);
    TextCentered("Phong cấp tốt thành…", (Rectangle){card.x, card.y + 22.0f, card.width, 40.0f}, 28.0f, COL_TEXT, true);

    int color = g->board.side;
    for (int i = 0; i < 4; i++) {
        Rectangle r = ChessPromotionRect(i);
        bool focus = g->promotionChoice == i;
        bool hover = Hovered(r);
        DrawRectangleRounded(r, 0.18f, 10, focus ? WithAlpha(COL_ACCENT, 0.9f) : (Color){255, 255, 255, (unsigned char)(hover ? 40 : 20)});
        Glyph(CHESS_PROMOTION_TYPES[i], color, (Vector2){r.x + r.width * 0.5f, r.y + 54.0f}, 70.0f, 1.0f);
        char label[24];
        snprintf(label, sizeof(label), "%d · %s", i + 1, PIECE_NAME[CHESS_PROMOTION_TYPES[i]]);
        TextCentered(label, (Rectangle){r.x, r.y + r.height - 32.0f, r.width, 26.0f}, 16.0f,
                     focus ? COL_ACCENT_INK : COL_TEXT, true);
    }
}

static const char *EndReasonText(const ChessGame *g)
{
    switch (g->endReason) {
        case CHESS_CHECKMATE:       return "Chiếu hết";
        case CHESS_STALEMATE:       return "Hết nước đi (hoà pat)";
        case CHESS_DRAW_FIFTY:      return "Luật 50 nước không bắt quân, không đi tốt";
        case CHESS_DRAW_REPETITION: return "Thế cờ lặp lại 3 lần";
        case CHESS_DRAW_MATERIAL:   return "Không đủ quân để chiếu hết";
        default:                    return "";
    }
}

static void DrawOverCard(const ChessGame *g)
{
    if (g->overCardHidden) {
        const char *hint = "Đang xem lại bàn cờ — bấm Enter hoặc click để mở lại bảng kết quả";
        float w = TextWidth(hint, 17.0f, false);
        Rectangle pill = {BOARD_CENTER_X - w * 0.5f - 18.0f, 26.0f, w + 36.0f, 36.0f};
        DrawRectangleRounded(pill, 0.5f, 10, COL_PANEL);
        TextCentered(hint, pill, 17.0f, COL_TEXT, false);
        return;
    }

    float pop = EaseOutBack(g->resultTime * 3.0f);
    Rectangle card = OVER_CARD;
    float s = 0.88f + 0.12f * pop;
    card.x += card.width * (1.0f - s) * 0.5f;
    card.y += card.height * (1.0f - s) * 0.5f;
    card.width *= s;
    card.height *= s;
    Card(card, 0.07f);

    const char *title;
    Color c = COL_ACCENT;
    int winner = g->result == CHESS_RESULT_WHITE_WINS ? CHESS_WHITE : CHESS_BLACK;
    if (g->result == CHESS_RESULT_DRAW) {
        title = "Hoà cờ";
    } else if (g->mode == CHESS_MODE_VS_AI) {
        bool won = winner == g->humanColor;
        title = won ? "Bạn thắng!" : "Máy thắng";
        c = won ? COL_GOOD : COL_CHECK;
    } else {
        title = winner == CHESS_WHITE ? "Bên Trắng thắng!" : "Bên Đen thắng!";
    }

    if (g->result != CHESS_RESULT_DRAW) {
        Glyph(CHESS_KING, winner, (Vector2){card.x + 70.0f, card.y + 78.0f}, 76.0f, 1.0f);
    } else {
        Glyph(CHESS_KING, CHESS_WHITE, (Vector2){card.x + 56.0f, card.y + 78.0f}, 64.0f, 1.0f);
        Glyph(CHESS_KING, CHESS_BLACK, (Vector2){card.x + 92.0f, card.y + 78.0f}, 64.0f, 1.0f);
    }
    Text(title, card.x + 130.0f, card.y + 36.0f, 46.0f, c, true);
    char reason[128];
    snprintf(reason, sizeof(reason), "%s sau %d nước", EndReasonText(g), (g->board.ply + 1) / 2);
    Text(reason, card.x + 132.0f, card.y + 96.0f, 18.0f, COL_TEXT_DIM, false);

    if (g->mode == CHESS_MODE_VS_AI) {
        int d = g->difficulty;
        char rec[128];
        snprintf(rec, sizeof(rec), "Thành tích mức %s:  %d thắng · %d thua · %d hoà",
                 DIFF_NAMES[d], g->wins[d], g->losses[d], g->draws[d]);
        Text(rec, card.x + 34.0f, card.y + 160.0f, 18.0f, COL_TEXT, false);
    }

    Button(ChessOverButtonRect(0), "Ván mới", "Enter", true, true);
    Button(ChessOverButtonRect(1), "Xem bàn cờ", "Space", false, true);
    Button(ChessOverButtonRect(2), "Về menu", "Esc", false, true);
}

static void DrawPause(const ChessGame *g)
{
    DrawRectangle(0, 0, CHESS_CANVAS_W, CHESS_CANVAS_H, (Color){4, 6, 10, 160});
    float pop = EaseOutBack(g->stateTime * 4.0f);
    Rectangle card = PAUSE_CARD;
    float s = 0.9f + 0.1f * pop;
    card.x += card.width * (1.0f - s) * 0.5f;
    card.y += card.height * (1.0f - s) * 0.5f;
    card.width *= s;
    card.height *= s;
    Card(card, 0.06f);
    TextCentered("Tạm dừng", (Rectangle){card.x, card.y + 26.0f, card.width, 50.0f}, 38.0f, COL_TEXT, true);
    const char *rows[CHESS_PAUSE_ROWS] = {"Tiếp tục", "Ván mới", "Về menu", g->soundEnabled ? "Âm thanh: Bật" : "Âm thanh: Tắt"};
    for (int i = 0; i < CHESS_PAUSE_ROWS; i++) {
        Rectangle r = ChessPauseRowRect(i);
        bool focus = g->pauseRow == i;
        DrawRectangleRounded(r, 0.3f, 10, focus ? COL_ACCENT : (Color){255, 255, 255, (unsigned char)(Hovered(r) ? 34 : 16)});
        TextCentered(rows[i], r, 21.0f, focus ? COL_ACCENT_INK : COL_TEXT, true);
    }
}

// ------------------------------------------------------------------ menu

static const char *OptionLabel(ChessMenuOption row)
{
    switch (row) {
        case CHESS_OPT_MODE:       return "Chế độ chơi";
        case CHESS_OPT_DIFFICULTY: return "Độ khó của máy";
        case CHESS_OPT_SIDE:       return "Bạn cầm quân";
        case CHESS_OPT_QUALITY:    return "Chất lượng đồ hoạ";
        default:                   return "";
    }
}

static const char *OptionValue(ChessMenuOption row, int i)
{
    static const char *MODE[] = {"Đấu với máy", "2 người"};
    static const char *SIDE[] = {"Trắng", "Đen", "Ngẫu nhiên"};
    static const char *QUALITY[] = {"Cao", "Nhẹ (máy yếu)"};
    switch (row) {
        case CHESS_OPT_MODE:       return MODE[i];
        case CHESS_OPT_DIFFICULTY: return DIFF_NAMES[i];
        case CHESS_OPT_SIDE:       return SIDE[i];
        case CHESS_OPT_QUALITY:    return QUALITY[i];
        default:                   return "";
    }
}

static int CurrentOption(const ChessGame *g, ChessMenuOption row)
{
    switch (row) {
        case CHESS_OPT_MODE:       return g->mode;
        case CHESS_OPT_DIFFICULTY: return g->difficulty;
        case CHESS_OPT_SIDE:       return g->sideChoice;
        case CHESS_OPT_QUALITY:    return g->highQuality ? 0 : 1;
        default:                   return 0;
    }
}

static void DrawMenu(const ChessGame *g)
{
    float t = g->globalTime;
    Card(PANEL, 0.035f);
    float x = PANEL.x + 26.0f;

    Glyph(CHESS_KNIGHT, CHESS_WHITE, (Vector2){x + 34.0f, PANEL.y + 70.0f}, 74.0f, 1.0f);
    Text("CỜ VUA", x + 84.0f, PANEL.y + 30.0f, 52.0f, COL_TEXT, true);
    Text("3D · bộ cờ đá cẩm thạch", x + 86.0f, PANEL.y + 88.0f, 17.0f, COL_TEXT_DIM, false);
    // Ngắt dòng thủ công cho gọn trong bảng
    Text("Luật cờ vua quốc tế đầy đủ: nhập thành,", x, PANEL.y + 140.0f, 16.0f, WithAlpha(COL_TEXT_DIM, 0.9f), false);
    Text("bắt tốt qua đường, phong cấp, hoà pat…", x, PANEL.y + 162.0f, 16.0f, WithAlpha(COL_TEXT_DIM, 0.9f), false);
    DrawLineEx((Vector2){x, PANEL.y + 200.0f}, (Vector2){PANEL.x + PANEL.width - 26.0f, PANEL.y + 200.0f}, 1.0f, COL_PANEL_LINE);

    for (int row = 0; row < CHESS_OPT_COUNT; row++) {
        ChessMenuOption r = (ChessMenuOption)row;
        bool enabled = !(g->mode == CHESS_MODE_TWO_PLAYERS && (r == CHESS_OPT_DIFFICULTY || r == CHESS_OPT_SIDE));
        bool focusedRow = g->menuRow == row;
        float ry = MENU_ROW_Y0 + row * MENU_ROW_STEP;
        if (focusedRow && enabled) {
            DrawRectangleRounded((Rectangle){x - 12.0f, ry - 6.0f, PANEL.width - 28.0f, 88.0f}, 0.18f, 10, (Color){255, 255, 255, 12});
        }
        Color lc = enabled ? (focusedRow ? COL_ACCENT : COL_TEXT) : WithAlpha(COL_TEXT_DIM, 0.6f);
        Text(OptionLabel(r), x, ry, 18.0f, lc, true);
        int cur = CurrentOption(g, r);
        for (int i = 0; i < ChessMenuOptionCount(r); i++) {
            Chip(ChessMenuChipRect(r, i), OptionValue(r, i), i == cur, focusedRow && i == cur, enabled, t);
        }
    }

    float y = MENU_ROW_Y0 + CHESS_OPT_COUNT * MENU_ROW_STEP + 4.0f;
    if (g->mode == CHESS_MODE_VS_AI) {
        int d = g->difficulty;
        char rec[128];
        snprintf(rec, sizeof(rec), "Thành tích mức %s: %d thắng · %d thua · %d hoà", DIFF_NAMES[d], g->wins[d], g->losses[d], g->draws[d]);
        Text(rec, x, y, 16.0f, COL_TEXT_DIM, false);
    }

    Rectangle start = ChessMenuStartRect();
    if (g->menuRow == CHESS_MENU_START_ROW) {
        float pulse = 0.5f + 0.5f * sinf(t * 5.0f);
        DrawRectangleRoundedLinesEx((Rectangle){start.x - 4, start.y - 4, start.width + 8, start.height + 8}, 0.36f, 10, 2.5f,
                                    WithAlpha(COL_ACCENT, 0.4f + 0.6f * pulse));
    }
    Button(start, "Bắt đầu ván cờ", "Enter", true, true);

    const char *hint = "↑ ↓ chọn dòng  ·  ← → đổi lựa chọn  ·  1-3 độ khó  ·  Enter bắt đầu  ·  Chuột phải kéo để xoay bàn";
    Text(hint, 28.0f, CHESS_CANVAS_H - 34.0f, 16.0f, WithAlpha(COL_TEXT_DIM, 0.85f), false);
}

// ------------------------------------------------------------------ tổng

void DrawChessHud(const ChessGame *g)
{
    if (g->state == CHESS_STATE_MENU) {
        DrawMenu(g);
        return;
    }
    DrawBoardCoords(g);
    DrawSidePanel(g);

    // Dòng hướng dẫn chỉ hiện ở đầu ván rồi mờ dần, khỏi đè lên mép bàn cờ.
    float hintAlpha = (g->board.ply < 2) ? 1.0f : Clamp01((10.0f - g->stateTime) / 2.0f);
    if (hintAlpha > 0.0f) {
        const char *hint = "Chuột trái: chọn & đi quân  ·  Chuột phải kéo: xoay  ·  Lăn chuột: phóng to  ·  Mũi tên + Enter  ·  C: đặt lại góc nhìn";
        float w = TextWidth(hint, 15.0f, false);
        Rectangle pill = {BOARD_CENTER_X - w * 0.5f - 16.0f, CHESS_CANVAS_H - 44.0f, w + 32.0f, 30.0f};
        DrawRectangleRounded(pill, 0.5f, 10, WithAlpha(COL_PANEL, hintAlpha));
        TextCentered(hint, pill, 15.0f, WithAlpha(COL_TEXT_DIM, hintAlpha), false);
    }

    if (g->checkFlash > 0.0f && g->state == CHESS_STATE_PLAYING) {
        float a = Clamp01(g->checkFlash);
        const char *txt = "CHIẾU!";
        float size = 64.0f + (1.0f - a) * 20.0f;
        float w = TextWidth(txt, size, true);
        Text(txt, BOARD_CENTER_X - w * 0.5f, 90.0f, size, WithAlpha(COL_CHECK, a), true);
    }

    if (g->state == CHESS_STATE_PROMOTION) DrawPromotion(g);
    if (g->state == CHESS_STATE_OVER) DrawOverCard(g);
    if (g->state == CHESS_STATE_PAUSED) DrawPause(g);
}
