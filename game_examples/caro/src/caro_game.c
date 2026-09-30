#include "caro_game.h"
#include "caro_rules.h"
#include "caro_ai.h"
#include "caro_draw.h"
#include "caro_assets.h"
#include "save_data.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Khoá lưu dữ liệu (common/save_data) - trùng id của game trong Arcade Hub.
#define CARO_SAVE_ID "caro"

// Máy luôn "suy nghĩ" ít nhất chừng này giây để nước đi không hiện ra tức thì.
static const float MIN_THINK_TIME[CARO_DIFF_COUNT] = {0.45f, 0.40f, 0.30f};
#define RESULT_INPUT_DELAY 0.5f
#define PLACE_ANIM_SPEED   4.0f

static const char *DIFF_KEYS[CARO_DIFF_COUNT] = {"easy", "normal", "hard"};

static const Color X_COLOR = {236, 84, 84, 255};
static const Color O_COLOR = {64, 142, 236, 255};

// ------------------------------------------------------------------ lưu trữ

static void LoadStats(CaroGame *game)
{
    for (int i = 0; i < CARO_DIFF_COUNT; i++) {
        char key[32];
        snprintf(key, sizeof(key), "win.%s", DIFF_KEYS[i]);
        game->wins[i] = SaveDataGetInt(CARO_SAVE_ID, key, 0);
        snprintf(key, sizeof(key), "loss.%s", DIFF_KEYS[i]);
        game->losses[i] = SaveDataGetInt(CARO_SAVE_ID, key, 0);
        snprintf(key, sizeof(key), "draw.%s", DIFF_KEYS[i]);
        game->draws[i] = SaveDataGetInt(CARO_SAVE_ID, key, 0);
    }
}

static void SaveStats(const CaroGame *game)
{
    int totalWins = 0;
    for (int i = 0; i < CARO_DIFF_COUNT; i++) {
        char key[32];
        snprintf(key, sizeof(key), "win.%s", DIFF_KEYS[i]);
        SaveDataSetInt(CARO_SAVE_ID, key, game->wins[i]);
        snprintf(key, sizeof(key), "loss.%s", DIFF_KEYS[i]);
        SaveDataSetInt(CARO_SAVE_ID, key, game->losses[i]);
        snprintf(key, sizeof(key), "draw.%s", DIFF_KEYS[i]);
        SaveDataSetInt(CARO_SAVE_ID, key, game->draws[i]);
        totalWins += game->wins[i];
    }
    // "best" là con số Hub hiển thị: tổng số ván thắng máy.
    SaveDataSetInt(CARO_SAVE_ID, "best", totalWins);
}

void MigrateCaroSave(void)
{
}

// ------------------------------------------------------------------ tiện ích

static void SetState(CaroGame *game, CaroState state)
{
    game->state = state;
    game->stateTime = 0.0f;
}

static bool ClickedIn(Rectangle r)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r);
}

static bool ConfirmPressed(void)
{
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
}

static void Sfx(const CaroGame *game, CaroSfx sfx)
{
    PlayCaroSfx(sfx, game->soundEnabled, (sfx == CARO_SFX_PLACE_X || sfx == CARO_SFX_PLACE_O) ? 0.06f : 0.0f);
}

static bool VsAI(const CaroGame *game)
{
    return game->mode == CARO_MODE_VS_AI;
}

static bool HumanTurn(const CaroGame *game)
{
    return !VsAI(game) || game->board.toMove == game->humanStone;
}

static Color StoneColor(CaroStone s)
{
    return s == CARO_X ? X_COLOR : O_COLOR;
}

// ------------------------------------------------------------------ hiệu ứng

static void SpawnParticle(CaroGame *game, Vector2 pos, Vector2 vel, Color color, float size, float life)
{
    for (int i = 0; i < CARO_MAX_PARTICLES; i++) {
        CaroParticle *p = &game->particles[i];
        if (p->active) continue;
        *p = (CaroParticle){
            .pos = pos, .vel = vel, .color = color, .size = size,
            .rot = (float)GetRandomValue(0, 360), .spin = (float)GetRandomValue(-360, 360),
            .life = life, .maxLife = life, .active = true
        };
        return;
    }
}

static void EmitPlaceBurst(CaroGame *game, int cell, CaroStone s)
{
    Rectangle r = CaroCellRect(game->board.n, cell);
    Vector2 c = {r.x + r.width * 0.5f, r.y + r.height * 0.5f};
    Color col = StoneColor(s);
    for (int i = 0; i < 10; i++) {
        float a = (float)GetRandomValue(0, 628) / 100.0f;
        float sp = (float)GetRandomValue(60, 160);
        SpawnParticle(game, c, (Vector2){cosf(a) * sp, sinf(a) * sp}, col, (float)GetRandomValue(2, 4), 0.45f);
    }
}

