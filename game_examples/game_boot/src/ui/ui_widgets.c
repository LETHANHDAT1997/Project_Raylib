#include "ui_widgets.h"
#include "ui_theme.h"
#include "ui_anim.h"
#include "ui_focus.h"
#include "font_vn.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

// ------------------------------------------------------------- Tương tác

Vector2 UiMouse(void)
{
    // Canvas ảo đã gọi SetMouseOffset/SetMouseScale nên GetMousePosition()
    // trả về thẳng toạ độ trong hệ 1280x720.
    return GetMousePosition();
}

UiInteraction UiHitTest(Rectangle rec)
{
    UiInteraction it = {0};
    it.hovered = CheckCollisionPointRec(UiMouse(), rec);
    if (it.hovered) {
        it.pressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        it.clicked = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    }
    return it;
}

// ------------------------------------------------------------------ Chữ

void UiText(const char *text, Vector2 pos, float size, Color color)
{
    DrawTextVNPro(text, pos, size, UI_TRACK_NORMAL, color);
}

void UiTextBold(const char *text, Vector2 pos, float size, Color color)
{
    DrawTextVNBoldPro(text, pos, size, UI_TRACK_NORMAL, color);
}

void UiTextTracked(const char *text, Vector2 pos, float size, float tracking, Color color, bool bold)
{
    if (bold) DrawTextVNBoldPro(text, pos, size, tracking, color);
    else      DrawTextVNPro(text, pos, size, tracking, color);
}

float UiTextWidth(const char *text, float size, bool bold)
{
    if (bold) return MeasureTextVNBoldPro(text, size, UI_TRACK_NORMAL).x;
    return MeasureTextVNPro(text, size, UI_TRACK_NORMAL).x;
}

const char *UiTextEllipsis(const char *text, float size, bool bold, float maxWidth)
{
    static char buffer[256];

    if (!text) return "";
    if (UiTextWidth(text, size, bold) <= maxWidth) return text;

    size_t len = strlen(text);
    if (len >= sizeof(buffer) - 4) len = sizeof(buffer) - 4;

    // Cắt dần từ cuối, tôn trọng ranh giới ký tự UTF-8 để không vỡ dấu tiếng Việt.
    for (size_t cut = len; cut > 0; cut--) {
        if (((unsigned char)text[cut] & 0xC0) == 0x80) continue;  // byte nối của UTF-8
        memcpy(buffer, text, cut);
        buffer[cut] = '\0';
        strncat(buffer, "…", sizeof(buffer) - cut - 1);
        if (UiTextWidth(buffer, size, bold) <= maxWidth) return buffer;
    }
    return "…";
}

// Ngắt dòng theo từ, vẽ trong khung và trả về tổng chiều cao đã dùng.
float UiTextBox(const char *text, Rectangle box, float size, float lineGap,
                Color color, UiAlign align, bool bold)
{
    if (!text || !text[0]) return 0.0f;

    char line[512] = {0};
    char word[128] = {0};
    float y = box.y;
    int lineLen = 0;
    const char *p = text;

    while (1) {
        // Gom một từ (tính cả dấu cách đứng trước).
        int wlen = 0;
        while (*p == ' ') {
            if (wlen < (int)sizeof(word) - 1) word[wlen++] = *p;
            p++;
        }
        while (*p && *p != ' ' && *p != '\n') {
            if (wlen < (int)sizeof(word) - 1) word[wlen++] = *p;
            p++;
        }
        word[wlen] = '\0';

        bool forceBreak = (*p == '\n');
        bool done = (*p == '\0');

        if (wlen > 0) {
            char candidate[512];
            snprintf(candidate, sizeof(candidate), "%s%s", line, word);
            if (lineLen > 0 && UiTextWidth(candidate, size, bold) > box.width) {
                // Từ này tràn: xuống dòng rồi bắt đầu lại bằng chính nó.
                float w = UiTextWidth(line, size, bold);
                float x = box.x;
                if (align == UI_ALIGN_CENTER) x = box.x + (box.width - w) * 0.5f;
                else if (align == UI_ALIGN_RIGHT) x = box.x + box.width - w;
                UiTextTracked(line, (Vector2){x, y}, size, UI_TRACK_NORMAL, color, bold);
                y += size + lineGap;

                const char *trimmed = word;
                while (*trimmed == ' ') trimmed++;
                snprintf(line, sizeof(line), "%s", trimmed);
            } else {
                snprintf(line, sizeof(line), "%s", candidate);
            }
            lineLen = (int)strlen(line);
        }

        if (forceBreak || done) {
            if (lineLen > 0) {
                float w = UiTextWidth(line, size, bold);
                float x = box.x;
                if (align == UI_ALIGN_CENTER) x = box.x + (box.width - w) * 0.5f;
                else if (align == UI_ALIGN_RIGHT) x = box.x + box.width - w;
                UiTextTracked(line, (Vector2){x, y}, size, UI_TRACK_NORMAL, color, bold);
                y += size + lineGap;
            }
            line[0] = '\0';
            lineLen = 0;
            if (done) break;
            p++;  // bỏ qua '\n'
        }
    }
    return y - box.y;
}

