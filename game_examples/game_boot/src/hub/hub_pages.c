#include "hub_pages.h"
#include "boot_stats.h"
#include "save_data.h"
#include "boot_settings.h"
#include "boot_perf.h"
#include "hub_wallpaper.h"
#include "ui_theme.h"
#include "ui_glass.h"
#include "ui_widgets.h"
#include "ui_icons.h"
#include "ui_focus.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ------------------------------------------------------- Trang Điều khiển

// Phím tắt dùng chung cho cả launcher, không phụ thuộc game nào.
static const char *GLOBAL_KEYS[][2] = {
    {"↑ ↓ / W S",        "Chọn game trong thư viện"},
    {"Enter / Space",    "Bắt đầu chơi game đang chọn"},
    {"1 … 9",            "Chọn nhanh theo số thứ tự"},
    {"Tab",              "Chuyển qua lại giữa các trang"},
    {"F1 / Home",        "Thoát game, quay lại Hub"},
    {"F3",               "Bật/tắt bộ đếm FPS (cả khi đang chơi)"},
    {"F11",              "Bật/tắt toàn màn hình"},
    {"Esc",              "Về trang Thư viện"}
};
#define GLOBAL_KEY_COUNT ((int)(sizeof(GLOBAL_KEYS) / sizeof(GLOBAL_KEYS[0])))

// Thẻ phím của từng game: phần đầu (icon + tên) rồi mỗi phím một dòng.
#define CONTROL_CARD_HEADER (UI_PAD_MD + 38.0f + UI_PAD_SM)
#define CONTROL_LINE_H      22.0f

// Một dòng phím: ô phím kiểu keycap bên trái, mô tả bên phải.
static void DrawKeyRow(Rectangle row, const char *key, const char *desc, Color accent)
{
    float capW = 132.0f;
    Rectangle cap = {row.x, row.y + 2.0f, capW, row.height - 4.0f};

    UiGlassStyle style = UiGlassStyleSunken();
    style.tint = accent;
    style.tintStrength = 0.14f;
    UiGlassPanel(cap, UI_RADIUS_SM, style);
    UiGlassStroke(cap, UI_RADIUS_SM, 1.0f, UiAlpha(UI.strokeSoft, 0.9f));

    const char *fitted = UiTextEllipsis(key, UI_FS_SMALL, true, capW - UI_PAD_SM * 2.0f);
    float kw = UiTextWidth(fitted, UI_FS_SMALL, true);
    UiTextBold(fitted, (Vector2){cap.x + (capW - kw) * 0.5f, cap.y + (cap.height - UI_FS_SMALL) * 0.5f - 1.0f},
               UI_FS_SMALL, UI.textPrimary);

    UiText(desc, (Vector2){cap.x + capW + UI_PAD_MD, row.y + (row.height - UI_FS_BODY) * 0.5f - 1.0f},
           UI_FS_BODY, UI.textSecondary);
}

// Thẻ phím của một game trong lưới bên phải.
static void DrawGameControlCard(HubContext *ctx, Rectangle card, const GameEntry *game, bool selected, int focus)
{
    UiGlassStyle style = UiGlassStyleRaised();
    style.alpha = selected ? 0.72f : 0.46f;
    if (selected) {
        style.tint = game->accent;
        style.tintStrength = 0.24f;
        style.targetLum = 0.54f;
    }

    UiGlassPanel(card, UI_RADIUS_MD, style);
    UiGlassStroke(card, UI_RADIUS_MD, 1.1f, selected ? UiAlpha(UI.strokeHighlight, 0.9f) : UiAlpha(UI.strokeSoft, 0.7f));

    // Nhấn hoặc focus rồi Enter để đổi game đang chọn của cả launcher.
    // (focus do trang đăng ký sẵn cho mọi thẻ, kể cả thẻ đang bị cuộn khuất.)
    if (UiHitTest(card).clicked || UiFocusActivated(focus)) {
        ctx->app->selectedGame = GameRegistryIndexOfId(game->id);
    }
    if (UiFocusRingVisible(focus)) UiFocusDrawRing(card, UI_RADIUS_MD, game->accent);

    float thumb = 38.0f;
    Rectangle thumbBox = {card.x + UI_PAD_MD, card.y + UI_PAD_MD, thumb, thumb};
    UiGlassStyle ts = UiGlassStyleAccent(game->accentDeep);
    ts.tintStrength = 0.80f;
    UiGlassPanel(thumbBox, thumb * 0.30f, ts);
    if (game->drawIcon) {
        float pad = thumb * 0.16f;
        game->drawIcon((Rectangle){thumbBox.x + pad, thumbBox.y + pad, thumb - pad * 2.0f, thumb - pad * 2.0f},
                       ctx->time);
    }

    UiTextBold(game->title, (Vector2){thumbBox.x + thumb + UI_PAD_SM, card.y + UI_PAD_MD + 10.0f},
               UI_FS_H3, UI.textPrimary);

    float y = card.y + CONTROL_CARD_HEADER;
    float textW = card.width - UI_PAD_MD * 2.0f - 14.0f;
    for (int i = 0; i < game->controlCount; i++) {
        DrawCircleV((Vector2){card.x + UI_PAD_MD + 3.0f, y + 8.0f}, 3.0f, UiAlpha(game->accent, 0.9f));
        UiText(UiTextEllipsis(game->controls[i], UI_FS_SMALL, false, textW),
               (Vector2){card.x + UI_PAD_MD + 14.0f, y}, UI_FS_SMALL, UI.textSecondary);
        y += CONTROL_LINE_H;
    }
}

