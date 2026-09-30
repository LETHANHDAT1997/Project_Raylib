#include "chess_game.h"
#include "chess_render.h"
#include "chess_hud.h"
#include "chess_assets.h"
#include "save_data.h"
#include "raymath.h"
#include "perf_hint.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Khoá lưu dữ liệu (common/save_data) - trùng id của game trong Arcade Hub.
#define CHESS_SAVE_ID "chess"

static const char *DIFF_KEYS[3] = {"easy", "normal", "hard"};
static const float MIN_THINK_TIME[3] = {0.7f, 0.6f, 0.4f};

#define CAM_DEFAULT_PITCH  0.95f
#define CAM_TOP_PITCH      1.52f
#define CAM_DEFAULT_DIST   16.0f
#define CAM_MIN_DIST       8.5f
#define CAM_MAX_DIST       22.0f
#define CAM_MENU_SPIN      0.12f
#define RESULT_INPUT_DELAY 0.6f

// ------------------------------------------------------------------ lưu trữ

static void LoadStats(ChessGame *g)
{
    for (int i = 0; i < 3; i++) {
        char key[32];
        snprintf(key, sizeof(key), "win.%s", DIFF_KEYS[i]);
        g->wins[i] = SaveDataGetInt(CHESS_SAVE_ID, key, 0);
        snprintf(key, sizeof(key), "loss.%s", DIFF_KEYS[i]);
        g->losses[i] = SaveDataGetInt(CHESS_SAVE_ID, key, 0);
        snprintf(key, sizeof(key), "draw.%s", DIFF_KEYS[i]);
        g->draws[i] = SaveDataGetInt(CHESS_SAVE_ID, key, 0);
    }
}

static void SaveStats(const ChessGame *g)
{
    int total = 0;
    for (int i = 0; i < 3; i++) {
        char key[32];
        snprintf(key, sizeof(key), "win.%s", DIFF_KEYS[i]);
        SaveDataSetInt(CHESS_SAVE_ID, key, g->wins[i]);
        snprintf(key, sizeof(key), "loss.%s", DIFF_KEYS[i]);
        SaveDataSetInt(CHESS_SAVE_ID, key, g->losses[i]);
        snprintf(key, sizeof(key), "draw.%s", DIFF_KEYS[i]);
        SaveDataSetInt(CHESS_SAVE_ID, key, g->draws[i]);
        total += g->wins[i];
    }
    // "best" là con số Hub hiển thị: tổng số ván thắng máy.
    SaveDataSetInt(CHESS_SAVE_ID, "best", total);
}

// ------------------------------------------------------------------ tiện ích

static void SetState(ChessGame *g, ChessState s)
{
    g->state = s;
    g->stateTime = 0.0f;
}

static void Sfx(const ChessGame *g, ChessSfx sfx)
{
    PlayChessSfx(sfx, g->soundEnabled, (sfx == CHESS_SFX_MOVE || sfx == CHESS_SFX_CAPTURE) ? 0.05f : 0.0f);
}

static bool ClickedIn(Rectangle r)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r);
}

static bool ConfirmPressed(void)
{
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
}

static bool VsAI(const ChessGame *g)
{
    return g->mode == CHESS_MODE_VS_AI;
}

static bool HumanTurn(const ChessGame *g)
{
    return !VsAI(g) || g->board.side == g->humanColor;
}

// Bên ngồi "phía trước" màn hình - quyết định chỗ xếp quân bị bắt.
static int ViewerColor(const ChessGame *g)
{
    return VsAI(g) ? g->humanColor : CHESS_WHITE;
}

static void RefreshLegal(ChessGame *g)
{
    g->legalCount = ChessGenLegal(&g->board, g->legal);
}

static void CancelThinking(ChessGame *g)
{
    ChessAICancel();
    g->aiThinking = false;
    g->hintPending = false;
    g->aiTimer = 0.0f;
}

// ------------------------------------------------------------------ hình 3D của quân

static bool AnyAnimating(const ChessGame *g)
{
    for (int i = 0; i < CHESS_MAX_VISUALS; i++) {
        if (g->visuals[i].alive && g->visuals[i].moving) return true;
    }
    return false;
}