// ------------------------------------------------------------ Thành phần

UiInteraction UiGlassButton(Rectangle rec, const char *label, Color accent, bool primary, bool enabled)
{
    UiInteraction it = enabled ? UiHitTest(rec) : (UiInteraction){0};

    int focus = enabled ? UiFocusRegister(rec, UI_FOCUS_FLAG_NONE) : -1;
    if (UiFocusActivated(focus)) it.clicked = true;
    if (UiFocusIsFocused(focus)) it.hovered = true;

    // Nhấn xuống làm nút thu nhỏ nhẹ, tạo cảm giác vật lý.
    Rectangle draw = rec;
    if (it.pressed) {
        draw.x += 1.0f; draw.y += 1.0f;
        draw.width -= 2.0f; draw.height -= 2.0f;
    }

    float radius = draw.height * 0.5f;
    UiGlassStyle style;

    if (primary) {
        style = UiGlassStyleAccent(accent);
        if (it.hovered) style.targetLum += 0.05f;
        if (!enabled) style.alpha = 0.45f;
        UiGlassShadow(draw, radius, 18.0f, 0.30f);
        if (it.hovered) UiGlassGlow(draw, radius, 26.0f, UiAlpha(accent, 0.34f));
    } else {
        style = UiGlassStyleRaised();
        if (it.hovered) style.targetLum += 0.06f;
        if (!enabled) style.alpha = 0.4f;
    }

    UiGlassPanel(draw, radius, style);
    UiGlassStroke(draw, radius, 1.2f, it.hovered ? UI.strokeHighlight : UI.strokeSoft);
    if (UiFocusRingVisible(focus)) UiFocusDrawRing(draw, radius, accent);

    Color textColor = primary ? UI.textOnAccent : UI.textPrimary;
    if (!enabled) textColor = UiAlpha(textColor, 0.5f);

    float fs = UI_FS_H3;
    float w = UiTextWidth(label, fs, true);
    UiTextBold(label, (Vector2){draw.x + (draw.width - w) * 0.5f,
                                draw.y + (draw.height - fs) * 0.5f - 1.0f}, fs, textColor);
    return it;
}

UiInteraction UiIconButton(Rectangle rec, void (*drawIcon)(Rectangle area, Color tint),
                           Color accent, bool active)
{
    UiInteraction it = UiHitTest(rec);
    float radius = fminf(rec.width, rec.height) * 0.5f;

    int focus = UiFocusRegister(rec, UI_FOCUS_FLAG_NONE);
    if (UiFocusActivated(focus)) it.clicked = true;
    if (UiFocusIsFocused(focus)) it.hovered = true;

    UiGlassStyle style = active ? UiGlassStyleAccent(accent) : UiGlassStyleRaised();
    if (it.hovered) style.targetLum += 0.06f;
    if (it.pressed) style.targetLum -= 0.04f;

    UiGlassPanel(rec, radius, style);
    UiGlassStroke(rec, radius, 1.1f, it.hovered ? UI.strokeHighlight : UI.strokeSoft);
    if (UiFocusRingVisible(focus)) UiFocusDrawRing(rec, radius, accent);

    if (drawIcon) {
        float pad = rec.height * 0.30f;
        drawIcon((Rectangle){rec.x + pad, rec.y + pad, rec.width - pad * 2.0f, rec.height - pad * 2.0f},
                 active ? UI.textOnAccent : UI.textPrimary);
    }
    return it;
}