void HubControlsPageDraw(HubContext *ctx, Rectangle area)
{
    float gap = 18.0f;
    float leftW = (area.width - gap) * 0.42f;

    // --- Cột trái: phím chung của launcher
    Rectangle left = {area.x, area.y, leftW, area.height};
    UiGlassShadow(left, UI_RADIUS_LG, 26.0f, 0.22f);
    UiGlassPanel(left, UI_RADIUS_LG, UiGlassStylePanel());
    UiGlassStroke(left, UI_RADIUS_LG, 1.2f, UI.strokeSoft);

    UiTextBold("Phím tắt của Hub", (Vector2){left.x + UI_PAD_LG, left.y + UI_PAD_LG}, UI_FS_H2, UI.textPrimary);
    UiText("Dùng được ở mọi trang của launcher",
           (Vector2){left.x + UI_PAD_LG, left.y + UI_PAD_LG + 30.0f}, UI_FS_SMALL, UI.textMuted);

    float rowH = 38.0f;
    float y = left.y + UI_PAD_LG + 62.0f;
    for (int i = 0; i < GLOBAL_KEY_COUNT; i++) {
        DrawKeyRow((Rectangle){left.x + UI_PAD_LG, y, left.width - UI_PAD_LG * 2.0f, rowH},
                   GLOBAL_KEYS[i][0], GLOBAL_KEYS[i][1], ctx->accent);
        y += rowH + 4.0f;
    }

    // Ghi chú cuối cột trái, lấp phần trống và nói rõ dữ liệu nằm ở đâu.
    Rectangle note = {left.x + UI_PAD_LG, left.y + left.height - 96.0f - UI_PAD_LG,
                      left.width - UI_PAD_LG * 2.0f, 96.0f};
    UiGlassPanel(note, UI_RADIUS_MD, UiGlassStyleSunken());
    UiGlassStroke(note, UI_RADIUS_MD, 1.0f, UiAlpha(UI.strokeSoft, 0.7f));
    UiIconSparkle((Rectangle){note.x + UI_PAD_MD, note.y + UI_PAD_MD, 16.0f, 16.0f}, ctx->accent);
    UiTextBold("Ghi chú", (Vector2){note.x + UI_PAD_MD + 24.0f, note.y + UI_PAD_MD}, UI_FS_BODY, UI.textPrimary);
    UiTextBox("Chuột: nhấn một lần để chọn game, nhấn lần nữa (hoặc nhấn đúp) để vào chơi. "
              "Thời gian chơi được ghi lại tự động sau mỗi phiên.",
              (Rectangle){note.x + UI_PAD_MD, note.y + UI_PAD_MD + 24.0f, note.width - UI_PAD_MD * 2.0f, 60.0f},
              UI_FS_SMALL, 4.0f, UI.textSecondary, UI_ALIGN_LEFT, false);

    // --- Cột phải: lưới phím của toàn bộ game trong thư viện
    Rectangle right = {area.x + leftW + gap, area.y, area.width - leftW - gap, area.height};
    UiGlassShadow(right, UI_RADIUS_LG, 26.0f, 0.22f);
    UiGlassPanel(right, UI_RADIUS_LG, UiGlassStylePanel());
    UiGlassStroke(right, UI_RADIUS_LG, 1.2f, UI.strokeSoft);

    UiTextBold("Phím điều khiển từng game",
               (Vector2){right.x + UI_PAD_LG, right.y + UI_PAD_LG}, UI_FS_H2, UI.textPrimary);
    UiText("Nhấn vào một thẻ để chọn game đó trong thư viện",
           (Vector2){right.x + UI_PAD_LG, right.y + UI_PAD_LG + 30.0f}, UI_FS_SMALL, UI.textMuted);

    int count = GameRegistryCount();
    const int cols = 2;
    int totalRows = (count + cols - 1) / cols;

    // Chiều cao thẻ tính theo game có nhiều dòng phím nhất để chữ không bao giờ
    // tràn khỏi thẻ; khung không đủ chỗ thì cuộn theo từng hàng (giống sidebar)
    // thay vì bóp thẻ lại. Thêm game mới không phải chỉnh gì ở đây.
    int maxLines = 1;
    for (int i = 0; i < count; i++) {
        const GameEntry *g = GameRegistryGet(i);
        if (g && g->controlCount > maxLines) maxLines = g->controlCount;
    }
    float cellGap = 14.0f;
    float gridTop = right.y + UI_PAD_LG + 62.0f;
    float gridH = right.y + right.height - UI_PAD_LG - gridTop;
    float cardW = (right.width - UI_PAD_LG * 2.0f - cellGap * (cols - 1)) / (float)cols;
    float cardH = CONTROL_CARD_HEADER + maxLines * CONTROL_LINE_H + UI_PAD_SM;

    int visibleRows = (int)((gridH + cellGap) / (cardH + cellGap));
    if (visibleRows < 1) visibleRows = 1;
    if (visibleRows > totalRows) visibleRows = totalRows;
    if (totalRows <= visibleRows) {
        // Đủ chỗ: giãn thẻ lấp khung (có giới hạn) như trước.
        float fit = (gridH - cellGap * (totalRows - 1)) / (float)(totalRows > 0 ? totalRows : 1);
        if (fit > cardH) cardH = fminf(fit, 210.0f);
    }

    static int s_ctrlScrollRow = 0;
    Rectangle gridArea = {right.x, gridTop, right.width, gridH};
    if (totalRows > visibleRows && CheckCollisionPointRec(UiMouse(), gridArea)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) s_ctrlScrollRow -= (int)wheel;
    }

    // Đăng ký focus cho MỌI thẻ theo đúng vị trí của nó trên lưới (thẻ khuất
    // nằm tiếp phía trên/dưới khung). Nhờ vậy điều hướng không gian bằng mũi tên
    // đi được tới cả thẻ đang bị cuộn khuất, rồi lưới tự cuộn theo.
    int focusIds[BOOT_MAX_GAMES];
    int focusedGame = -1;
    for (int i = 0; i < count && i < BOOT_MAX_GAMES; i++) {
        int row = i / cols - s_ctrlScrollRow;
        Rectangle rec = {right.x + UI_PAD_LG + (i % cols) * (cardW + cellGap),
                         gridTop + row * (cardH + cellGap), cardW, cardH};
        bool shown = (row >= 0 && row < visibleRows);
        focusIds[i] = UiFocusRegister(rec, shown ? UI_FOCUS_FLAG_NONE : UI_FOCUS_NO_MOUSE);
        if (UiFocusIsFocused(focusIds[i])) focusedGame = i;
    }

    // Kéo vào tầm nhìn: thẻ vừa nhận focus (phím mũi tên) hoặc game vừa được
    // chọn (phím số / từ thư viện). Chỉ làm khi chúng ĐỔI, để con lăn chuột
    // vẫn cuộn tự do được.
    static int s_lastFocusGame = -1;
    static int s_lastSelGame = -1;
    int follow = -1;
    if (focusedGame >= 0 && focusedGame != s_lastFocusGame) follow = focusedGame;
    else if (ctx->gameIndex != s_lastSelGame) follow = ctx->gameIndex;
    s_lastFocusGame = focusedGame;
    s_lastSelGame = ctx->gameIndex;
    if (follow >= 0) {
        int followRow = follow / cols;
        if (followRow < s_ctrlScrollRow) s_ctrlScrollRow = followRow;
        if (followRow >= s_ctrlScrollRow + visibleRows) s_ctrlScrollRow = followRow - visibleRows + 1;
    }
    int maxScroll = totalRows - visibleRows;
    if (s_ctrlScrollRow > maxScroll) s_ctrlScrollRow = maxScroll;
    if (s_ctrlScrollRow < 0) s_ctrlScrollRow = 0;

    int first = s_ctrlScrollRow * cols;
    int last = (s_ctrlScrollRow + visibleRows) * cols;
    for (int i = first; i < count && i < last && i < BOOT_MAX_GAMES; i++) {
        const GameEntry *game = GameRegistryGet(i);
        if (!game) continue;

        int col = i % cols;
        int row = i / cols - s_ctrlScrollRow;
        Rectangle card = {right.x + UI_PAD_LG + col * (cardW + cellGap),
                          gridTop + row * (cardH + cellGap), cardW, cardH};
        DrawGameControlCard(ctx, card, game, i == ctx->gameIndex, focusIds[i]);
    }

    // Gợi ý còn game bị khuất, đặt cùng hàng với dòng phụ đề để không chiếm chỗ của thẻ.
    if (totalRows > visibleRows) {
        int above = first;
        int below = count - (last < count ? last : count);
        char more[96];
        if (above > 0 && below > 0) snprintf(more, sizeof(more), "↑ %d · ↓ %d game · phím mũi tên / cuộn chuột", above, below);
        else if (below > 0)         snprintf(more, sizeof(more), "Còn %d game bên dưới · phím ↓ / cuộn chuột", below);
        else                        snprintf(more, sizeof(more), "Còn %d game bên trên · phím ↑ / cuộn chuột", above);
        float mw = UiTextWidth(more, UI_FS_SMALL, false);
        UiText(more, (Vector2){right.x + right.width - UI_PAD_LG - mw, right.y + UI_PAD_LG + 30.0f},
               UI_FS_SMALL, UiAlpha(ctx->accent, 0.95f));
    }
}