static ChessVisual *VisualAt(ChessGame *g, int sq)
{
    for (int i = 0; i < CHESS_MAX_VISUALS; i++) {
        ChessVisual *v = &g->visuals[i];
        if (v->alive && !v->captured && v->square == sq) return v;
    }
    return NULL;
}

static ChessVisual *NewVisual(ChessGame *g)
{
    for (int i = 0; i < CHESS_MAX_VISUALS; i++) {
        if (!g->visuals[i].alive) return &g->visuals[i];
    }
    return NULL;
}

// Dựng lại toàn bộ hình từ thế cờ (dùng khi bắt đầu ván và khi hoàn tác).
static void SyncVisuals(ChessGame *g)
{
    memset(g->visuals, 0, sizeof(g->visuals));
    g->capturedCount[0] = g->capturedCount[1] = 0;
    for (int sq = 0; sq < 64; sq++) {
        unsigned char p = g->board.sq[sq];
        if (!p) continue;
        ChessVisual *v = NewVisual(g);
        if (!v) break;
        *v = (ChessVisual){.type = CHESS_TYPE(p), .color = CHESS_COLOR(p), .square = sq,
                           .pos = ChessSquareWorld(sq), .alive = true};
    }
    // Quân đã bị bắt lấy theo đúng thứ tự trong lịch sử nước đi.
    for (int i = 0; i < g->board.ply; i++) {
        unsigned char cap = g->board.undo[i].captured;
        if (!cap) continue;
        ChessVisual *v = NewVisual(g);
        if (!v) break;
        int color = CHESS_COLOR(cap);
        *v = (ChessVisual){.type = CHESS_TYPE(cap), .color = color, .square = -1, .captured = true, .alive = true,
                           .pos = ChessGraveWorld(color, g->capturedCount[color]++, ViewerColor(g))};
    }
}

static void StartTween(ChessVisual *v, Vector3 to, float duration, float arc, float delay)
{
    v->from = v->pos;
    v->to = to;
    v->t = 0.0f;
    v->duration = duration;
    v->arc = arc;
    v->delay = delay;
    v->moving = true;
}

// Tạo hoạt ảnh cho một nước đi. Gọi TRƯỚC ChessMakeMove.
static void AnimateMove(ChessGame *g, ChessMove m)
{
    int from = CHESS_MOVE_FROM(m), to = CHESS_MOVE_TO(m), flags = CHESS_MOVE_FLAGS(m);
    ChessVisual *mover = VisualAt(g, from);

    if (flags & CHESS_FLAG_CAPTURE) {
        int capSq = (flags & CHESS_FLAG_EP) ? to + (g->board.side == CHESS_WHITE ? -8 : 8) : to;
        ChessVisual *victim = VisualAt(g, capSq);
        if (victim) {
            int color = victim->color;
            victim->captured = true;
            victim->square = -1;
            StartTween(victim, ChessGraveWorld(color, g->capturedCount[color]++, ViewerColor(g)), 0.6f, 1.4f, 0.26f);
        }
    }
    if (mover) {
        int dist = abs(CHESS_FILE(from) - CHESS_FILE(to)) + abs(CHESS_RANK(from) - CHESS_RANK(to));
        bool knight = mover->type == CHESS_KNIGHT;
        float duration = 0.22f + 0.035f * (float)dist;
        float arc = knight ? 0.9f : 0.25f + 0.04f * (float)dist;
        mover->square = to;
        StartTween(mover, ChessSquareWorld(to), duration, arc, 0.0f);
        mover->promoteTo = CHESS_MOVE_PROMO(m);
    }
    if (flags & CHESS_FLAG_CASTLE) {
        int rookFrom = (to > from) ? to + 1 : to - 2;
        int rookTo = (to > from) ? to - 1 : to + 1;
        ChessVisual *rook = VisualAt(g, rookFrom);
        if (rook) {
            rook->square = rookTo;
            StartTween(rook, ChessSquareWorld(rookTo), 0.4f, 0.7f, 0.12f);
        }
    }
}

static float EaseInOut(float t)
{
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}

