#include "hub_sidebar.h"
#include "boot_app.h"
#include "boot_stats.h"
#include "ui_theme.h"
#include "ui_glass.h"
#include "ui_widgets.h"
#include "ui_icons.h"
#include "ui_anim.h"

#include <stdio.h>
#include <math.h>

#define ROW_HEIGHT     76.0f
#define ROW_GAP        6.0f
#define HEADER_HEIGHT  58.0f
#define SUMMARY_HEIGHT 126.0f
#define LIST_PAD       12.0f

static Rectangle s_highlight = {0};
static bool  s_highlightReady = false;
static int   s_scrollRow = 0;       // Hàng đầu tiên đang hiển thị
static float s_hoverGlow[BOOT_MAX_GAMES] = {0};
static int    s_lastClickIndex = -1;
static double s_lastClickTime = 0.0;

static Rectangle ListArea(Rectangle panel)
{
    float top = panel.y + HEADER_HEIGHT;
    float bottom = panel.y + panel.height - SUMMARY_HEIGHT - LIST_PAD;
    return (Rectangle){panel.x + LIST_PAD, top, panel.width - LIST_PAD * 2.0f, bottom - top};
}

// Số hàng vừa khít vùng danh sách; luôn hiển thị hàng nguyên vẹn để không
// phải cắt xén bằng scissor (vốn tính sai toạ độ khi vẽ vào render texture).
static int VisibleRows(Rectangle list)
{
    int n = (int)floorf((list.height + ROW_GAP) / (ROW_HEIGHT + ROW_GAP));
    return (n < 1) ? 1 : n;
}

static Rectangle RowRect(Rectangle list, int visualIndex)
{
    return (Rectangle){list.x, list.y + visualIndex * (ROW_HEIGHT + ROW_GAP), list.width, ROW_HEIGHT};
}

void HubSidebarUpdate(HubContext *ctx, Rectangle panel, float dt)
{
    int count = GameRegistryCount();
    Rectangle list = ListArea(panel);
    int visible = VisibleRows(list);

    // Cuộn bằng con lăn khi danh sách dài hơn khung.
    if (count > visible && CheckCollisionPointRec(UiMouse(), list)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) s_scrollRow -= (int)wheel;
    }

    // Luôn kéo mục đang chọn vào tầm nhìn.
    if (ctx->gameIndex < s_scrollRow) s_scrollRow = ctx->gameIndex;
    if (ctx->gameIndex >= s_scrollRow + visible) s_scrollRow = ctx->gameIndex - visible + 1;

    int maxScroll = count - visible;
    if (maxScroll < 0) maxScroll = 0;
    if (s_scrollRow > maxScroll) s_scrollRow = maxScroll;
    if (s_scrollRow < 0) s_scrollRow = 0;

    // Cập nhật độ sáng hover của từng hàng.
    for (int i = 0; i < count && i < BOOT_MAX_GAMES; i++) {
        int visual = i - s_scrollRow;
        bool hovered = false;
        if (visual >= 0 && visual < visible) {
            hovered = CheckCollisionPointRec(UiMouse(), RowRect(list, visual));
        }
        s_hoverGlow[i] = UiApproach(s_hoverGlow[i], hovered ? 1.0f : 0.0f, 12.0f, dt);
    }

    // Viên kính nền của mục đang chọn trượt theo.
    int selVisual = ctx->gameIndex - s_scrollRow;
    if (selVisual >= 0 && selVisual < visible) {
        Rectangle target = RowRect(list, selVisual);
        if (!s_highlightReady) {
            s_highlight = target;
            s_highlightReady = true;
        }
        s_highlight = UiApproachRect(s_highlight, target, ctx->reduceMotion ? 1000.0f : 16.0f, dt);
    }

    // Chuột: nhấn để chọn, nhấn lần hai (hoặc nhấn đúp) để vào chơi.
    // Cố tình KHÔNG chọn theo di chuột: nếu không, chỉ cần lia chuột ngang
    // qua danh sách là lựa chọn bằng bàn phím bị nhảy mất.
    for (int visual = 0; visual < visible; visual++) {
        int index = s_scrollRow + visual;
        if (index >= count) break;

        if (!UiHitTest(RowRect(list, visual)).clicked) continue;

        double now = GetTime();
        bool doubleClick = (index == s_lastClickIndex) && (now - s_lastClickTime < 0.4);

        if (index == ctx->app->selectedGame || doubleClick) {
            BootAppRequestGame(ctx->app, index);
        } else {
            ctx->app->selectedGame = index;
        }

        s_lastClickIndex = index;
        s_lastClickTime = now;
    }
}