// --------------------------------------------------------- Trang Cài đặt

// Ô chọn hình nền: thumbnail bo góc, viền sáng khi đang dùng.
// Trả về true nếu ô vừa được chọn (bằng chuột hoặc bằng bàn phím).
static bool DrawWallpaperTile(HubContext *ctx, Rectangle tile, int index, bool active)
{
    float radius = UI_RADIUS_SM;
    Texture2D thumb = HubWallpaperThumbnail(index);

    if (IsTextureValid(thumb)) {
        UiGlassStyle style = UiGlassStylePanel();
        // Thumbnail hiện đúng màu ảnh; ô chưa chọn chỉ phủ tối nhẹ.
        style.level = 0.0f;
        style.saturation = 1.0f;
        style.tint = BLACK;
        style.tintStrength = active ? 0.0f : 0.30f;
        style.refraction = 4.0f;
        style.alpha = 1.0f;
        UiGlassImagePanel(tile, radius, thumb, false, style);
    } else {
        // Mục gradient động: vẽ thu nhỏ chính hiệu ứng mà nó đại diện,
        // để ô xem trước nói đúng thứ người dùng sẽ nhận được.
        UiGlassStyle style = UiGlassStyleAccent((Color){52, 74, 138, 255});
        style.tintStrength = active ? 0.86f : 0.70f;
        UiGlassPanel(tile, radius, style);

        // Các quầng màu đặt lùi vào trong để không tràn ra góc bo.
        static const Color preview[3] = {
            {170, 196, 255, 255}, {236, 186, 226, 255}, {150, 214, 240, 255}
        };
        static const float px[3] = {0.28f, 0.72f, 0.52f};
        static const float py[3] = {0.34f, 0.32f, 0.72f};

        BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < 3; i++) {
                Vector2 c = {tile.x + tile.width * px[i], tile.y + tile.height * py[i]};
                Color col = UiMix(preview[i], ctx->accent, 0.30f);
                DrawCircleGradient(c, tile.height * 0.42f, UiAlpha(col, 0.55f), UiAlpha(col, 0.0f));
            }
        EndBlendMode();
    }

    UiInteraction it = UiHitTest(tile);

    int focus = UiFocusRegister(tile, UI_FOCUS_FLAG_NONE);
    if (UiFocusActivated(focus)) it.clicked = true;

    if (active) {
        UiGlassGlow(tile, radius, 16.0f, UiAlpha(ctx->accent, 0.30f));
        UiGlassStroke(tile, radius, 2.0f, UiAlpha(UI.strokeHighlight, 0.95f));
    } else {
        UiGlassStroke(tile, radius, 1.0f, it.hovered ? UI.strokeStrong : UiAlpha(UI.strokeSoft, 0.7f));
    }

    // Nhãn nằm trên một dải tối ở đáy ô để đọc được trên mọi ảnh.
    float bandH = 20.0f;
    Rectangle band = {tile.x, tile.y + tile.height - bandH, tile.width, bandH};
    DrawRectangleRec(band, (Color){6, 12, 26, 165});

    const char *label = UiTextEllipsis(HubWallpaperLabel(index), UI_FS_TINY, active, tile.width - 12.0f);
    UiTextTracked(label, (Vector2){tile.x + 6.0f, band.y + 4.0f}, UI_FS_TINY, UI_TRACK_NORMAL,
                  active ? UI.textPrimary : UI.textSecondary, active);

    if (UiFocusRingVisible(focus)) UiFocusDrawRing(tile, radius, ctx->accent);
    return it.clicked;
}