static void UpdateVisuals(ChessGame *g, float dt)
{
    for (int i = 0; i < CHESS_MAX_VISUALS; i++) {
        ChessVisual *v = &g->visuals[i];
        if (!v->alive || !v->moving) continue;
        if (v->delay > 0.0f) {
            v->delay -= dt;
            continue;
        }
        v->t += dt / (v->duration > 0.01f ? v->duration : 0.01f);
        if (v->t >= 1.0f) {
            v->t = 1.0f;
            v->moving = false;
            v->pos = v->to;
            if (v->promoteTo) {
                v->type = v->promoteTo;
                v->promoteTo = 0;
            }
            continue;
        }
        float e = EaseInOut(v->t);
        v->pos = Vector3Lerp(v->from, v->to, e);
        v->pos.y += sinf(v->t * PI) * v->arc;
    }
}

// ------------------------------------------------------------------ camera

static void ResetCamera(ChessGame *g, bool snap)
{
    ChessCameraRig *c = &g->cam;
    c->targetYaw = (VsAI(g) && g->humanColor == CHESS_BLACK) ? PI : 0.0f;
    c->targetPitch = c->topDown ? CAM_TOP_PITCH : CAM_DEFAULT_PITCH;
    c->targetDistance = CAM_DEFAULT_DIST;
    if (snap) {
        c->yaw = c->targetYaw;
        c->pitch = c->targetPitch;
        c->distance = c->targetDistance;
    }
}

static void UpdateCameraRig(ChessGame *g, float dt, bool allowInput)
{
    ChessCameraRig *c = &g->cam;
    Vector2 mouse = GetMousePosition();

    if (allowInput) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !ChessPointInUi(g, mouse)) c->dragging = true;
        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) c->dragging = false;
        if (c->dragging) {
            Vector2 d = GetMouseDelta();
            c->targetYaw -= d.x * 0.008f;
            c->targetPitch += d.y * 0.006f;
            c->topDown = false;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && !ChessPointInUi(g, mouse)) c->targetDistance -= wheel * 0.9f;
    }
    c->targetPitch = Clamp(c->targetPitch, 0.32f, CAM_TOP_PITCH);
    c->targetDistance = Clamp(c->targetDistance, CAM_MIN_DIST, CAM_MAX_DIST);

    float k = 1.0f - expf(-dt * 7.0f);
    c->yaw += (c->targetYaw - c->yaw) * k;
    c->pitch += (c->targetPitch - c->pitch) * k;
    c->distance += (c->targetDistance - c->distance) * k;
}

static void ToggleTopDown(ChessGame *g)
{
    g->cam.topDown = !g->cam.topDown;
    g->cam.targetPitch = g->cam.topDown ? CAM_TOP_PITCH : CAM_DEFAULT_PITCH;
    Sfx(g, CHESS_SFX_SELECT);
}

static void FlipView(ChessGame *g)
{
    // Làm tròn về hướng gần nhất của một bên rồi xoay nửa vòng
    float yaw = roundf(g->cam.targetYaw / PI) * PI;
    g->cam.targetYaw = yaw + PI;
    Sfx(g, CHESS_SFX_SELECT);
}

// ------------------------------------------------------------------ vòng đời ván

static void StartMatch(ChessGame *g)
{
    CancelThinking(g);
    ChessBoardInitStart(&g->board);
    if (g->sideChoice == CHESS_SIDE_RANDOM) g->humanColor = GetRandomValue(0, 1);
    else g->humanColor = (g->sideChoice == CHESS_SIDE_BLACK) ? CHESS_BLACK : CHESS_WHITE;
    if (!VsAI(g)) g->humanColor = CHESS_WHITE;

    g->selected = -1;
    g->hoverSquare = -1;
    g->cursor = (g->humanColor == CHESS_WHITE) ? CHESS_SQ(4, 1) : CHESS_SQ(4, 6);
    g->sanCount = 0;
    g->moveListScroll = 0;
    g->lastMove = CHESS_MOVE_NONE;
    g->hintMove = CHESS_MOVE_NONE;
    g->pendingPromotion = CHESS_MOVE_NONE;
    g->result = CHESS_RESULT_NONE;
    g->endReason = CHESS_ONGOING;
    g->overCardHidden = false;
    RefreshLegal(g);
    SyncVisuals(g);
    ResetCamera(g, false);
    SetState(g, CHESS_STATE_PLAYING);
}

static void GoToMenu(ChessGame *g)
{
    CancelThinking(g);
    ChessBoardInitStart(&g->board);
    g->selected = -1;
    g->lastMove = CHESS_MOVE_NONE;
    g->hintMove = CHESS_MOVE_NONE;
    g->legalCount = 0;
    SyncVisuals(g);
    g->cam.topDown = false;
    g->cam.targetPitch = 0.62f;
    g->cam.targetDistance = 15.5f;
    SetState(g, CHESS_STATE_MENU);
}