static void DrawThumbnail(const GameEntry *game, Rectangle box, float time, bool selected)
{
    float radius = box.width * 0.30f;

    UiGlassStyle style = UiGlassStyleAccent(game->accentDeep);
    style.tintStrength = selected ? 0.86f : 0.70f;
    style.targetLum = selected ? 0.54f : 0.44f;
    UiGlassPanel(box, radius, style);

    if (game->drawIcon) {
        float pad = box.width * 0.16f;
        game->drawIcon((Rectangle){box.x + pad, box.y + pad,
                                   box.width - pad * 2.0f, box.height - pad * 2.0f}, time);
    }
    UiGlassStroke(box, radius, 1.1f, selected ? UiAlpha(WHITE, 0.55f) : UI.strokeSoft);
}

static void DrawRow(HubContext *ctx, Rectangle row, int index, bool selected)
{
    const GameEntry *game = GameRegistryGet(index);
    if (!game) return;

    float hover = s_hoverGlow[index];
    float radius = UI_RADIUS_MD;

    if (!selected && hover > 0.01f) {
        UiGlassStyle style = UiGlassStyleRaised();
        style.alpha = 0.55f * hover;
        UiGlassPanel(row, radius, style);
    }

    float thumb = ROW_HEIGHT - 20.0f;
    Rectangle thumbBox = {row.x + 10.0f, row.y + 10.0f, thumb, thumb};
    DrawThumbnail(game, thumbBox, ctx->time, selected);

    float tx = thumbBox.x + thumb + UI_PAD_SM;
    float maxW = row.x + row.width - tx - 30.0f;

    Color titleCol = selected ? UI.textPrimary : UiAlpha(UI.textPrimary, 0.88f);
    UiTextBold(UiTextEllipsis(game->title, UI_FS_H3, true, maxW),
               (Vector2){tx, row.y + 20.0f}, UI_FS_H3, titleCol);

    // Dòng phụ: nền tảng kèm chấm tròn màu nhấn của game.
    float dotY = row.y + 46.0f;
    DrawCircleV((Vector2){tx + 4.0f, dotY + 6.0f}, 3.5f, UiAlpha(game->accent, selected ? 1.0f : 0.7f));
    UiText(UiTextEllipsis(game->platform, UI_FS_SMALL, false, maxW - 16.0f),
           (Vector2){tx + 14.0f, dotY}, UI_FS_SMALL, UI.textMuted);

    if (selected) {
        Rectangle chev = {row.x + row.width - 26.0f, row.y + ROW_HEIGHT * 0.5f - 8.0f, 16.0f, 16.0f};
        UiIconChevronRight(chev, UiAlpha(UI.textPrimary, 0.85f));
    }
}