static void EmitConfetti(CaroGame *game)
{
    static const Color palette[] = {
        {255, 206, 92, 255}, {236, 84, 84, 255}, {64, 142, 236, 255},
        {120, 220, 150, 255}, {250, 250, 250, 255}, {196, 132, 255, 255}
    };
    Rectangle area = CaroBoardArea(game->board.n);
    for (int i = 0; i < 160; i++) {
        Vector2 pos = {area.x + (float)GetRandomValue(0, (int)area.width), area.y - (float)GetRandomValue(10, 220)};
        Vector2 vel = {(float)GetRandomValue(-60, 60), (float)GetRandomValue(60, 200)};
        SpawnParticle(game, pos, vel, palette[GetRandomValue(0, 5)], (float)GetRandomValue(5, 9), 3.2f);
    }
}

static void UpdateParticles(CaroGame *game, float dt)
{
    for (int i = 0; i < CARO_MAX_PARTICLES; i++) {
        CaroParticle *p = &game->particles[i];
        if (!p->active) continue;
        p->life -= dt;
        if (p->life <= 0.0f) { p->active = false; continue; }
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        if (p->size > 4.5f) {
            // Giấy màu: rơi chậm, lắc lư
            p->vel.y += 60.0f * dt;
            p->vel.x += sinf(game->globalTime * 3.0f + (float)i) * 40.0f * dt;
        } else {
            p->vel.x *= 1.0f - 4.0f * dt;
            p->vel.y *= 1.0f - 4.0f * dt;
        }
        p->rot += p->spin * dt;
    }
}

static void ClearParticles(CaroGame *game)
{
    for (int i = 0; i < CARO_MAX_PARTICLES; i++) game->particles[i].active = false;
}

// ------------------------------------------------------------------ vòng đời ván

static void CancelThinking(CaroGame *game)
{
    CaroAICancel();
    game->aiThinking = false;
    game->hintPending = false;
    game->aiTimer = 0.0f;
}

static void StartMatch(CaroGame *game)
{
    CancelThinking(game);
    int n = (game->sizeChoice == 0) ? 15 : 19;
    CaroBoardInit(&game->board, n, game->rule);
    game->humanStone = (VsAI(game) && !game->humanFirst) ? CARO_O : CARO_X;
    game->result = CARO_RESULT_NONE;
    game->winAnim = 0.0f;
    game->resultTime = 0.0f;
    game->cursor = CaroIndex(&game->board, n / 2, n / 2);
    game->hoverCell = -1;
    game->hintCell = -1;
    game->hintTimer = 0.0f;
    game->gameTime = 0.0f;
    game->shake = 0.0f;
    memset(game->placeAnim, 0, sizeof(game->placeAnim));
    ClearParticles(game);
    SetState(game, CARO_STATE_PLAYING);
}

static void GoToMenu(CaroGame *game)
{
    CancelThinking(game);
    ClearParticles(game);
    SetState(game, CARO_STATE_MENU);
}

static void FinishMatch(CaroGame *game, CaroResult result)
{
    game->result = result;
    game->resultTime = 0.0f;
    game->hintCell = -1;
    SetState(game, CARO_STATE_OVER);

    if (result == CARO_RESULT_X_WINS) game->sessionX++;
    else if (result == CARO_RESULT_O_WINS) game->sessionO++;
    else game->sessionDraw++;

    if (VsAI(game)) {
        bool humanWon = (result == CARO_RESULT_X_WINS && game->humanStone == CARO_X) ||
                        (result == CARO_RESULT_O_WINS && game->humanStone == CARO_O);
        if (result == CARO_RESULT_DRAW) game->draws[game->difficulty]++;
        else if (humanWon) game->wins[game->difficulty]++;
        else game->losses[game->difficulty]++;
        SaveStats(game);

        if (result == CARO_RESULT_DRAW) Sfx(game, CARO_SFX_CONFIRM);
        else Sfx(game, humanWon ? CARO_SFX_WIN : CARO_SFX_LOSE);
        if (humanWon) EmitConfetti(game);
    } else {
        Sfx(game, result == CARO_RESULT_DRAW ? CARO_SFX_CONFIRM : CARO_SFX_WIN);
        if (result != CARO_RESULT_DRAW) EmitConfetti(game);
    }
    if (result != CARO_RESULT_DRAW) game->shake = 0.35f;
}