static void FinishMatch(ChessGame *g, ChessStatus status)
{
    g->endReason = status;
    if (status == CHESS_CHECKMATE) {
        g->result = (g->board.side == CHESS_WHITE) ? CHESS_RESULT_BLACK_WINS : CHESS_RESULT_WHITE_WINS;
    } else {
        g->result = CHESS_RESULT_DRAW;
    }
    g->selected = -1;
    g->hintMove = CHESS_MOVE_NONE;
    g->overCardHidden = false;
    g->resultTime = 0.0f;
    SetState(g, CHESS_STATE_OVER);

    if (VsAI(g)) {
        int d = g->difficulty;
        bool humanWon = (g->result == CHESS_RESULT_WHITE_WINS && g->humanColor == CHESS_WHITE) ||
                        (g->result == CHESS_RESULT_BLACK_WINS && g->humanColor == CHESS_BLACK);
        if (g->result == CHESS_RESULT_DRAW) { g->draws[d]++; Sfx(g, CHESS_SFX_DRAW); }
        else if (humanWon) { g->wins[d]++; Sfx(g, CHESS_SFX_WIN); }
        else { g->losses[d]++; Sfx(g, CHESS_SFX_LOSE); }
        SaveStats(g);
    } else {
        Sfx(g, g->result == CHESS_RESULT_DRAW ? CHESS_SFX_DRAW : CHESS_SFX_WIN);
    }
}

static void PlayMove(ChessGame *g, ChessMove m)
{
    int flags = CHESS_MOVE_FLAGS(m);
    if (g->sanCount < CHESS_MAX_SAN) {
        ChessMoveToSAN(&g->board, m, g->san[g->sanCount], (int)sizeof(g->san[0]));
        g->sanCount++;
    }
    AnimateMove(g, m);
    ChessMakeMove(&g->board, m);
    g->lastMove = m;
    g->hintMove = CHESS_MOVE_NONE;
    g->selected = -1;
    g->moveListScroll = 0;
    RefreshLegal(g);

    if (CHESS_MOVE_PROMO(m)) Sfx(g, CHESS_SFX_PROMOTE);
    else if (flags & CHESS_FLAG_CASTLE) Sfx(g, CHESS_SFX_CASTLE);
    else if (flags & CHESS_FLAG_CAPTURE) Sfx(g, CHESS_SFX_CAPTURE);
    else Sfx(g, CHESS_SFX_MOVE);

    ChessStatus st = ChessGetStatus(&g->board);
    if (st != CHESS_ONGOING) {
        FinishMatch(g, st);
    } else if (ChessInCheck(&g->board, g->board.side)) {
        g->checkFlash = 1.0f;
        Sfx(g, CHESS_SFX_CHECK);
    }
}

static void UndoMove(ChessGame *g)
{
    if (g->board.ply == 0) {
        Sfx(g, CHESS_SFX_ERROR);
        return;
    }
    CancelThinking(g);
    ChessUnmakeMove(&g->board);
    if (g->sanCount > 0) g->sanCount--;
    if (VsAI(g)) {
        // Lùi tới lượt người chơi (máy đi trước thì có thể lùi về thế ban đầu)
        while (g->board.ply > 0 && g->board.side != g->humanColor) {
            ChessUnmakeMove(&g->board);
            if (g->sanCount > 0) g->sanCount--;
        }
    }
    g->lastMove = g->board.ply > 0 ? g->board.undo[g->board.ply - 1].move : CHESS_MOVE_NONE;
    g->selected = -1;
    g->hintMove = CHESS_MOVE_NONE;
    RefreshLegal(g);
    SyncVisuals(g);
    Sfx(g, CHESS_SFX_UNDO);
}

static void RequestHint(ChessGame *g)
{
    if (g->aiThinking || g->hintPending || ChessAIBusy()) return;
    g->hintPending = true;
    g->hintMove = CHESS_MOVE_NONE;
    ChessAIRequest(&g->board, CHESS_AI_HINT);
}

// ------------------------------------------------------------------ khởi tạo

