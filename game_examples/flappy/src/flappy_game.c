#include "flappy_game.h"
#include "flappy_world.h"
#include "flappy_assets.h"
#include "flappy_audio.h"
#include "flappy_draw.h"
#include "save_data.h"
#include <stdio.h>

// Khoá lưu dữ liệu (common/save_data) - phải trùng id của game trong Arcade Hub.
#define FLAPPY_SAVE_ID "flappy"

// Mỗi độ khó một kỷ lục riêng; "best" là kỷ lục tổng mà Hub hiển thị.
static const char *BEST_KEYS[FLAPPY_DIFF_COUNT] = {"best.easy", "best.normal", "best.hard"};

#define GAME_OVER_INPUT_DELAY 0.6f   // Chống bấm nhầm "chơi lại" ngay khi vừa rơi

static void SaveBestScores(const FlappyGame *game)
{
    for (int i = 0; i < FLAPPY_DIFF_COUNT; i++) {
        SaveDataSubmitBest(FLAPPY_SAVE_ID, BEST_KEYS[i], game->best[i]);
        SaveDataSubmitBest(FLAPPY_SAVE_ID, "best", game->best[i]);
    }
}

// Bản cũ ghi 3 số int nhị phân (Dễ/Thường/Khó) vào thư mục đang chạy game.
static void ImportLegacyScores(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return;
    int values[FLAPPY_DIFF_COUNT] = {0};
    if (fread(values, sizeof(int), FLAPPY_DIFF_COUNT, f) == FLAPPY_DIFF_COUNT) {
        for (int i = 0; i < FLAPPY_DIFF_COUNT; i++) {
            SaveDataSubmitBest(FLAPPY_SAVE_ID, BEST_KEYS[i], values[i]);
            SaveDataSubmitBest(FLAPPY_SAVE_ID, "best", values[i]);
        }
    }
    fclose(f);
}

void MigrateFlappySave(void)
{
    SaveDataMigrateLegacy("flappy_highscore.dat", ImportLegacyScores);
}

static void LoadBestScores(FlappyGame *game)
{
    MigrateFlappySave();
    for (int i = 0; i < FLAPPY_DIFF_COUNT; i++) {
        int v = SaveDataGetInt(FLAPPY_SAVE_ID, BEST_KEYS[i], 0);
        game->best[i] = (v >= 0) ? v : 0;
    }
}

static void SetState(FlappyGame *game, FlappyState state)
{
    game->state = state;
    game->stateTime = 0.0f;
}

// Bắt đầu một lượt mới: dựng lại địa hình, đưa máy bay về vạch xuất phát.
static void StartRun(FlappyGame *game)
{
    ResetWorld(game);
    ResetPlane(&game->plane);
    ClearEffects(game);
    game->score = 0;
    game->starsCollected = 0;
    game->rocksPassed = 0;
    game->newBest = false;
    game->scorePop = 0.0f;
    game->shownScore = 0;
    game->shakeTimer = 0.0f;
    game->flashTimer = 0.0f;
    game->gameOverAnim = 0.0f;
    SetState(game, FLAPPY_STATE_READY);
}

static void GoToMenu(FlappyGame *game)
{
    ResetWorld(game);
    ResetPlane(&game->plane);
    ClearEffects(game);
    SetState(game, FLAPPY_STATE_MENU);
}

static void Shake(FlappyGame *game, float magnitude, float duration)
{
    game->shakeMagnitude = magnitude;
    game->shakeTimer = duration;
}

static void EnterGameOver(FlappyGame *game)
{
    int *best = &game->best[game->difficulty];
    if (game->score > *best) {
        *best = game->score;
        game->newBest = true;
        SaveBestScores(game);
    }
    game->shownScore = 0;
    game->countTimer = 0.0f;
    game->gameOverAnim = 0.0f;
    SetState(game, FLAPPY_STATE_GAME_OVER);
    PlayFlappySfx(game->newBest ? FLAPPY_SFX_NEW_BEST : FLAPPY_SFX_GAME_OVER, game->soundEnabled, 0.0f);
}