static void PlaceStone(CaroGame *game, int cell)
{
    CaroStone s = game->board.toMove;
    if (!CaroPlace(&game->board, cell)) {
        Sfx(game, CARO_SFX_ERROR);
        return;
    }
    game->placeAnim[cell] = 0.0f;
    game->hintCell = -1;
    Sfx(game, s == CARO_X ? CARO_SFX_PLACE_X : CARO_SFX_PLACE_O);
    EmitPlaceBurst(game, cell, s);

    int a, b;
    if (CaroIsWinningMove(&game->board, cell, &a, &b)) {
        game->winCells[0] = a;
        game->winCells[1] = b;
        FinishMatch(game, s == CARO_X ? CARO_RESULT_X_WINS : CARO_RESULT_O_WINS);
    } else if (CaroBoardFull(&game->board)) {
        FinishMatch(game, CARO_RESULT_DRAW);
    }
}

static void UndoMove(CaroGame *game)
{
    CaroBoard *b = &game->board;
    if (b->moveCount == 0) {
        Sfx(game, CARO_SFX_ERROR);
        return;
    }
    CancelThinking(game);
    if (VsAI(game)) {
        // Lùi tới khi lại tới lượt người chơi và đã gỡ ít nhất một nước của họ.
        CaroUndo(b);
        // Nếu máy đi trước và đã lùi về bàn trống, máy sẽ tự đi lại nước đầu.
        while (b->moveCount > 0 && b->toMove != game->humanStone) CaroUndo(b);
    } else {
        CaroUndo(b);
    }
    game->hintCell = -1;
    Sfx(game, CARO_SFX_UNDO);
}

static void RequestHint(CaroGame *game)
{
    if (game->aiThinking || game->hintPending || CaroAIBusy()) return;
    game->hintPending = true;
    game->hintCell = -1;
    CaroAIRequest(&game->board, CARO_DIFF_NORMAL);
}

// ------------------------------------------------------------------ khởi tạo

void InitCaroGame(CaroGame *game)
{
    *game = (CaroGame){0};
    LoadStats(game);
    game->mode = CARO_MODE_VS_AI;
    game->difficulty = CARO_DIFF_NORMAL;
    game->sizeChoice = 0;
    game->rule = CARO_RULE_FREE;
    game->humanFirst = true;
    game->soundEnabled = true;
    game->hoverCell = -1;
    game->hintCell = -1;
    CaroBoardInit(&game->board, 15, CARO_RULE_FREE);
    SetState(game, CARO_STATE_MENU);
}

void CloseCaroGame(CaroGame *game)
{
    CancelThinking(game);
    CaroDrawRelease();
}

// ------------------------------------------------------------------ menu

int CaroMenuOptionCount(CaroMenuOption row)
{
    switch (row) {
        case CARO_OPT_MODE:       return CARO_MODE_COUNT;
        case CARO_OPT_DIFFICULTY: return CARO_DIFF_COUNT;
        case CARO_OPT_SIZE:       return 2;
        case CARO_OPT_RULE:       return CARO_RULE_COUNT;
        case CARO_OPT_FIRST:      return 2;
        default:                  return 0;
    }
}

static int GetOption(const CaroGame *game, CaroMenuOption row)
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

static void SetOption(CaroGame *game, CaroMenuOption row, int value)
{
    switch (row) {
        case CARO_OPT_MODE:       game->mode = (CaroMode)value; break;
        case CARO_OPT_DIFFICULTY: game->difficulty = (CaroDifficulty)value; break;
        case CARO_OPT_SIZE:       game->sizeChoice = value; break;
        case CARO_OPT_RULE:       game->rule = (CaroRule)value; break;
        case CARO_OPT_FIRST:      game->humanFirst = (value == 0); break;
        default: break;
    }
}

// Độ khó và lượt đi trước chỉ có nghĩa khi đấu với máy.
static bool OptionEnabled(const CaroGame *game, CaroMenuOption row)
{
    if (row == CARO_OPT_DIFFICULTY || row == CARO_OPT_FIRST) return VsAI(game);
    return true;
}

static void MoveMenuRow(CaroGame *game, int dir)
{
    int rows = CARO_MENU_START_ROW + 1;
    int r = game->menuRow;
    do {
        r = (r + dir + rows) % rows;
    } while (r < CARO_MENU_START_ROW && !OptionEnabled(game, (CaroMenuOption)r));
    game->menuRow = r;
    Sfx(game, CARO_SFX_SELECT);
}