// Lưới chọn hình nền; trả về true nếu người dùng vừa đổi lựa chọn.
static bool DrawWallpaperPicker(HubContext *ctx, Rectangle area, BootSettings *cfg)
{
    UiTextBold("Hình nền", (Vector2){area.x, area.y}, UI_FS_H3, UI.textPrimary);

    int count = HubWallpaperCount();
    int skipped = HubWallpaperSkippedCount();

    char hint[128];
    if (skipped > 0) {
        // Nói thẳng khi có file bị loại, thay vì để người dùng tự hỏi
        // sao ảnh mình vừa thả vào lại không xuất hiện.
        snprintf(hint, sizeof(hint), "%d lựa chọn · %d file không đọc được", count, skipped);
    } else {
        snprintf(hint, sizeof(hint), "%d lựa chọn · thả thêm ảnh vào assets/wallpapers", count);
    }
    float hintW = UiTextWidth(hint, UI_FS_SMALL, false);
    UiText(hint, (Vector2){area.x + area.width - hintW, area.y + 3.0f}, UI_FS_SMALL,
           (skipped > 0) ? UI.warning : UI.textMuted);

    float gridTop = area.y + 26.0f;
    float gridH = area.y + area.height - gridTop;
    if (gridH < 40.0f) return false;

    const int cols = 4;
    float gap = 10.0f;
    float tileW = (area.width - gap * (cols - 1)) / (float)cols;
    float tileH = tileW * 9.0f / 16.0f;

    // Dùng hai hàng nếu còn đủ chỗ, còn không thì một hàng.
    int maxRows = (gridH >= tileH * 2.0f + gap) ? 2 : 1;
    int visible = cols * maxRows;
    if (visible > count) visible = count;

    bool changed = false;
    for (int i = 0; i < visible; i++) {
        Rectangle tile = {area.x + (i % cols) * (tileW + gap),
                          gridTop + (i / cols) * (tileH + gap), tileW, tileH};

        bool picked = DrawWallpaperTile(ctx, tile, i, i == HubWallpaperCurrent());

        if (picked && i != HubWallpaperCurrent()) {
            HubWallpaperSelect(i);
            snprintf(cfg->wallpaperId, sizeof(cfg->wallpaperId), "%s",
                     HubWallpaperId(HubWallpaperCurrent()));
            changed = true;
        }
    }

    if (count > visible) {
        char more[48];
        snprintf(more, sizeof(more), "+%d ảnh nữa trong thư mục", count - visible);
        UiText(more, (Vector2){area.x, gridTop + maxRows * (tileH + gap) + 2.0f},
               UI_FS_TINY, UI.textMuted);
    }
    return changed;
}