static void CrashIntoGround(FlappyGame *game)
{
    game->plane.alive = false;
    game->plane.pos.y = GroundTopUnder(game, game->plane.pos.x) - PLANE_RADIUS + 3.0f;
    EmitCrash(game, game->plane.pos);
    Shake(game, 9.0f, 0.4f);
    game->flashTimer = 0.25f;
    PlayFlappySfx(FLAPPY_SFX_HIT, game->soundEnabled, 0.05f);
    EnterGameOver(game);
}

static void CrashIntoRock(FlappyGame *game)
{
    Plane *p = &game->plane;
    p->alive = false;
    p->vy = (p->vy < -120.0f) ? p->vy : -120.0f;   // Bật nhẹ lên rồi mới rơi
    p->spinSpeed = 460.0f;
    p->trailTimer = 0.0f;
    EmitCrash(game, p->pos);
    Shake(game, 10.0f, 0.35f);
    game->flashTimer = 0.25f;
    PlayFlappySfx(FLAPPY_SFX_HIT, game->soundEnabled, 0.05f);
    SetState(game, FLAPPY_STATE_DYING);
}

static bool FlapPressed(void)
{
    return IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool ClickedIn(Rectangle r)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r);
}

static void ToggleSound(FlappyGame *game)
{
    game->soundEnabled = !game->soundEnabled;
    PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
}

void InitFlappyGame(FlappyGame *game)
{
    *game = (FlappyGame){0};
    LoadBestScores(game);
    game->difficulty = FLAPPY_DIFF_NORMAL;
    game->planeSkin = 0;
    game->soundEnabled = true;

    Color sky = FlappyBiome(0)->sky;
    game->skyTint = (Vector3){(float)sky.r, (float)sky.g, (float)sky.b};

    GoToMenu(game);
}

void CloseFlappyGame(FlappyGame *game)
{
    SaveBestScores(game);
}

static void UpdateMenu(FlappyGame *game)
{
    int skinCount = FlappyPlaneSkinCount();

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        game->planeSkin = (game->planeSkin + skinCount - 1) % skinCount;
        PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        game->planeSkin = (game->planeSkin + 1) % skinCount;
        PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        game->difficulty = (FlappyDifficulty)((game->difficulty + FLAPPY_DIFF_COUNT - 1) % FLAPPY_DIFF_COUNT);
        PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        game->difficulty = (FlappyDifficulty)((game->difficulty + 1) % FLAPPY_DIFF_COUNT);
        PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
    }
    for (int i = 0; i < FLAPPY_DIFF_COUNT; i++) {
        if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) {
            game->difficulty = (FlappyDifficulty)i;
            PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
        }
    }

    // Chuột: chọn máy bay, độ khó hoặc bấm nút bắt đầu
    for (int i = 0; i < skinCount; i++) {
        if (ClickedIn(FlappyMenuPlaneCard(i)) && game->planeSkin != i) {
            game->planeSkin = i;
            PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
        }
    }
    for (int i = 0; i < FLAPPY_DIFF_COUNT; i++) {
        if (ClickedIn(FlappyMenuDifficultyChip(i)) && game->difficulty != (FlappyDifficulty)i) {
            game->difficulty = (FlappyDifficulty)i;
            PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
        ClickedIn(FlappyMenuStartButton())) {
        PlayFlappySfx(FLAPPY_SFX_CONFIRM, game->soundEnabled, 0.0f);
        StartRun(game);
        return;
    }

    if (IsKeyPressed(KEY_M)) ToggleSound(game);
    if (IsKeyPressed(KEY_ESCAPE)) game->quitRequested = true;
}

static void UpdatePlaying(FlappyGame *game, float dt)
{
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
        game->pausedFrom = game->state;
        SetState(game, FLAPPY_STATE_PAUSED);
        PlayFlappySfx(FLAPPY_SFX_PAUSE, game->soundEnabled, 0.0f);
        return;
    }
    if (IsKeyPressed(KEY_M)) ToggleSound(game);

    if (FlapPressed()) FlapPlane(game);

    int scoreBefore = game->score;
    UpdatePlaneFlight(game, dt);
    UpdateObstacles(game, dt);
    ScrollWorld(game, dt);
    UpdateEffects(game, dt, game->scrollSpeed);

    if (game->score != scoreBefore) game->scorePop = 1.0f;

    // Đổi vùng địa hình sau mỗi BIOME_EVERY điểm
    int biome = game->score / BIOME_EVERY;
    if (biome > game->biome) {
        game->biome = biome;
        game->biomeBanner = 2.6f;
        PlayFlappySfx(FLAPPY_SFX_LEVEL_UP, game->soundEnabled, 0.0f);
    }

    if (PlaneHitsGround(game)) {
        CrashIntoGround(game);
    } else if (PlaneHitsRock(game)) {
        CrashIntoRock(game);
    }
}