static void UpdateMenu(CaroGame *game)
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) MoveMenuRow(game, -1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) MoveMenuRow(game, 1);

    if (game->menuRow < CARO_MENU_START_ROW) {
        CaroMenuOption row = (CaroMenuOption)game->menuRow;
        int count = CaroMenuOptionCount(row);
        int dir = 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dir = -1;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dir = 1;
        if (dir != 0) {
            SetOption(game, row, (GetOption(game, row) + dir + count) % count);
            Sfx(game, CARO_SFX_SELECT);
        }
    }
    for (int i = 0; i < CARO_DIFF_COUNT; i++) {
        if ((IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) && VsAI(game)) {
            game->difficulty = (CaroDifficulty)i;
            Sfx(game, CARO_SFX_SELECT);
        }
    }

    // Chuột: bấm chip nào thì chọn chip đó
    for (int row = 0; row < CARO_OPT_COUNT; row++) {
        if (!OptionEnabled(game, (CaroMenuOption)row)) continue;
        for (int i = 0; i < CaroMenuOptionCount((CaroMenuOption)row); i++) {
            if (ClickedIn(CaroMenuChipRect((CaroMenuOption)row, i))) {
                SetOption(game, (CaroMenuOption)row, i);
                game->menuRow = row;
                Sfx(game, CARO_SFX_SELECT);
            }
        }
    }

    bool start = ClickedIn(CaroMenuStartRect());
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) start = true;
    if (start) {
        Sfx(game, CARO_SFX_CONFIRM);
        StartMatch(game);
        return;
    }

    if (IsKeyPressed(KEY_M)) {
        game->soundEnabled = !game->soundEnabled;
        Sfx(game, CARO_SFX_SELECT);
    }
    if (IsKeyPressed(KEY_ESCAPE)) game->quitRequested = true;
}

// ------------------------------------------------------------------ đang chơi

static void MoveCursor(CaroGame *game, int dx, int dy)
{
    int n = game->board.n;
    int x = game->cursor % n + dx, y = game->cursor / n + dy;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= n) x = n - 1;
    if (y >= n) y = n - 1;
    game->cursor = y * n + x;
    game->keyboardCursor = true;
}

static void HandleBoardInput(CaroGame *game)
{
    int n = game->board.n;
    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
    if (delta.x != 0.0f || delta.y != 0.0f) game->keyboardCursor = false;
    game->hoverCell = CaroCellAtPoint(n, mouse);

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressedRepeat(KEY_LEFT)) MoveCursor(game, -1, 0);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsKeyPressedRepeat(KEY_RIGHT)) MoveCursor(game, 1, 0);
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressedRepeat(KEY_UP)) MoveCursor(game, 0, -1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressedRepeat(KEY_DOWN)) MoveCursor(game, 0, 1);

    if (!HumanTurn(game) || game->aiThinking) return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && game->hoverCell >= 0) {
        game->cursor = game->hoverCell;
        if (game->board.cells[game->hoverCell] == CARO_EMPTY) PlaceStone(game, game->hoverCell);
        else Sfx(game, CARO_SFX_ERROR);
    } else if (ConfirmPressed()) {
        if (game->board.cells[game->cursor] == CARO_EMPTY) PlaceStone(game, game->cursor);
        else Sfx(game, CARO_SFX_ERROR);
        game->keyboardCursor = true;
    }
}

static void UpdateAI(CaroGame *game, float dt)
{
    if (game->hintPending) {
        int cell;
        if (CaroAIPoll(&cell)) {
            game->hintPending = false;
            if (game->state == CARO_STATE_PLAYING && HumanTurn(game) && cell >= 0) {
                game->hintCell = cell;
                game->hintTimer = 0.0f;
                game->cursor = cell;
                Sfx(game, CARO_SFX_HINT);
            }
        }
        return;
    }

    if (!VsAI(game) || HumanTurn(game) || game->state != CARO_STATE_PLAYING) return;

    if (!game->aiThinking) {
        game->aiThinking = true;
        game->aiTimer = 0.0f;
        CaroAIRequest(&game->board, game->difficulty);
        return;
    }
    game->aiTimer += dt;
    if (game->aiTimer < MIN_THINK_TIME[game->difficulty]) return;

    int cell;
    if (CaroAIPoll(&cell)) {
        game->aiThinking = false;
        if (cell >= 0) PlaceStone(game, cell);
    }
}