void InitChessGame(ChessGame *g)
{
    *g = (ChessGame){0};
    LoadStats(g);
    g->mode = CHESS_MODE_VS_AI;
    g->difficulty = CHESS_AI_NORMAL;
    g->sideChoice = CHESS_SIDE_WHITE;
    g->highQuality = !PerfHintLite();   // Máy yếu (Pi 0-3) mở ở chất lượng Thấp
    g->soundEnabled = true;
    g->selected = -1;
    g->hoverSquare = -1;
    g->cursor = CHESS_SQ(4, 1);
    g->cam.yaw = 0.6f;
    g->cam.pitch = 0.62f;
    g->cam.distance = 15.5f;
    GoToMenu(g);
    g->cam.targetYaw = g->cam.yaw;
    ChessRenderSetQuality(g->highQuality);
    ChessRenderScene(g);          // Có ảnh ngay khung hình đầu dù Update chưa chạy
}

void CloseChessGame(ChessGame *g)
{
    CancelThinking(g);
}

// ------------------------------------------------------------------ menu

int ChessMenuOptionCount(ChessMenuOption row)
{
    switch (row) {
        case CHESS_OPT_MODE:       return CHESS_MODE_COUNT;
        case CHESS_OPT_DIFFICULTY: return 3;
        case CHESS_OPT_SIDE:       return CHESS_SIDE_COUNT;
        case CHESS_OPT_QUALITY:    return 2;
        default:                   return 0;
    }
}

static int GetOption(const ChessGame *g, ChessMenuOption row)
{
    switch (row) {
        case CHESS_OPT_MODE:       return g->mode;
        case CHESS_OPT_DIFFICULTY: return g->difficulty;
        case CHESS_OPT_SIDE:       return g->sideChoice;
        case CHESS_OPT_QUALITY:    return g->highQuality ? 0 : 1;
        default:                   return 0;
    }
}

static void SetOption(ChessGame *g, ChessMenuOption row, int v)
{
    switch (row) {
        case CHESS_OPT_MODE:       g->mode = (ChessMode)v; break;
        case CHESS_OPT_DIFFICULTY: g->difficulty = (ChessAILevel)v; break;
        case CHESS_OPT_SIDE:       g->sideChoice = (ChessSideChoice)v; break;
        case CHESS_OPT_QUALITY:
            g->highQuality = (v == 0);
            ChessRenderSetQuality(g->highQuality);
            break;
        default: break;
    }
}

static bool OptionEnabled(const ChessGame *g, ChessMenuOption row)
{
    if (row == CHESS_OPT_DIFFICULTY || row == CHESS_OPT_SIDE) return VsAI(g);
    return true;
}

static void MoveMenuRow(ChessGame *g, int dir)
{
    int rows = CHESS_MENU_START_ROW + 1;
    int r = g->menuRow;
    do {
        r = (r + dir + rows) % rows;
    } while (r < CHESS_MENU_START_ROW && !OptionEnabled(g, (ChessMenuOption)r));
    g->menuRow = r;
    Sfx(g, CHESS_SFX_SELECT);
}

static void UpdateMenu(ChessGame *g, float dt)
{
    g->cam.targetYaw += dt * CAM_MENU_SPIN;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) MoveMenuRow(g, -1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) MoveMenuRow(g, 1);
    if (g->menuRow < CHESS_MENU_START_ROW) {
        ChessMenuOption row = (ChessMenuOption)g->menuRow;
        int count = ChessMenuOptionCount(row);
        int dir = 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dir = -1;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dir = 1;
        if (dir != 0) {
            SetOption(g, row, (GetOption(g, row) + dir + count) % count);
            Sfx(g, CHESS_SFX_SELECT);
        }
    }
    for (int i = 0; i < 3; i++) {
        if ((IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) && VsAI(g)) {
            g->difficulty = (ChessAILevel)i;
            Sfx(g, CHESS_SFX_SELECT);
        }
    }
    for (int row = 0; row < CHESS_OPT_COUNT; row++) {
        if (!OptionEnabled(g, (ChessMenuOption)row)) continue;
        for (int i = 0; i < ChessMenuOptionCount((ChessMenuOption)row); i++) {
            if (ClickedIn(ChessMenuChipRect((ChessMenuOption)row, i))) {
                SetOption(g, (ChessMenuOption)row, i);
                g->menuRow = row;
                Sfx(g, CHESS_SFX_SELECT);
            }
        }
    }
    if (ConfirmPressed() || ClickedIn(ChessMenuStartRect())) {
        Sfx(g, CHESS_SFX_CONFIRM);
        StartMatch(g);
        return;
    }
    if (IsKeyPressed(KEY_M)) {
        g->soundEnabled = !g->soundEnabled;
        Sfx(g, CHESS_SFX_SELECT);
    }
    if (IsKeyPressed(KEY_ESCAPE)) g->quitRequested = true;
}