void UiChip(Rectangle rec, const char *label, Color accent, bool solid)
{
    float radius = rec.height * 0.5f;

    UiGlassStyle style = solid ? UiGlassStyleAccent(accent) : UiGlassStyleRaised();
    style.alpha = solid ? 0.95f : 0.82f;

    UiGlassPanel(rec, radius, style);
    UiGlassStroke(rec, radius, 1.0f, solid ? UiAlpha(WHITE, 0.55f) : UI.strokeSoft);

    float fs = UI_FS_SMALL;
    float w = UiTextWidth(label, fs, false);
    UiText(label, (Vector2){rec.x + (rec.width - w) * 0.5f, rec.y + (rec.height - fs) * 0.5f - 1.0f},
           fs, solid ? UI.textOnAccent : UI.textPrimary);
}

float UiChipRow(Vector2 origin, const char *const *labels, int count, float height, float gap, Color accent)
{
    float x = origin.x;
    for (int i = 0; i < count; i++) {
        float w = UiTextWidth(labels[i], UI_FS_SMALL, false) + UI_PAD_MD * 2.0f;
        UiChip((Rectangle){x, origin.y, w, height}, labels[i], accent, false);
        x += w + gap;
    }
    return x - origin.x;
}

void UiProgressBar(Rectangle rec, float value01, Color accent)
{
    if (value01 < 0.0f) value01 = 0.0f;
    if (value01 > 1.0f) value01 = 1.0f;

    float radius = rec.height * 0.5f;
    UiGlassPanel(rec, radius, UiGlassStyleSunken());

    float fillW = rec.width * value01;
    if (fillW > rec.height * 0.6f) {
        Rectangle fill = {rec.x, rec.y, fillW, rec.height};
        UiGlassStyle style = UiGlassStyleAccent(accent);
        style.tintStrength = 0.88f;
        UiGlassPanel(fill, radius, style);
        UiGlassStroke(fill, radius, 1.0f, UiAlpha(WHITE, 0.4f));
    }
    UiGlassStroke(rec, radius, 1.0f, UI.strokeSoft);
}

void UiStatLine(Rectangle rec, void (*drawIcon)(Rectangle area, Color tint),
                const char *label, const char *value)
{
    float iconSize = 15.0f;
    float cy = rec.y + (rec.height - iconSize) * 0.5f;

    if (drawIcon) {
        drawIcon((Rectangle){rec.x, cy, iconSize, iconSize}, UiAlpha(UI.textSecondary, 0.85f));
    }

    float textY = rec.y + (rec.height - UI_FS_BODY) * 0.5f - 0.5f;
    UiText(label, (Vector2){rec.x + iconSize + UI_PAD_SM, textY}, UI_FS_BODY, UI.textSecondary);

    float vw = UiTextWidth(value, UI_FS_BODY, true);
    UiTextBold(value, (Vector2){rec.x + rec.width - vw, textY}, UI_FS_BODY, UI.textPrimary);
}

void UiSeparator(Vector2 start, float length, bool vertical)
{
    Vector2 end = vertical ? (Vector2){start.x, start.y + length}
                           : (Vector2){start.x + length, start.y};
    DrawLineEx(start, end, 1.0f, UiAlpha(UI.strokeSoft, 0.7f));
}

UiInteraction UiToggle(Rectangle rec, bool value, Color accent)
{
    return UiToggleEx(rec, value, accent, UI_FOCUS_AUTO);
}

UiInteraction UiToggleEx(Rectangle rec, bool value, Color accent, int focusId)
{
    UiInteraction it = UiHitTest(rec);
    float radius = rec.height * 0.5f;

    // Công tắc nhận cả Trái/Phải: sang phải là bật, sang trái là tắt.
    int focus = (focusId == UI_FOCUS_AUTO) ? UiFocusRegister(rec, UI_FOCUS_CONSUMES_H) : focusId;
    int axis = UiFocusAxisH(focus);
    if (UiFocusActivated(focus)) it.clicked = true;
    if ((axis > 0 && !value) || (axis < 0 && value)) it.clicked = true;
    if (UiFocusIsFocused(focus)) it.hovered = true;

    UiGlassStyle style = value ? UiGlassStyleAccent(accent) : UiGlassStyleSunken();
    if (it.hovered) style.targetLum += 0.05f;
    UiGlassPanel(rec, radius, style);
    UiGlassStroke(rec, radius, 1.1f, it.hovered ? UI.strokeHighlight : UI.strokeSoft);
    if (focusId == UI_FOCUS_AUTO && UiFocusRingVisible(focus)) UiFocusDrawRing(rec, radius, accent);

    float knobR = rec.height * 0.5f - 3.0f;
    float knobX = value ? (rec.x + rec.width - knobR - 3.0f) : (rec.x + knobR + 3.0f);
    DrawCircleV((Vector2){knobX, rec.y + rec.height * 0.5f + 1.0f}, knobR, UiAlpha(BLACK, 0.20f));
    DrawCircleV((Vector2){knobX, rec.y + rec.height * 0.5f}, knobR, (Color){250, 252, 255, 250});
    return it;
}

