#include "boot_app.h"
#include "boot_canvas.h"
#include "boot_viewport.h"
#include "boot_stats.h"
#include "boot_settings.h"
#include "boot_input.h"
#include "game_registry.h"
#include "save_data.h"

#include "hub_screen.h"
#include "hub_overlay.h"

#include <math.h>

#define TRANSITION_SPEED 4.5f

// --------------------------------------------------------------- Vòng đời

void BootAppInit(BootApp *app)
{
    app->state = BOOT_STATE_HUB;
    app->activeGame = -1;
    app->selectedGame = 0;
    app->tab = HUB_TAB_LIBRARY;
    app->globalTime = 0.0f;
    app->transition = (BootTransition){.active = false, .alpha = 0.0f, .pendingGame = -1};

    BootSettingsLoad();
    BootStatsLoad();
    for (int i = 0; i < GameRegistryCount(); i++) {
        const GameEntry *g = GameRegistryGet(i);
        if (g && g->migrateSave) g->migrateSave();
    }

    app->canvas = (BootCanvas){0};
    BootViewportUpdate(&app->viewport, HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT);
    HubScreenInit(app);
    BootSettingsApply();
}

void BootAppClose(BootApp *app)
{
    // Kết thúc phiên chơi dở để thời gian không bị mất.
    if (app->state == BOOT_STATE_GAME && app->activeGame >= 0) {
        const GameEntry *game = GameRegistryGet(app->activeGame);
        BootStatsEndSession(app->activeGame);
        if (game && game->close) game->close();
    }

    BootStatsSave();
    BootSettingsSave();

    HubScreenClose();
    CloseBootCanvas(&app->canvas);
}

void BootAppRequestGame(BootApp *app, int gameIndex)
{
    if (app->transition.active) return;

    // Bỏ qua nếu đang ở đúng màn hình được yêu cầu.
    if (app->state == BOOT_STATE_HUB && gameIndex < 0) return;
    if (app->state == BOOT_STATE_GAME && gameIndex == app->activeGame) return;

    app->transition.active = true;
    app->transition.pendingGame = gameIndex;
}

// Thực hiện việc đổi màn hình tại đúng thời điểm màn hình đen hoàn toàn.
static void CommitTransition(BootApp *app)
{
    int target = app->transition.pendingGame;

    // Dọn dẹp game đang chạy.
    if (app->state == BOOT_STATE_GAME && app->activeGame >= 0) {
        const GameEntry *old = GameRegistryGet(app->activeGame);
        BootStatsEndSession(app->activeGame);
        if (old && old->close) old->close();
        app->activeGame = -1;
    }

    if (target < 0) {
        app->state = BOOT_STATE_HUB;
        CloseBootCanvas(&app->canvas);
        SaveDataInvalidate();
    } else {
        const GameEntry *game = GameRegistryGet(target);
        if (game) {
            app->state = BOOT_STATE_GAME;
            app->activeGame = target;
            app->selectedGame = target;
            SetBootCanvasSize(&app->canvas, game->canvasWidth, game->canvasHeight);
            if (game->init) game->init();
            BootStatsBeginSession(target);
        } else {
            app->state = BOOT_STATE_HUB;
            CloseBootCanvas(&app->canvas);
        }
    }

    app->transition.active = false;
    app->transition.pendingGame = -1;
}

static void UpdateTransition(BootApp *app, float dt)
{
    if (app->transition.active) {
        app->transition.alpha += dt * TRANSITION_SPEED;
        if (app->transition.alpha >= 1.0f) {
            app->transition.alpha = 1.0f;
            CommitTransition(app);
        }
    } else if (app->transition.alpha > 0.0f) {
        app->transition.alpha -= dt * TRANSITION_SPEED;
        if (app->transition.alpha < 0.0f) app->transition.alpha = 0.0f;
    }
}

void BootAppUpdate(BootApp *app, float dt)
{
    app->globalTime += dt;

    // Toàn bộ input đi qua boot_input; ở đây chỉ nói "hành động nào" chứ
    // không nhắc tới phím cụ thể nào.
    BootInputUpdate(dt);

    if (BootInputPressed(BOOT_ACTION_FULLSCREEN)) ToggleFullscreen();
    if (BootInputPressed(BOOT_ACTION_SCREENSHOT)) TakeScreenshot("arcade_hub_screenshot.png");
    if (BootInputPressed(BOOT_ACTION_TOGGLE_FPS)) {
        BootSettings *cfg = BootSettingsGet();
        cfg->showFps = !cfg->showFps;
        BootSettingsSave();
    }

    if (app->state == BOOT_STATE_GAME && BootInputPressed(BOOT_ACTION_HOME)) {
        BootAppRequestGame(app, -1);
    }

    UpdateTransition(app, dt);

    // Trong lúc màn hình đang tối dần thì ngừng nhận thao tác để tránh
    // người dùng bấm thêm vào màn hình sắp biến mất.
    if (app->transition.active) return;

    if (app->state == BOOT_STATE_HUB) {
        HubScreenUpdate(app, dt);
    } else {
        const GameEntry *game = GameRegistryGet(app->activeGame);
        if (game && game->update) game->update(dt);
    }
}

void BootAppDraw(BootApp *app)
{
    if (app->state == BOOT_STATE_HUB) {
        // Hub vẽ trực tiếp ở độ phân giải cửa sổ nên chữ luôn sắc nét.
        // Nền mờ và tranh hero vẫn cần render target riêng, phải xong trước.
        BootViewportUpdate(&app->viewport, HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT);
        HubScreenPrepare(app);

        BootViewportBegin(&app->viewport);
            HubScreenDraw(app);
            if (app->transition.alpha > 0.001f) {
                DrawRectangle(0, 0, HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT,
                              Fade(BLACK, app->transition.alpha));
            }
        BootViewportEnd(&app->viewport);
    } else {
        // Game giữ nguyên cách cũ: vẽ vào canvas ảo cố định rồi phóng to,
        // đúng như từng game được thiết kế.
        BeginBootCanvas(&app->canvas);
            const GameEntry *game = GameRegistryGet(app->activeGame);
            if (game && game->draw) game->draw();
            HubOverlayDraw(app);

            if (app->transition.alpha > 0.001f) {
                DrawRectangle(0, 0, app->canvas.virtualWidth, app->canvas.virtualHeight,
                              Fade(BLACK, app->transition.alpha));
            }
        EndBootCanvas(&app->canvas);
    }
}