// ------------------------------------------------------------------ đang chơi

static bool IsOwnPiece(const ChessGame *g, int sq)
{
    unsigned char p = g->board.sq[sq];
    return p && CHESS_COLOR(p) == g->board.side;
}

static int FindMove(const ChessGame *g, int from, int to, int promo)
{
    for (int i = 0; i < g->legalCount; i++) {
        ChessMove m = g->legal[i];
        if (CHESS_MOVE_FROM(m) == from && CHESS_MOVE_TO(m) == to &&
            (promo < 0 || CHESS_MOVE_PROMO(m) == promo)) return i;
    }
    return -1;
}

// Chọn / đi quân tại một ô (dùng chung cho chuột và bàn phím).
static void ActivateSquare(ChessGame *g, int sq)
{
    if (sq < 0) return;
    if (g->selected >= 0 && sq != g->selected) {
        int idx = FindMove(g, g->selected, sq, -1);
        if (idx >= 0) {
            ChessMove m = g->legal[idx];
            if (CHESS_MOVE_PROMO(m)) {
                g->pendingPromotion = m;
                g->promotionChoice = 0;
                SetState(g, CHESS_STATE_PROMOTION);
                Sfx(g, CHESS_SFX_SELECT);
            } else {
                PlayMove(g, m);
            }
            return;
        }
    }
    if (IsOwnPiece(g, sq)) {
        g->selected = (g->selected == sq) ? -1 : sq;
        Sfx(g, CHESS_SFX_SELECT);
    } else {
        if (g->selected >= 0) Sfx(g, CHESS_SFX_ERROR);
        g->selected = -1;
    }
}

// Mũi tên di chuyển con trỏ theo hướng nhìn của camera, nên cầm đen hay
// xoay bàn thì "lên" vẫn là đi xa khỏi người chơi.
static void MoveCursor(ChessGame *g, int screenDx, int screenDy)
{
    float yaw = g->cam.yaw;
    Vector2 fwd = {-sinf(yaw), cosf(yaw)};
    Vector2 right = {-cosf(yaw), -sinf(yaw)};
    Vector2 dir = {right.x * screenDx + fwd.x * screenDy, right.y * screenDx + fwd.y * screenDy};
    int dfile = 0, drank = 0;
    if (fabsf(dir.x) > fabsf(dir.y)) dfile = dir.x > 0 ? -1 : 1;
    else drank = dir.y > 0 ? 1 : -1;
    int f = CHESS_FILE(g->cursor) + dfile, r = CHESS_RANK(g->cursor) + drank;
    if (f < 0) f = 0;
    if (f > 7) f = 7;
    if (r < 0) r = 0;
    if (r > 7) r = 7;
    g->cursor = CHESS_SQ(f, r);
    g->keyboardCursor = true;
}

static void OpenPause(ChessGame *g)
{
    g->pauseRow = 0;
    SetState(g, CHESS_STATE_PAUSED);
    Sfx(g, CHESS_SFX_SELECT);
}

static void UpdateAI(ChessGame *g, float dt)
{
    if (g->hintPending) {
        ChessMove m;
        if (ChessAIPoll(&m)) {
            g->hintPending = false;
            if (g->state == CHESS_STATE_PLAYING && HumanTurn(g) && m != CHESS_MOVE_NONE) {
                g->hintMove = m;
                g->hintTimer = 0.0f;
                g->cursor = CHESS_MOVE_FROM(m);
                Sfx(g, CHESS_SFX_HINT);
            }
        }
        return;
    }
    if (!VsAI(g) || HumanTurn(g) || g->state != CHESS_STATE_PLAYING) return;

    if (!g->aiThinking) {
        g->aiThinking = true;
        g->aiTimer = 0.0f;
        ChessAIRequest(&g->board, g->difficulty);
        return;
    }
    g->aiTimer += dt;
    if (g->aiTimer < MIN_THINK_TIME[g->difficulty] || AnyAnimating(g)) return;

    ChessMove m;
    if (ChessAIPoll(&m)) {
        g->aiThinking = false;
        if (m != CHESS_MOVE_NONE && FindMove(g, CHESS_MOVE_FROM(m), CHESS_MOVE_TO(m), CHESS_MOVE_PROMO(m)) >= 0) {
            PlayMove(g, m);
        }
    }
}