UiInteraction UiSlider(Rectangle rec, float *value01, Color accent)
{
    return UiSliderEx(rec, value01, accent, UI_FOCUS_AUTO);
}

UiInteraction UiSliderEx(Rectangle rec, float *value01, Color accent, int focusId)
{
    UiInteraction it = UiHitTest(rec);

    // Kéo thả: giữ chuột trong vùng là cập nhật liên tục.
    if (it.pressed && rec.width > 0.0f) {
        float t = (UiMouse().x - rec.x) / rec.width;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        *value01 = t;
    }

    // Bàn phím: Trái/Phải đổi giá trị theo bước 5%, giữ phím thì chạy đều
    // nhờ cơ chế lặp của boot_input.
    int focus = (focusId == UI_FOCUS_AUTO) ? UiFocusRegister(rec, UI_FOCUS_CONSUMES_H) : focusId;
    int axis = UiFocusAxisH(focus);
    if (axis != 0) {
        *value01 += (float)axis * 0.05f;
        if (*value01 < 0.0f) *value01 = 0.0f;
        if (*value01 > 1.0f) *value01 = 1.0f;
    }
    if (UiFocusIsFocused(focus)) it.hovered = true;

    float trackH = 8.0f;
    Rectangle track = {rec.x, rec.y + (rec.height - trackH) * 0.5f, rec.width, trackH};
    UiProgressBar(track, *value01, accent);

    if (focusId == UI_FOCUS_AUTO && UiFocusRingVisible(focus)) UiFocusDrawRing(track, track.height * 0.5f, accent);

    float knobX = rec.x + rec.width * (*value01);
    float knobR = rec.height * 0.32f;
    DrawCircleV((Vector2){knobX, rec.y + rec.height * 0.5f + 1.0f}, knobR, UiAlpha(BLACK, 0.22f));
    DrawCircleV((Vector2){knobX, rec.y + rec.height * 0.5f}, knobR, (Color){250, 252, 255, 250});
    return it;
}

UiInteraction UiSegmented(Rectangle rec, const char *const *labels, int count, int *index, Color accent)
{
    UiInteraction result = {0};
    if (count <= 0) return result;

    float radius = rec.height * 0.5f;

    // Cả nhóm là một điểm dừng focus; Trái/Phải đổi lựa chọn bên trong.
    int focus = UiFocusRegister(rec, UI_FOCUS_CONSUMES_H);
    int axis = UiFocusAxisH(focus);
    if (axis != 0) {
        *index = (*index + axis + count) % count;
        result.clicked = true;
    }

    UiGlassPanel(rec, radius, UiGlassStyleSunken());
    UiGlassStroke(rec, radius, 1.0f, UI.strokeSoft);
    if (UiFocusRingVisible(focus)) UiFocusDrawRing(rec, radius, accent);

    float segW = rec.width / (float)count;
    for (int i = 0; i < count; i++) {
        Rectangle seg = {rec.x + i * segW, rec.y, segW, rec.height};
        UiInteraction it = UiHitTest(seg);
        if (it.clicked) {
            *index = i;
            result.clicked = true;
        }
        if (it.hovered) result.hovered = true;

        bool active = (*index == i);
        if (active) {
            Rectangle pill = {seg.x + 3.0f, seg.y + 3.0f, seg.width - 6.0f, seg.height - 6.0f};
            UiGlassStyle style = UiGlassStyleAccent(accent);
            style.tintStrength = 0.80f;
            UiGlassPanel(pill, pill.height * 0.5f, style);
            UiGlassStroke(pill, pill.height * 0.5f, 1.0f, UiAlpha(WHITE, 0.45f));
        }

        float fs = UI_FS_SMALL;
        float w = UiTextWidth(labels[i], fs, active);
        Color col = active ? UI.textOnAccent : (it.hovered ? UI.textPrimary : UI.textSecondary);
        UiTextTracked(labels[i], (Vector2){seg.x + (segW - w) * 0.5f, seg.y + (seg.height - fs) * 0.5f - 1.0f},
                      fs, UI_TRACK_NORMAL, col, active);
    }
    return result;
}