// Một dòng cài đặt: icon + nhãn + mô tả bên trái, widget điều khiển bên phải.
//
// Cả hàng là MỘT điểm dừng bàn phím chứ không phải riêng ô điều khiển nhỏ
// bên trong - nhờ vậy vòng focus to và rõ như thẻ game trong Thư viện, và
// Lên/Xuống nhảy đúng từng hàng.
typedef struct {
    Rectangle row;    // Toàn bộ hàng
    Rectangle slot;   // Vùng bên phải dành cho widget điều khiển
    int focus;        // Id focus dùng chung cho cả hàng
} SettingRowLayout;

static SettingRowLayout SettingRow(Rectangle area, float y, float height,
                                   UiIconFn icon, const char *label, const char *desc, Color accent)
{
    Rectangle row = {area.x + UI_PAD_LG, y, area.width - UI_PAD_LG * 2.0f, height};

    int focus = UiFocusRegister(row, UI_FOCUS_CONSUMES_H);
    bool focused = UiFocusIsFocused(focus);

    UiGlassStyle style = UiGlassStyleRaised();
    style.alpha = focused ? 0.70f : 0.42f;
    if (focused) style.targetLum += 0.06f;
    UiGlassPanel(row, UI_RADIUS_MD, style);
    UiGlassStroke(row, UI_RADIUS_MD, 1.0f,
                  focused ? UI.strokeStrong : UiAlpha(UI.strokeSoft, 0.6f));
    if (UiFocusRingVisible(focus)) UiFocusDrawRing(row, UI_RADIUS_MD, accent);

    if (icon) icon((Rectangle){row.x + UI_PAD_MD, row.y + row.height * 0.5f - 10.0f, 20.0f, 20.0f}, accent);

    float tx = row.x + UI_PAD_MD + 20.0f + UI_PAD_SM;
    UiTextBold(label, (Vector2){tx, row.y + 14.0f}, UI_FS_BODY, UI.textPrimary);
    UiText(desc, (Vector2){tx, row.y + 34.0f}, UI_FS_SMALL, UI.textMuted);

    return (SettingRowLayout){
        .row = row,
        .slot = (Rectangle){row.x + row.width - 200.0f - UI_PAD_MD, row.y, 200.0f, row.height},
        .focus = focus
    };
}