static void UpdatePlaying(ChessGame *g, float dt)
{
    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
    if (delta.x != 0.0f || delta.y != 0.0f) g->keyboardCursor = false;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || ClickedIn(ChessPanelButtonRect(CHESS_BTN_PAUSE))) {
        if (IsKeyPressed(KEY_ESCAPE) && g->selected >= 0) {
            g->selected = -1;
            return;
        }
        OpenPause(g);
        return;
    }
    if (IsKeyPressed(KEY_U) || IsKeyPressed(KEY_BACKSPACE) || ClickedIn(ChessPanelButtonRect(CHESS_BTN_UNDO))) {
        UndoMove(g);
        return;
    }
    if (IsKeyPressed(KEY_H) || ClickedIn(ChessPanelButtonRect(CHESS_BTN_HINT))) {
        if (HumanTurn(g) && !g->aiThinking) RequestHint(g);
        else Sfx(g, CHESS_SFX_ERROR);
    }
    if (IsKeyPressed(KEY_F) || ClickedIn(ChessPanelButtonRect(CHESS_BTN_FLIP))) FlipView(g);
    if (IsKeyPressed(KEY_V) || ClickedIn(ChessPanelButtonRect(CHESS_BTN_VIEW))) ToggleTopDown(g);
    if (IsKeyPressed(KEY_C)) {
        ResetCamera(g, false);
        Sfx(g, CHESS_SFX_SELECT);
    }
    if (IsKeyPressed(KEY_N) || ClickedIn(ChessPanelButtonRect(CHESS_BTN_NEW))) {
        Sfx(g, CHESS_SFX_CONFIRM);
        StartMatch(g);
        return;
    }
    if (IsKeyPressed(KEY_M)) {
        g->soundEnabled = !g->soundEnabled;
        Sfx(g, CHESS_SFX_SELECT);
    }

    // Cuộn danh sách nước khi chuột ở trên bảng
    if (ChessPointInUi(g, mouse)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) g->moveListScroll += (wheel > 0.0f) ? 1 : -1;
        if (g->moveListScroll < 0) g->moveListScroll = 0;
    }

    g->hoverSquare = ChessPointInUi(g, mouse) ? -1 : ChessPickSquare(g, mouse);

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT) || IsKeyPressed(KEY_A)) MoveCursor(g, -1, 0);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT) || IsKeyPressed(KEY_D)) MoveCursor(g, 1, 0);
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) || IsKeyPressed(KEY_W)) MoveCursor(g, 0, 1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN) || IsKeyPressed(KEY_S)) MoveCursor(g, 0, -1);

    bool canAct = HumanTurn(g) && !g->aiThinking && !AnyAnimating(g);
    if (canAct) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g->hoverSquare >= 0) {
            g->cursor = g->hoverSquare;
            ActivateSquare(g, g->hoverSquare);
        } else if (ConfirmPressed()) {
            g->keyboardCursor = true;
            ActivateSquare(g, g->cursor);
        }
    }
    if (g->state == CHESS_STATE_PLAYING) UpdateAI(g, dt);
}