static void DrawSummary(HubContext *ctx, Rectangle box)
{
    UiGlassPanel(box, UI_RADIUS_MD, UiGlassStyleSunken());
    UiGlassStroke(box, UI_RADIUS_MD, 1.0f, UiAlpha(UI.strokeSoft, 0.8f));

    Rectangle inner = {box.x + UI_PAD_MD, box.y + UI_PAD_SM,
                       box.width - UI_PAD_MD * 2.0f, box.height - UI_PAD_SM * 2.0f};

    UiTextTracked("TỔNG QUAN", (Vector2){inner.x, inner.y}, UI_FS_TINY, 1.8f, UI.textMuted, true);

    double total = BootStatsTotalPlaytime();
    int played = BootStatsPlayedGameCount();
    int count = GameRegistryCount();

    UiStatLine((Rectangle){inner.x, inner.y + 22.0f, inner.width, 22.0f},
               UiIconClock, "Tổng giờ chơi", BootStatsFormatDuration(total));

    char progress[32];
    snprintf(progress, sizeof(progress), "%d/%d", played, count);
    UiStatLine((Rectangle){inner.x, inner.y + 46.0f, inner.width, 22.0f},
               UiIconTrophy, "Đã trải nghiệm", progress);

    int favourite = BootStatsMostPlayedIndex();
    const GameEntry *fav = (favourite >= 0) ? GameRegistryGet(favourite) : NULL;
    UiStatLine((Rectangle){inner.x, inner.y + 70.0f, inner.width, 22.0f},
               UiIconCounter, "Chơi nhiều nhất", fav ? fav->title : "—");

    Rectangle bar = {inner.x, inner.y + 100.0f, inner.width, 6.0f};
    UiProgressBar(bar, (count > 0) ? (float)played / (float)count : 0.0f, ctx->accent);
}

void HubSidebarDraw(HubContext *ctx, Rectangle panel)
{
    UiGlassShadow(panel, UI_RADIUS_LG, 26.0f, 0.22f);
    UiGlassPanel(panel, UI_RADIUS_LG, UiGlassStylePanel());
    UiGlassStroke(panel, UI_RADIUS_LG, 1.2f, UI.strokeSoft);

    // Tiêu đề khu vực
    int count = GameRegistryCount();
    UiTextBold("Thư viện game", (Vector2){panel.x + UI_PAD_LG, panel.y + UI_PAD_MD}, UI_FS_H3, UI.textPrimary);

    char badge[24];
    snprintf(badge, sizeof(badge), "%d", count);
    float badgeW = UiTextWidth(badge, UI_FS_SMALL, true) + UI_PAD_SM * 2.0f;
    Rectangle badgeRec = {panel.x + panel.width - UI_PAD_LG - badgeW, panel.y + UI_PAD_MD - 2.0f, badgeW, 22.0f};
    UiChip(badgeRec, badge, ctx->accent, false);

    Rectangle list = ListArea(panel);
    int visible = VisibleRows(list);

    // Nền của mục đang chọn vẽ trước để các hàng nằm đè lên trên.
    int selVisual = ctx->gameIndex - s_scrollRow;
    if (s_highlightReady && selVisual >= 0 && selVisual < visible) {
        UiGlassStyle style = UiGlassStyleRaised();
        // Mục đang chọn được phép mang màu nhấn: đó là tín hiệu trạng thái,
        // không phải màu nền của vật liệu kính.
        style.tint = ctx->accent;
        style.tintStrength = 0.26f;
        style.targetLum = 0.56f;
        UiGlassGlow(s_highlight, UI_RADIUS_MD, 20.0f, UiAlpha(ctx->accent, 0.22f));
        UiGlassPanel(s_highlight, UI_RADIUS_MD, style);
        UiGlassStroke(s_highlight, UI_RADIUS_MD, 1.3f, UiAlpha(UI.strokeHighlight, 0.85f));
    }

    for (int visual = 0; visual < visible; visual++) {
        int index = s_scrollRow + visual;
        if (index >= count) break;
        DrawRow(ctx, RowRect(list, visual), index, index == ctx->gameIndex);
    }

    // Gợi ý còn game phía dưới khi danh sách bị cắt.
    if (count > visible) {
        char more[48];
        snprintf(more, sizeof(more), "Còn %d game khác · cuộn chuột", count - visible);
        UiText(more, (Vector2){list.x + 4.0f, list.y + list.height - 16.0f}, UI_FS_TINY, UI.textMuted);
    }

    Rectangle summary = {panel.x + LIST_PAD, panel.y + panel.height - SUMMARY_HEIGHT - LIST_PAD + 4.0f,
                         panel.width - LIST_PAD * 2.0f, SUMMARY_HEIGHT};
    DrawSummary(ctx, summary);
}