static void UpdateGameOver(FlappyGame *game, float dt)
{
    game->gameOverAnim += dt;
    UpdateEffects(game, dt, 0.0f);

    // Điểm đếm dần lên trên bảng tổng kết
    if (game->shownScore < game->score && game->gameOverAnim > 0.7f) {
        game->countTimer -= dt;
        if (game->countTimer <= 0.0f) {
            int remaining = game->score - game->shownScore;
            game->shownScore += (remaining > 20) ? remaining / 10 : 1;
            game->countTimer = 0.035f;
        }
    }

    if (game->stateTime < GAME_OVER_INPUT_DELAY) return;

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
        IsKeyPressed(KEY_R) || ClickedIn(FlappyGameOverButton(0))) {
        PlayFlappySfx(FLAPPY_SFX_CONFIRM, game->soundEnabled, 0.0f);
        StartRun(game);
    } else if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q) || ClickedIn(FlappyGameOverButton(1))) {
        PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
        GoToMenu(game);
    } else if (IsKeyPressed(KEY_M)) {
        ToggleSound(game);
    }
}

void UpdateFlappyGame(FlappyGame *game, float dt)
{
    game->globalTime += dt;
    game->stateTime += dt;

    if (game->shakeTimer > 0.0f) game->shakeTimer -= dt;
    if (game->flashTimer > 0.0f) game->flashTimer -= dt;
    if (game->biomeBanner > 0.0f) game->biomeBanner -= dt;
    if (game->scorePop > 0.0f) game->scorePop -= dt * 4.0f;

    switch (game->state) {
        case FLAPPY_STATE_MENU:
            game->scrollSpeed = 90.0f;
            ScrollWorld(game, dt);
            UpdateEffects(game, dt, game->scrollSpeed);
            UpdateMenu(game);
            break;

        case FLAPPY_STATE_READY:
            ScrollWorld(game, dt);
            UpdatePlaneHover(game, dt);
            UpdateEffects(game, dt, game->scrollSpeed);
            if (IsKeyPressed(KEY_ESCAPE)) {
                GoToMenu(game);
            } else if (IsKeyPressed(KEY_M)) {
                ToggleSound(game);
            } else if (FlapPressed()) {
                SetState(game, FLAPPY_STATE_PLAYING);
                FlapPlane(game);
            }
            break;

        case FLAPPY_STATE_PLAYING:
            UpdatePlaying(game, dt);
            break;

        case FLAPPY_STATE_PAUSED:
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
                SetState(game, game->pausedFrom);
                PlayFlappySfx(FLAPPY_SFX_PAUSE, game->soundEnabled, 0.0f);
            } else if (IsKeyPressed(KEY_R)) {
                PlayFlappySfx(FLAPPY_SFX_CONFIRM, game->soundEnabled, 0.0f);
                StartRun(game);
            } else if (IsKeyPressed(KEY_Q)) {
                PlayFlappySfx(FLAPPY_SFX_SELECT, game->soundEnabled, 0.0f);
                GoToMenu(game);
            } else if (IsKeyPressed(KEY_M)) {
                ToggleSound(game);
            }
            break;

        case FLAPPY_STATE_DYING:
            UpdateEffects(game, dt, 0.0f);
            if (UpdatePlaneFalling(game, dt)) {
                EmitCrash(game, game->plane.pos);
                Shake(game, 6.0f, 0.25f);
                PlayFlappySfx(FLAPPY_SFX_HIT, game->soundEnabled, 0.1f);
                EnterGameOver(game);
            }
            break;

        case FLAPPY_STATE_GAME_OVER:
            UpdateGameOver(game, dt);
            break;
    }
}
