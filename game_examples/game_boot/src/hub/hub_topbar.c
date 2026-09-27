#include "hub_topbar.h"
#include "ui_theme.h"
#include "ui_glass.h"
#include "ui_widgets.h"
#include "ui_icons.h"
#include "ui_anim.h"
#include "boot_settings.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

static const char *TAB_LABELS[HUB_TAB_COUNT] = {"Thư viện", "Điều khiển", "Cài đặt"};
static const UiIconFn TAB_ICONS[HUB_TAB_COUNT] = {UiIconLibrary, UiIconKeyboard, UiIconSliders};

// Viên thuốc sáng chạy mượt từ tab cũ sang tab mới thay vì nhảy tức thì.
static Rectangle s_pill = {0};
static bool s_pillReady = false;

static void DrawBrand(HubContext *ctx, Rectangle area)
{
    float size = area.height - 10.0f;
    Rectangle badge = {area.x, area.y + 5.0f, size, size};

    UiGlassStyle style = UiGlassStyleAccent(ctx->accent);
    UiGlassPanel(badge, size * 0.32f, style);
    UiGlassStroke(badge, size * 0.32f, 1.1f, UiAlpha(WHITE, 0.45f));
    UiIconGamepad((Rectangle){badge.x + size * 0.20f, badge.y + size * 0.20f,
                              size * 0.60f, size * 0.60f}, UI.textOnAccent);

    float tx = badge.x + size + UI_PAD_SM;
    UiTextBold("Arcade Hub", (Vector2){tx, area.y + 9.0f}, UI_FS_H3, UI.textPrimary);
    UiTextTracked("RAYLIB COLLECTION", (Vector2){tx, area.y + 29.0f}, UI_FS_TINY - 1.0f,
                  1.7f, UiAlpha(UI.textSecondary, 0.72f), false);
}

static void DrawTabs(HubContext *ctx, Rectangle area, float dt)
{
    float h = 36.0f;
    float y = area.y + (area.height - h) * 0.5f;
    float x = area.x;
    float iconSize = 15.0f;

    Rectangle rects[HUB_TAB_COUNT];
    for (int i = 0; i < HUB_TAB_COUNT; i++) {
        float w = UiTextWidth(TAB_LABELS[i], UI_FS_BODY, true) + iconSize + UI_PAD_SM + UI_PAD_MD * 2.0f;
        rects[i] = (Rectangle){x, y, w, h};
        x += w + UI_PAD_XS;
    }

    Rectangle target = rects[ctx->app->tab];
    if (!s_pillReady) {
        s_pill = target;
        s_pillReady = true;
    }
    s_pill = UiApproachRect(s_pill, target, ctx->reduceMotion ? 1000.0f : 18.0f, dt);

    UiGlassStyle pillStyle = UiGlassStyleRaised();
    pillStyle.tint = ctx->accent;
    pillStyle.tintStrength = 0.26f;
    pillStyle.targetLum = 0.56f;
    UiGlassPanel(s_pill, h * 0.5f, pillStyle);
    UiGlassStroke(s_pill, h * 0.5f, 1.1f, UiAlpha(UI.strokeStrong, 0.9f));

    for (int i = 0; i < HUB_TAB_COUNT; i++) {
        UiInteraction it = UiHitTest(rects[i]);
        if (it.clicked) ctx->app->tab = (HubTab)i;

        bool active = (ctx->app->tab == (HubTab)i);
        Color col = active ? UI.textPrimary : (it.hovered ? UI.textPrimary : UiAlpha(UI.textSecondary, 0.82f));

        float textW = UiTextWidth(TAB_LABELS[i], UI_FS_BODY, active);
        float startX = rects[i].x + (rects[i].width - (textW + iconSize + UI_PAD_XS)) * 0.5f;

        if (TAB_ICONS[i]) {
            TAB_ICONS[i]((Rectangle){startX, rects[i].y + (h - iconSize) * 0.5f, iconSize, iconSize},
                         active ? ctx->accent : UiAlpha(col, 0.8f));
        }
        UiTextTracked(TAB_LABELS[i],
                      (Vector2){startX + iconSize + UI_PAD_XS, rects[i].y + (h - UI_FS_BODY) * 0.5f - 1.0f},
                      UI_FS_BODY, UI_TRACK_NORMAL, col, active);
    }
}

// Cụm chỉ báo bên phải: FPS (tuỳ chọn), âm lượng và đồng hồ hệ thống.
// Toàn bộ dùng màu chữ trung tính - đây là thông tin trạng thái của máy,
// không liên quan tới game đang chọn nên không nhuộm theo màu nhấn.
static void DrawStatusCluster(Rectangle area)
{
    BootSettings *cfg = BootSettingsGet();

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char clock[16];
    if (lt) snprintf(clock, sizeof(clock), "%02d:%02d", lt->tm_hour, lt->tm_min);
    else    snprintf(clock, sizeof(clock), "--:--");

    float cy = area.y + area.height * 0.5f;
    float x = area.x + area.width;

    // Đồng hồ nằm ngoài cùng bên phải.
    float clockW = UiTextWidth(clock, UI_FS_BODY, true);
    x -= clockW;
    UiTextBold(clock, (Vector2){x, cy - UI_FS_BODY * 0.5f - 1.0f}, UI_FS_BODY, UI.textPrimary);

    // Âm lượng
    x -= UI_PAD_MD;
    char vol[16];
    snprintf(vol, sizeof(vol), "%d%%", (int)(cfg->masterVolume * 100.0f + 0.5f));
    float volW = UiTextWidth(vol, UI_FS_SMALL, false);
    x -= volW;
    UiText(vol, (Vector2){x, cy - UI_FS_SMALL * 0.5f - 1.0f}, UI_FS_SMALL, UI.textSecondary);

    x -= UI_PAD_XS + 16.0f;
    UiIconSpeaker((Rectangle){x, cy - 8.0f, 16.0f, 16.0f}, UiAlpha(UI.textSecondary, 0.9f));

    if (cfg->showFps) {
        x -= UI_PAD_MD;
        char fps[24];
        snprintf(fps, sizeof(fps), "%d FPS", GetFPS());
        float fpsW = UiTextWidth(fps, UI_FS_SMALL, false);
        x -= fpsW;
        UiText(fps, (Vector2){x, cy - UI_FS_SMALL * 0.5f - 1.0f}, UI_FS_SMALL,
               UI.textSecondary);
    }
}

void HubTopbarDraw(HubContext *ctx, Rectangle area)
{
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    DrawBrand(ctx, (Rectangle){area.x, area.y, 210.0f, area.height});
    DrawTabs(ctx, (Rectangle){area.x + 240.0f, area.y, 420.0f, area.height}, dt);
    DrawStatusCluster((Rectangle){area.x + area.width - 340.0f, area.y, 340.0f, area.height});

    // Đường kẻ mảnh ngăn thanh điều hướng với nội dung.
    UiSeparator((Vector2){area.x, area.y + area.height + UI_PAD_XS}, area.width, false);
}
