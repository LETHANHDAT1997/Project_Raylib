#include "hub_info.h"
#include "boot_stats.h"
#include "ui_theme.h"
#include "ui_glass.h"
#include "ui_widgets.h"
#include "ui_icons.h"

#include <stdio.h>
#include <math.h>

#define CARD_GAP 18.0f

static void DrawCardShell(Rectangle card, const char *title)
{
    UiGlassShadow(card, UI_RADIUS_LG, 24.0f, 0.22f);
    UiGlassPanel(card, UI_RADIUS_LG, UiGlassStylePanel());
    UiGlassStroke(card, UI_RADIUS_LG, 1.2f, UI.strokeSoft);

    UiTextBold(title, (Vector2){card.x + UI_PAD_LG, card.y + UI_PAD_MD}, UI_FS_H3, UI.textPrimary);
}

// Thẻ trái: mọi con số đều lấy từ thống kê thật đã lưu trên đĩa.
static void DrawLastPlayedCard(HubContext *ctx, Rectangle card)
{
    DrawCardShell(card, "Lần chơi gần đây");

    const GameStats *stats = BootStatsGet(ctx->gameIndex);
    const GameEntry *game = ctx->game;

    float thumb = 62.0f;
    Rectangle thumbBox = {card.x + UI_PAD_LG, card.y + 52.0f, thumb, thumb};

    UiGlassStyle style = UiGlassStyleAccent(game ? game->accentDeep : ctx->accent);
    style.tintStrength = 0.78f;
    UiGlassPanel(thumbBox, thumb * 0.30f, style);
    if (game && game->drawIcon) {
        float pad = thumb * 0.17f;
        game->drawIcon((Rectangle){thumbBox.x + pad, thumbBox.y + pad,
                                   thumb - pad * 2.0f, thumb - pad * 2.0f}, ctx->time);
    }
    UiGlassStroke(thumbBox, thumb * 0.30f, 1.1f, UiAlpha(WHITE, 0.4f));

    float tx = thumbBox.x + thumb + UI_PAD_MD;
    UiText("Chơi gần nhất", (Vector2){tx, card.y + 52.0f}, UI_FS_SMALL, UI.textMuted);
    UiTextBold(BootStatsFormatRelative(stats->lastPlayedUnix),
               (Vector2){tx, card.y + 70.0f}, UI_FS_H2, UI.textPrimary);

    // Thời lượng phiên gần nhất, canh phải trong thẻ.
    const char *dur = BootStatsFormatDuration(stats->lastSeconds);
    float durW = UiTextWidth(dur, UI_FS_BODY, true);
    float durX = card.x + card.width - UI_PAD_LG - durW;
    UiIconClock((Rectangle){durX - 20.0f, card.y + 72.0f, 14.0f, 14.0f}, UI.textMuted);
    UiTextBold(dur, (Vector2){durX, card.y + 71.0f}, UI_FS_BODY, UI.textSecondary);

    // Thanh so sánh: game này chiếm bao nhiêu so với game được chơi nhiều nhất.
    double maxPlay = BootStatsMaxPlaytime();
    float ratio = (maxPlay > 0.0) ? (float)(stats->totalSeconds / maxPlay) : 0.0f;

    float barY = card.y + card.height - 42.0f;
    UiText("Tổng thời gian tích luỹ", (Vector2){card.x + UI_PAD_LG, barY - 20.0f}, UI_FS_SMALL, UI.textMuted);

    char pct[24];
    snprintf(pct, sizeof(pct), "%s", BootStatsFormatDuration(stats->totalSeconds));
    float pctW = UiTextWidth(pct, UI_FS_SMALL, true);
    UiTextBold(pct, (Vector2){card.x + card.width - UI_PAD_LG - pctW, barY - 20.0f}, UI_FS_SMALL, UI.textSecondary);

    UiProgressBar((Rectangle){card.x + UI_PAD_LG, barY, card.width - UI_PAD_LG * 2.0f, 8.0f},
                  ratio, ctx->accent);
}

// Thẻ phải: thông tin tĩnh của game lấy từ registry + số liệu runtime.
static void DrawGameInfoCard(HubContext *ctx, Rectangle card)
{
    DrawCardShell(card, "Thông tin game");

    const GameEntry *game = ctx->game;
    if (!game) return;

    const GameStats *stats = BootStatsGet(ctx->gameIndex);

    char resolution[32];
    snprintf(resolution, sizeof(resolution), "%d × %d", game->canvasWidth, game->canvasHeight);

    char sessions[32];
    snprintf(sessions, sizeof(sessions), "%d lần", stats->sessions);

    struct {
        UiIconFn icon;
        const char *label;
        const char *value;
    } rows[4] = {
        {UiIconPerson,   "Nhà phát triển", game->developer},
        {UiIconCalendar, "Phiên bản",      game->releaseDate},
        {UiIconDisplay,  "Độ phân giải",   resolution},
        {UiIconCounter,  "Số lần khởi động", sessions}
    };

    float rowH = 26.0f;
    float y = card.y + 50.0f;
    for (int i = 0; i < 4; i++) {
        UiStatLine((Rectangle){card.x + UI_PAD_LG, y, card.width - UI_PAD_LG * 2.0f, rowH},
                   rows[i].icon, rows[i].label, rows[i].value);
        y += rowH;
        if (i < 3) UiSeparator((Vector2){card.x + UI_PAD_LG, y - 3.0f}, card.width - UI_PAD_LG * 2.0f, false);
    }
}

void HubInfoDraw(HubContext *ctx, Rectangle area)
{
    float cardW = (area.width - CARD_GAP) * 0.5f;

    DrawLastPlayedCard(ctx, (Rectangle){area.x, area.y, cardW, area.height});
    DrawGameInfoCard(ctx, (Rectangle){area.x + cardW + CARD_GAP, area.y, cardW, area.height});
}