// Dòng cài đặt dạng thanh trượt: slider + số phần trăm canh phải.
// Trả về true nếu giá trị vừa thay đổi.
static bool SettingSlider(SettingRowLayout layout, float *value, Color accent)
{
    Rectangle slot = layout.slot;
    float before = *value;

    float pctW = 42.0f;
    Rectangle slider = {slot.x + 16.0f, slot.y + (slot.height - 28.0f) * 0.5f,
                        slot.width - 16.0f - pctW - UI_PAD_SM, 28.0f};
    UiSliderEx(slider, value, accent, layout.focus);

    char pct[16];
    snprintf(pct, sizeof(pct), "%d%%", (int)(*value * 100.0f + 0.5f));
    float w = UiTextWidth(pct, UI_FS_BODY, true);
    UiTextBold(pct, (Vector2){slider.x + slider.width + UI_PAD_SM + (pctW - w),
                              slot.y + (slot.height - UI_FS_BODY) * 0.5f - 1.0f},
               UI_FS_BODY, UI.textPrimary);

    return fabsf(before - *value) > 0.001f;
}

void HubSettingsPageDraw(HubContext *ctx, Rectangle area)
{
    BootSettings *cfg = BootSettingsGet();

    float gap = 18.0f;
    float leftW = (area.width - gap) * 0.62f;

    Rectangle panel = {area.x, area.y, leftW, area.height};
    UiGlassShadow(panel, UI_RADIUS_LG, 26.0f, 0.22f);
    UiGlassPanel(panel, UI_RADIUS_LG, UiGlassStylePanel());
    UiGlassStroke(panel, UI_RADIUS_LG, 1.2f, UI.strokeSoft);

    UiTextBold("Cài đặt", (Vector2){panel.x + UI_PAD_LG, panel.y + UI_PAD_LG}, UI_FS_H2, UI.textPrimary);
    UiText("Thay đổi được lưu ngay và áp dụng cho lần chạy sau",
           (Vector2){panel.x + UI_PAD_LG, panel.y + UI_PAD_LG + 30.0f}, UI_FS_SMALL, UI.textMuted);

    float rowH = 56.0f;
    float rowGap = 8.0f;
    float y = panel.y + UI_PAD_LG + 62.0f;
    bool changed = false;

    // 1. Độ mờ hậu cảnh của lớp kính
    {
        const char *desc = UiGlassIsLite() ? "Không dùng ở đồ hoạ Nhẹ (xem cột Hệ thống)"
                                           : "Kéo về trái để kính trong, về phải để mờ sâu";
        SettingRowLayout r = SettingRow(panel, y, rowH, UiIconSparkle, "Độ mờ hậu cảnh",
                                        desc, ctx->accent);
        if (SettingSlider(r, &cfg->glassBlur, ctx->accent)) {
            UiGlassSetBlurAmount(cfg->glassBlur);
            changed = true;
        }
        y += rowH + rowGap;
    }

    // 2. Độ đục của mặt kính
    {
        SettingRowLayout r = SettingRow(panel, y, rowH, UiIconDisplay, "Độ đục mặt kính",
                                        "Lượng màu phủ lên kính; thấp thì thấy nền rõ hơn", ctx->accent);
        if (SettingSlider(r, &cfg->glassTint, ctx->accent)) {
            UiGlassSetLevelScale(GlassLevelScale(cfg->glassTint));
            changed = true;
        }
        y += rowH + rowGap;
    }

    // 3. Âm lượng chung
    {
        SettingRowLayout r = SettingRow(panel, y, rowH, UiIconSpeaker, "Âm lượng chung",
                                        "Áp dụng cho toàn bộ game trong thư viện", ctx->accent);
        if (SettingSlider(r, &cfg->masterVolume, ctx->accent)) {
            BootSettingsApply();
            changed = true;
        }
        y += rowH + rowGap;
    }

    // 4. Hiển thị FPS
    {
        SettingRowLayout r = SettingRow(panel, y, rowH, UiIconCounter, "Hiện chỉ số FPS",
                                        "Hiện số khung hình ở Hub và khi chơi (F3)", ctx->accent);
        Rectangle tog = {r.slot.x + r.slot.width - 58.0f, r.slot.y + (r.slot.height - 30.0f) * 0.5f, 54.0f, 30.0f};
        if (UiToggleEx(tog, cfg->showFps, ctx->accent, r.focus).clicked) {
            cfg->showFps = !cfg->showFps;
            changed = true;
        }
        y += rowH + rowGap;
    }

    // 5. Giảm chuyển động
    {
        SettingRowLayout r = SettingRow(panel, y, rowH, UiIconSliders, "Giảm chuyển động",
                                        "Tắt hiệu ứng trôi nền và trượt mượt", ctx->accent);
        Rectangle tog = {r.slot.x + r.slot.width - 58.0f, r.slot.y + (r.slot.height - 30.0f) * 0.5f, 54.0f, 30.0f};
        if (UiToggleEx(tog, cfg->reduceMotion, ctx->accent, r.focus).clicked) {
            cfg->reduceMotion = !cfg->reduceMotion;
            changed = true;
        }
        y += rowH + rowGap;
    }

    // Lưới chọn hình nền chiếm phần còn lại của cột trái.
    Rectangle wallArea = {panel.x + UI_PAD_LG, y + 6.0f,
                          panel.width - UI_PAD_LG * 2.0f,
                          panel.y + panel.height - UI_PAD_LG - (y + 6.0f)};
    if (DrawWallpaperPicker(ctx, wallArea, cfg)) changed = true;

    // Kéo thanh trượt sinh thay đổi ở mọi khung hình, nên chỉ ghi file khi
    // người dùng đã thả chuột thay vì ghi đĩa 60 lần mỗi giây.
    static bool pendingSave = false;
    if (changed) pendingSave = true;
    if (pendingSave && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        BootSettingsSave();
        pendingSave = false;
    }

    // --- Cột phải: thông tin hệ thống và nút xoá dữ liệu
    Rectangle side = {area.x + leftW + gap, area.y, area.width - leftW - gap, area.height};
    UiGlassShadow(side, UI_RADIUS_LG, 26.0f, 0.22f);
    UiGlassPanel(side, UI_RADIUS_LG, UiGlassStylePanel());
    UiGlassStroke(side, UI_RADIUS_LG, 1.2f, UI.strokeSoft);

    UiTextBold("Hệ thống", (Vector2){side.x + UI_PAD_LG, side.y + UI_PAD_LG}, UI_FS_H2, UI.textPrimary);

    char resolution[32];
    snprintf(resolution, sizeof(resolution), "%d × %d", GetScreenWidth(), GetScreenHeight());

    char fps[16];
    snprintf(fps, sizeof(fps), "%d", GetFPS());

    char monitor[24];
    snprintf(monitor, sizeof(monitor), "%d Hz", GetMonitorRefreshRate(GetCurrentMonitor()));

    struct { UiIconFn icon; const char *label; const char *value; } info[4] = {
        {UiIconDisplay, "Cửa sổ",       resolution},
        {UiIconCounter, "FPS hiện tại", fps},
        {UiIconClock,   "Tần số quét",  monitor},
        {UiIconSparkle, "Shader kính",  UiGlassIsShaderReady() ? "Bật" : (UiGlassIsLite() ? "Tắt (Nhẹ)" : "Dự phòng")}
    };

    float iy = side.y + UI_PAD_LG + 42.0f;
    for (int i = 0; i < 4; i++) {
        UiStatLine((Rectangle){side.x + UI_PAD_LG, iy, side.width - UI_PAD_LG * 2.0f, 26.0f},
                   info[i].icon, info[i].label, info[i].value);
        iy += 26.0f;
        if (i < 3) UiSeparator((Vector2){side.x + UI_PAD_LG, iy - 3.0f}, side.width - UI_PAD_LG * 2.0f, false);
    }

    // Khối giới thiệu: nói rõ dữ liệu người dùng được lưu ở đâu.
    Rectangle about = {side.x + UI_PAD_LG, iy + UI_PAD_LG,
                       side.width - UI_PAD_LG * 2.0f, 104.0f};
    UiGlassPanel(about, UI_RADIUS_MD, UiGlassStyleSunken());
    UiGlassStroke(about, UI_RADIUS_MD, 1.0f, UiAlpha(UI.strokeSoft, 0.7f));

    UiIconGamepad((Rectangle){about.x + UI_PAD_MD, about.y + UI_PAD_MD, 20.0f, 20.0f}, ctx->accent);
    UiTextBold("Arcade Hub", (Vector2){about.x + UI_PAD_MD + 28.0f, about.y + UI_PAD_MD + 2.0f},
               UI_FS_BODY, UI.textPrimary);

    char line[128];
    snprintf(line, sizeof(line), "%d game · Raylib %s", GameRegistryCount(), RAYLIB_VERSION);
    UiText(line, (Vector2){about.x + UI_PAD_MD, about.y + UI_PAD_MD + 32.0f}, UI_FS_SMALL, UI.textSecondary);
    // Kỷ lục của từng game, thống kê và cài đặt của Hub đều nằm chung một thư mục.
    UiText("Dữ liệu (kỷ lục, thống kê, cài đặt):",
           (Vector2){about.x + UI_PAD_MD, about.y + UI_PAD_MD + 52.0f}, UI_FS_SMALL, UI.textMuted);
    char dir[512];
    const char *home = getenv("HOME");
    const char *dataDir = SaveDataDir();
    size_t homeLen = home ? strlen(home) : 0;
    if (homeLen > 0 && strncmp(dataDir, home, homeLen) == 0) snprintf(dir, sizeof(dir), "~%s", dataDir + homeLen);
    else                                                    snprintf(dir, sizeof(dir), "%s", dataDir);
    // Đường dẫn quá dài (vd. ghi đè bằng RAYLIB_ARCADE_DATA): giữ phần cuối, thêm "…" ở đầu.
    const char *shown = dir;
    float maxW = about.width - UI_PAD_MD * 2.0f;
    while (shown[0] && UiTextWidth(TextFormat("…%s", shown), UI_FS_SMALL, false) > maxW) {
        shown++;
        while (((unsigned char)shown[0] & 0xC0) == 0x80) shown++;   // Không cắt giữa ký tự UTF-8
    }
    if (shown != dir) shown = TextFormat("…%s", shown);
    UiText(shown, (Vector2){about.x + UI_PAD_MD, about.y + UI_PAD_MD + 70.0f}, UI_FS_SMALL, UI.textSecondary);

    // Chất lượng đồ hoạ: hiệu ứng kính quá nặng cho GPU của Raspberry Pi 0-3.
    // Áp dụng cho cả Hub lẫn Caro / Cờ Vua (qua PerfHintLite).
    float gy = about.y + about.height + UI_PAD_LG;
    UiTextBold("Chất lượng đồ hoạ", (Vector2){side.x + UI_PAD_LG, gy}, UI_FS_BODY, UI.textPrimary);

    const char *gfxHint;
    if (cfg->graphicsMode == BOOT_GFX_QUALITY)   gfxHint = "Luôn bật hiệu ứng kính";
    else if (cfg->graphicsMode == BOOT_GFX_LITE) gfxHint = "Giảm hiệu ứng ở Hub, Caro, Cờ Vua";
    else if (BootPerfLite())                     gfxHint = TextFormat("Đang dùng Nhẹ: %s", BootPerfReason());
    else                                         gfxHint = "Đang dùng Đẹp";
    UiText(gfxHint, (Vector2){side.x + UI_PAD_LG, gy + 22.0f}, UI_FS_SMALL, UI.textMuted);

    static const char *const GFX_LABELS[BOOT_GFX_COUNT] = {"Tự động", "Đẹp", "Nhẹ"};
    Rectangle seg = {side.x + UI_PAD_LG, gy + 46.0f, side.width - UI_PAD_LG * 2.0f, 34.0f};
    if (UiSegmented(seg, GFX_LABELS, BOOT_GFX_COUNT, &cfg->graphicsMode, ctx->accent).clicked) {
        BootPerfResetMeasure();
        BootSettingsSave();
    }

    // Xoá toàn bộ thống kê: hành động không hoàn tác nên cần bấm xác nhận hai bước.
    static bool confirming = false;
    Rectangle btn = {side.x + UI_PAD_LG, side.y + side.height - 56.0f - UI_PAD_LG,
                     side.width - UI_PAD_LG * 2.0f, 46.0f};

    const char *label = confirming ? "Bấm lần nữa để xác nhận" : "Xoá toàn bộ thống kê & kỷ lục";
    if (UiGlassButton(btn, label, confirming ? UI.danger : ctx->accent, confirming, true).clicked) {
        if (confirming) {
            BootStatsResetAll();
            confirming = false;
        } else {
            confirming = true;
        }
    }
    // Rời chuột khỏi nút thì huỷ trạng thái chờ xác nhận.
    if (confirming && !CheckCollisionPointRec(UiMouse(), btn)) confirming = false;
}
