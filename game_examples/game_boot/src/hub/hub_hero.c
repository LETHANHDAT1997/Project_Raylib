#include "hub_hero.h"
#include "boot_app.h"
#include "boot_stats.h"
#include "ui_theme.h"
#include "ui_glass.h"
#include "ui_widgets.h"
#include "ui_icons.h"
#include "ui_anim.h"
#include "boot_viewport.h"
#include "boot_perf.h"

#include <math.h>
#include <stdio.h>

#define HERO_PAD_X   44.0f
#define PLAY_WIDTH   196.0f
#define PLAY_HEIGHT  52.0f
#define MENU_WIDTH   260.0f

static RenderTexture2D s_art = {0};
static int  s_artW = 0, s_artH = 0;
static bool s_menuOpen = false;
static float s_playGlow = 0.0f;

void HubHeroRelease(void)
{
    if (IsRenderTextureValid(s_art)) UnloadRenderTexture(s_art);
    s_art = (RenderTexture2D){0};
    s_artW = s_artH = 0;
}

static void EnsureArtTexture(int w, int h)
{
    if (s_artW == w && s_artH == h && IsRenderTextureValid(s_art)) return;

    HubHeroRelease();
    s_art = LoadRenderTexture(w, h);
    SetTextureFilter(s_art.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(s_art.texture, TEXTURE_WRAP_CLAMP);
    s_artW = w;
    s_artH = h;
}

void HubHeroPrepare(HubContext *ctx, Rectangle area)
{
    // Vẽ tranh ở đúng số pixel mà nó chiếm trên màn hình thật (giới hạn 3x
    // để không phình texture trên màn hình rất lớn).
    float scale = BootViewportScale();
    if (scale < 1.0f) scale = 1.0f;
    if (scale > 3.0f) scale = 3.0f;
    // Máy yếu: vẽ ở độ phân giải thiết kế rồi phóng lên, đỡ tốn fill-rate.
    if (BootPerfLite()) scale = 1.0f;

    int w = (int)(area.width * scale + 0.5f);
    int h = (int)(area.height * scale + 0.5f);
    if (w <= 0 || h <= 0) return;

    EnsureArtTexture(w, h);
    if (!IsRenderTextureValid(s_art)) return;

    BeginTextureMode(s_art);
        ClearBackground((Color){12, 18, 38, 255});

        Rectangle local = {0.0f, 0.0f, (float)w, (float)h};
        if (ctx->game && ctx->game->drawHeroArt) {
            ctx->game->drawHeroArt(local, ctx->time);
        }

        // Lớp phủ tối dần từ trái sang phải để chữ luôn đọc được, bất kể
        // tranh phía dưới sáng hay tối. Nướng thẳng vào texture nên góc bo
        // của tấm kính vẫn sạch sẽ.
        DrawRectangleGradientH(0, 0, (int)(w * 0.62f), h,
                               (Color){6, 12, 30, 226}, (Color){6, 12, 30, 0});
        DrawRectangleGradientV(0, (int)(h * 0.55f), w, (int)(h * 0.45f),
                               (Color){4, 9, 24, 0}, (Color){4, 9, 24, 150});
    EndTextureMode();
}

// Danh sách thẻ thể loại của game hiện tại.
static void DrawTags(HubContext *ctx, Vector2 origin)
{
    if (!ctx->game) return;
    UiChipRow(origin, ctx->game->tags, ctx->game->tagCount, 28.0f, UI_PAD_XS + 2.0f, ctx->accent);
}

void HubHeroDraw(HubContext *ctx, Rectangle area)
{
    const GameEntry *game = ctx->game;
    if (!game) return;

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    UiGlassShadow(area, UI_RADIUS_LG, 34.0f, 0.34f);

    // Tranh game bo góc, viền kính mỏng bao quanh.
    UiGlassStyle artStyle = UiGlassStylePanel();
    // Tranh hero hiện đúng như đã vẽ: không chuẩn hoá độ sáng, không phủ màu.
    artStyle.level = 0.0f;
    artStyle.saturation = 1.0f;
    artStyle.tint = game->accentDeep;
    artStyle.tintStrength = 0.08f;
    artStyle.refraction = 6.0f;
    artStyle.alpha = 1.0f;
    UiGlassImagePanel(area, UI_RADIUS_LG, s_art.texture, true, artStyle);
    UiGlassStroke(area, UI_RADIUS_LG, 1.3f, UiAlpha(UI.strokeStrong, 0.75f));

    float x = area.x + HERO_PAD_X;

    // Dòng nhãn nhỏ phía trên tiêu đề
    UiTextTracked(game->platform, (Vector2){x, area.y + 46.0f}, UI_FS_TINY,
                  UI_TRACK_WIDE, UiAlpha(ctx->accent, 0.95f), true);

    // Tiêu đề lớn
    UiTextTracked(game->title, (Vector2){x, area.y + 68.0f}, UI_FS_DISPLAY,
                  0.5f, UI.textPrimary, true);

    // Mô tả ngắn, tự ngắt dòng trong khung hẹp bên trái.
    UiTextBox(game->tagline, (Rectangle){x, area.y + 132.0f, area.width * 0.44f, 80.0f},
              UI_FS_BODY + 1.0f, 7.0f, UI.textSecondary, UI_ALIGN_LEFT, false);

    DrawTags(ctx, (Vector2){x, area.y + 214.0f});

    // Nút chơi + nút menu phụ
    Rectangle play = {x, area.y + area.height - PLAY_HEIGHT - 48.0f, PLAY_WIDTH, PLAY_HEIGHT};
    UiInteraction playIt = UiHitTest(play);
    s_playGlow = UiApproach(s_playGlow, playIt.hovered ? 1.0f : 0.0f, 10.0f, dt);

    if (s_playGlow > 0.01f) {
        UiGlassGlow(play, play.height * 0.5f, 30.0f, UiAlpha(ctx->accent, 0.30f * s_playGlow));
    }

    if (UiGlassButton(play, "Chơi ngay", ctx->accent, true, true).clicked) {
        BootAppRequestGame(ctx->app, ctx->gameIndex);
    }
    // Icon tam giác nhỏ đứng trước nhãn nút.
    UiIconPlay((Rectangle){play.x + 26.0f, play.y + play.height * 0.5f - 8.0f, 16.0f, 16.0f},
               UiAlpha(UI.textOnAccent, 0.9f));

    Rectangle menuBtn = {play.x + play.width + UI_PAD_SM, play.y, PLAY_HEIGHT, PLAY_HEIGHT};
    if (UiIconButton(menuBtn, UiIconEllipsis, ctx->accent, s_menuOpen).clicked) {
        s_menuOpen = !s_menuOpen;
    }

    // Nhãn trạng thái ở góc phải trên: lần chơi gần nhất.
    const GameStats *stats = BootStatsGet(ctx->gameIndex);
    const char *when = BootStatsFormatRelative(stats->lastPlayedUnix);
    float badgeW = UiTextWidth(when, UI_FS_SMALL, false) + UI_PAD_MD * 2.0f;
    Rectangle badge = {area.x + area.width - badgeW - UI_PAD_LG, area.y + UI_PAD_LG, badgeW, 28.0f};
    UiChip(badge, when, ctx->accent, false);
}

void HubHeroDrawOverlay(HubContext *ctx, Rectangle area)
{
    if (!s_menuOpen) return;

    Rectangle play = {area.x + HERO_PAD_X, area.y + area.height - PLAY_HEIGHT - 48.0f, PLAY_WIDTH, PLAY_HEIGHT};
    Rectangle menuBtn = {play.x + play.width + UI_PAD_SM, play.y, PLAY_HEIGHT, PLAY_HEIGHT};

    const char *items[2] = {"Xem bảng phím điều khiển", "Xoá thống kê & kỷ lục game này"};
    float itemH = 40.0f;
    Rectangle menu = {menuBtn.x, menuBtn.y + menuBtn.height + UI_PAD_XS, MENU_WIDTH,
                      itemH * 2.0f + UI_PAD_XS * 2.0f};

    UiGlassShadow(menu, UI_RADIUS_MD, 26.0f, 0.38f);
    UiGlassPanel(menu, UI_RADIUS_MD, UiGlassStyleRaised());
    UiGlassStroke(menu, UI_RADIUS_MD, 1.2f, UI.strokeStrong);

    for (int i = 0; i < 2; i++) {
        Rectangle row = {menu.x + UI_PAD_XS, menu.y + UI_PAD_XS + i * itemH,
                         menu.width - UI_PAD_XS * 2.0f, itemH};
        UiInteraction it = UiHitTest(row);

        if (it.hovered) {
            UiGlassStyle hl = UiGlassStyleAccent(ctx->accent);
            hl.tintStrength = 0.55f;
            UiGlassPanel(row, UI_RADIUS_SM, hl);
        }
        UiText(items[i], (Vector2){row.x + UI_PAD_SM, row.y + (itemH - UI_FS_BODY) * 0.5f - 1.0f},
               UI_FS_BODY, it.hovered ? UI.textOnAccent : UI.textPrimary);

        if (it.clicked) {
            if (i == 0) ctx->app->tab = HUB_TAB_CONTROLS;
            else        BootStatsReset(ctx->gameIndex);
            s_menuOpen = false;
        }
    }

    // Nhấn ra ngoài menu (và ngoài nút mở) thì đóng lại.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(UiMouse(), menu) &&
        !CheckCollisionPointRec(UiMouse(), menuBtn)) {
        s_menuOpen = false;
    }
}