static void UpdatePromotion(ChessGame *g)
{
    int choice = -1;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) g->promotionChoice = (g->promotionChoice + 3) % 4;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) g->promotionChoice = (g->promotionChoice + 1) % 4;
    for (int i = 0; i < 4; i++) {
        if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) choice = i;
        if (ClickedIn(ChessPromotionRect(i))) choice = i;
    }
    if (IsKeyPressed(KEY_Q)) choice = 0;
    if (IsKeyPressed(KEY_R)) choice = 1;
    if (IsKeyPressed(KEY_B)) choice = 2;
    if (IsKeyPressed(KEY_N)) choice = 3;
    if (ConfirmPressed()) choice = g->promotionChoice;

    if (IsKeyPressed(KEY_ESCAPE)) {
        g->pendingPromotion = CHESS_MOVE_NONE;
        SetState(g, CHESS_STATE_PLAYING);
        return;
    }
    if (choice >= 0) {
        ChessMove m = g->pendingPromotion;
        int idx = FindMove(g, CHESS_MOVE_FROM(m), CHESS_MOVE_TO(m), CHESS_PROMOTION_TYPES[choice]);
        g->pendingPromotion = CHESS_MOVE_NONE;
        SetState(g, CHESS_STATE_PLAYING);
        if (idx >= 0) PlayMove(g, g->legal[idx]);
    }
}

static void UpdatePaused(ChessGame *g)
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        g->pauseRow = (g->pauseRow + CHESS_PAUSE_ROWS - 1) % CHESS_PAUSE_ROWS;
        Sfx(g, CHESS_SFX_SELECT);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        g->pauseRow = (g->pauseRow + 1) % CHESS_PAUSE_ROWS;
        Sfx(g, CHESS_SFX_SELECT);
    }
    int chosen = -1;
    for (int i = 0; i < CHESS_PAUSE_ROWS; i++) {
        if (ClickedIn(ChessPauseRowRect(i))) chosen = i;
    }
    if (ConfirmPressed()) chosen = g->pauseRow;
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) chosen = 0;

    switch (chosen) {
        case 0: SetState(g, CHESS_STATE_PLAYING); Sfx(g, CHESS_SFX_SELECT); break;
        case 1: Sfx(g, CHESS_SFX_CONFIRM); StartMatch(g); break;
        case 2: Sfx(g, CHESS_SFX_SELECT); GoToMenu(g); break;
        case 3: g->soundEnabled = !g->soundEnabled; Sfx(g, CHESS_SFX_SELECT); break;
        default: break;
    }
}

static void UpdateOver(ChessGame *g, float dt)
{
    g->resultTime += dt;
    if (g->stateTime < RESULT_INPUT_DELAY) return;

    if (IsKeyPressed(KEY_F)) FlipView(g);
    if (IsKeyPressed(KEY_V)) ToggleTopDown(g);

    if (g->overCardHidden) {
        // Đang xem lại bàn cờ: bấm gì cũng mở lại thẻ kết quả
        if (ConfirmPressed() || IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g->overCardHidden = false;
            Sfx(g, CHESS_SFX_SELECT);
        }
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_N) || ClickedIn(ChessOverButtonRect(0))) {
        Sfx(g, CHESS_SFX_CONFIRM);
        StartMatch(g);
    } else if (IsKeyPressed(KEY_SPACE) || ClickedIn(ChessOverButtonRect(1))) {
        g->overCardHidden = true;
        Sfx(g, CHESS_SFX_SELECT);
    } else if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q) || ClickedIn(ChessOverButtonRect(2))) {
        Sfx(g, CHESS_SFX_SELECT);
        GoToMenu(g);
    } else if (IsKeyPressed(KEY_M)) {
        g->soundEnabled = !g->soundEnabled;
        Sfx(g, CHESS_SFX_SELECT);
    }
}

void UpdateChessGame(ChessGame *g, float dt)
{
    g->globalTime += dt;
    g->stateTime += dt;
    if (g->hintMove != CHESS_MOVE_NONE) g->hintTimer += dt;
    if (g->checkFlash > 0.0f) g->checkFlash -= dt;

    bool cameraInput = g->state == CHESS_STATE_PLAYING || g->state == CHESS_STATE_OVER || g->state == CHESS_STATE_MENU;
    UpdateCameraRig(g, dt, cameraInput);
    UpdateVisuals(g, dt);

    switch (g->state) {
        case CHESS_STATE_MENU:      UpdateMenu(g, dt); break;
        case CHESS_STATE_PLAYING:   UpdatePlaying(g, dt); break;
        case CHESS_STATE_PROMOTION: UpdatePromotion(g); break;
        case CHESS_STATE_PAUSED:    UpdatePaused(g); break;
        case CHESS_STATE_OVER:      UpdateOver(g, dt); break;
    }

    ChessRenderScene(g);
}

void DrawChessGame(const ChessGame *g)
{
    ChessRenderBlit(g);
    DrawChessHud(g);
}