static void OpenPause(CaroGame *game)
{
    game->pauseRow = 0;
    SetState(game, CARO_STATE_PAUSED);
    Sfx(game, CARO_SFX_SELECT);
}

static void UpdatePlaying(CaroGame *game, float dt)
{
    game->gameTime += dt;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || ClickedIn(CaroPanelButtonRect(CARO_BTN_MENU))) {
        OpenPause(game);
        return;
    }
    if (IsKeyPressed(KEY_U) || IsKeyPressed(KEY_BACKSPACE) || ClickedIn(CaroPanelButtonRect(CARO_BTN_UNDO))) {
        UndoMove(game);
        return;
    }
    if (IsKeyPressed(KEY_H) || ClickedIn(CaroPanelButtonRect(CARO_BTN_HINT))) {
        if (HumanTurn(game) && !game->aiThinking) RequestHint(game);
        else Sfx(game, CARO_SFX_ERROR);
    }
    if (IsKeyPressed(KEY_N) || ClickedIn(CaroPanelButtonRect(CARO_BTN_NEW))) {
        Sfx(game, CARO_SFX_CONFIRM);
        StartMatch(game);
        return;
    }
    if (IsKeyPressed(KEY_M)) {
        game->soundEnabled = !game->soundEnabled;
        Sfx(game, CARO_SFX_SELECT);
    }

    HandleBoardInput(game);
    if (game->state == CARO_STATE_PLAYING) UpdateAI(game, dt);
}

static void UpdatePaused(CaroGame *game)
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        game->pauseRow = (game->pauseRow + CARO_PAUSE_ROWS - 1) % CARO_PAUSE_ROWS;
        Sfx(game, CARO_SFX_SELECT);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        game->pauseRow = (game->pauseRow + 1) % CARO_PAUSE_ROWS;
        Sfx(game, CARO_SFX_SELECT);
    }
    int chosen = -1;
    for (int i = 0; i < CARO_PAUSE_ROWS; i++) {
        if (ClickedIn(CaroPauseRowRect(i))) chosen = i;
    }
    if (ConfirmPressed()) chosen = game->pauseRow;
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) chosen = 0;

    switch (chosen) {
        case 0:
            SetState(game, CARO_STATE_PLAYING);
            Sfx(game, CARO_SFX_SELECT);
            break;
        case 1:
            Sfx(game, CARO_SFX_CONFIRM);
            StartMatch(game);
            break;
        case 2:
            Sfx(game, CARO_SFX_SELECT);
            GoToMenu(game);
            break;
        case 3:
            game->soundEnabled = !game->soundEnabled;
            Sfx(game, CARO_SFX_SELECT);
            break;
        default:
            break;
    }
}

static void UpdateOver(CaroGame *game, float dt)
{
    game->resultTime += dt;
    game->winAnim += dt;
    if (game->stateTime < RESULT_INPUT_DELAY) return;

    if (ConfirmPressed() || IsKeyPressed(KEY_N) || IsKeyPressed(KEY_R) ||
        ClickedIn(CaroPanelButtonRect(CARO_BTN_NEW))) {
        Sfx(game, CARO_SFX_CONFIRM);
        StartMatch(game);
    } else if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q) || ClickedIn(CaroPanelButtonRect(CARO_BTN_MENU))) {
        Sfx(game, CARO_SFX_SELECT);
        GoToMenu(game);
    } else if (IsKeyPressed(KEY_M)) {
        game->soundEnabled = !game->soundEnabled;
        Sfx(game, CARO_SFX_SELECT);
    }
}

void UpdateCaroGame(CaroGame *game, float dt)
{
    game->globalTime += dt;
    game->stateTime += dt;
    if (game->shake > 0.0f) game->shake -= dt;
    if (game->hintCell >= 0) game->hintTimer += dt;

    int cells = game->board.n * game->board.n;
    for (int i = 0; i < cells; i++) {
        if (game->board.cells[i] != CARO_EMPTY && game->placeAnim[i] < 1.0f) {
            game->placeAnim[i] += dt * PLACE_ANIM_SPEED;
            if (game->placeAnim[i] > 1.0f) game->placeAnim[i] = 1.0f;
        }
    }
    UpdateParticles(game, dt);

    switch (game->state) {
        case CARO_STATE_MENU:    UpdateMenu(game); break;
        case CARO_STATE_PLAYING: UpdatePlaying(game, dt); break;
        case CARO_STATE_PAUSED:  UpdatePaused(game); break;
        case CARO_STATE_OVER:    UpdateOver(game, dt); break;
    }

    CaroDrawPrepare(game);
}
