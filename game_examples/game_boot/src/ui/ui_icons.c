#include "ui_icons.h"
#include "ui_theme.h"
#include <math.h>

// Mọi icon được dựng trong hệ toạ độ vuông đã chuẩn hoá rồi ánh xạ ra `area`,
// nên chỉ cần chỉnh một chỗ là icon co giãn đúng ở mọi kích thước.
typedef struct {
    Vector2 origin;
    float size;
} IconFrame;

static IconFrame Frame(Rectangle area)
{
    float s = fminf(area.width, area.height);
    return (IconFrame){
        .origin = {area.x + (area.width - s) * 0.5f, area.y + (area.height - s) * 0.5f},
        .size = s
    };
}

// Chuyển toạ độ 0..1 sang pixel trong khung icon.
static Vector2 P(IconFrame f, float x, float y)
{
    return (Vector2){f.origin.x + x * f.size, f.origin.y + y * f.size};
}

static float S(IconFrame f, float v)
{
    return v * f.size;
}

// Màu cho chi tiết nằm TRÊN một mảng đặc cùng màu: tự chọn sáng hay tối
// dựa theo độ sáng của màu nền để luôn nhìn thấy được.
static Color Contrast(Color base)
{
    float lum = (0.299f * base.r + 0.587f * base.g + 0.114f * base.b) / 255.0f;
    return (lum > 0.55f) ? (Color){12, 22, 42, base.a} : (Color){245, 250, 255, base.a};
}

static void Stroke(IconFrame f, float x1, float y1, float x2, float y2, float w, Color c)
{
    DrawLineEx(P(f, x1, y1), P(f, x2, y2), S(f, w), c);
}

static void RoundRect(IconFrame f, float x, float y, float w, float h, float r, float thick, Color c)
{
    Rectangle rec = {P(f, x, y).x, P(f, x, y).y, S(f, w), S(f, h)};
    float roundness = (S(f, r) * 2.0f) / fminf(rec.width, rec.height);
    if (roundness > 1.0f) roundness = 1.0f;
    if (thick <= 0.0f) DrawRectangleRounded(rec, roundness, 8, c);
    else DrawRectangleRoundedLinesEx(rec, roundness, 8, S(f, thick), c);
}

void UiIconGamepad(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    Color detail = UiAlpha(Contrast(tint), 0.9f);

    RoundRect(f, 0.06f, 0.28f, 0.88f, 0.44f, 0.20f, 0.0f, tint);
    // Phím điều hướng
    DrawRectangleRec((Rectangle){P(f, 0.18f, 0.46f).x, P(f, 0.18f, 0.46f).y, S(f, 0.21f), S(f, 0.08f)}, detail);
    DrawRectangleRec((Rectangle){P(f, 0.245f, 0.395f).x, P(f, 0.245f, 0.395f).y, S(f, 0.08f), S(f, 0.21f)}, detail);
    // Hai nút bấm
    DrawCircleV(P(f, 0.68f, 0.43f), S(f, 0.062f), detail);
    DrawCircleV(P(f, 0.80f, 0.57f), S(f, 0.062f), detail);
}

void UiIconPlay(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    Vector2 pts[3] = {P(f, 0.28f, 0.18f), P(f, 0.28f, 0.82f), P(f, 0.82f, 0.50f)};
    DrawTriangle(pts[0], pts[1], pts[2], tint);
}

void UiIconEllipsis(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    float r = S(f, 0.075f);
    DrawCircleV(P(f, 0.24f, 0.5f), r, tint);
    DrawCircleV(P(f, 0.50f, 0.5f), r, tint);
    DrawCircleV(P(f, 0.76f, 0.5f), r, tint);
}

void UiIconChevronRight(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    Stroke(f, 0.40f, 0.24f, 0.64f, 0.50f, 0.085f, tint);
    Stroke(f, 0.64f, 0.50f, 0.40f, 0.76f, 0.085f, tint);
}

void UiIconClock(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    DrawRing(P(f, 0.5f, 0.5f), S(f, 0.36f), S(f, 0.44f), 0.0f, 360.0f, 32, tint);
    Stroke(f, 0.50f, 0.50f, 0.50f, 0.28f, 0.07f, tint);
    Stroke(f, 0.50f, 0.50f, 0.68f, 0.56f, 0.07f, tint);
}

void UiIconCalendar(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    RoundRect(f, 0.14f, 0.22f, 0.72f, 0.64f, 0.12f, 0.07f, tint);
    Stroke(f, 0.14f, 0.40f, 0.86f, 0.40f, 0.07f, tint);
    Stroke(f, 0.32f, 0.12f, 0.32f, 0.28f, 0.08f, tint);
    Stroke(f, 0.68f, 0.12f, 0.68f, 0.28f, 0.08f, tint);
}

void UiIconPerson(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    DrawCircleV(P(f, 0.5f, 0.32f), S(f, 0.17f), tint);
    // Thân người: nửa vòng tròn cắt bởi khung icon
    DrawCircleSector(P(f, 0.5f, 0.92f), S(f, 0.33f), 180.0f, 360.0f, 24, tint);
}

void UiIconDisplay(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    RoundRect(f, 0.10f, 0.20f, 0.80f, 0.52f, 0.10f, 0.07f, tint);
    Stroke(f, 0.34f, 0.84f, 0.66f, 0.84f, 0.08f, tint);
    Stroke(f, 0.50f, 0.72f, 0.50f, 0.84f, 0.08f, tint);
}

void UiIconSparkle(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    Vector2 c = P(f, 0.5f, 0.5f);
    float big = S(f, 0.42f), small = S(f, 0.14f);
    Vector2 d[4] = {{c.x, c.y - big}, {c.x + small, c.y}, {c.x, c.y + big}, {c.x - small, c.y}};
    DrawTriangle(d[0], d[3], d[1], tint);
    DrawTriangle(d[1], d[3], d[2], tint);
    Vector2 h[4] = {{c.x - big, c.y}, {c.x, c.y - small}, {c.x + big, c.y}, {c.x, c.y + small}};
    DrawTriangle(h[0], h[1], h[3], tint);
    DrawTriangle(h[1], h[2], h[3], tint);
}

void UiIconKeyboard(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    RoundRect(f, 0.06f, 0.26f, 0.88f, 0.48f, 0.10f, 0.06f, tint);
    for (int i = 0; i < 4; i++) {
        float x = 0.17f + i * 0.175f;
        DrawRectangleRec((Rectangle){P(f, x, 0.40f).x, P(f, x, 0.40f).y, S(f, 0.09f), S(f, 0.07f)}, tint);
    }
    DrawRectangleRec((Rectangle){P(f, 0.27f, 0.56f).x, P(f, 0.27f, 0.56f).y, S(f, 0.46f), S(f, 0.07f)}, tint);
}

void UiIconSliders(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    Stroke(f, 0.12f, 0.30f, 0.88f, 0.30f, 0.07f, UiAlpha(tint, 0.55f));
    Stroke(f, 0.12f, 0.70f, 0.88f, 0.70f, 0.07f, UiAlpha(tint, 0.55f));
    DrawCircleV(P(f, 0.66f, 0.30f), S(f, 0.13f), tint);
    DrawCircleV(P(f, 0.34f, 0.70f), S(f, 0.13f), tint);
}

void UiIconLibrary(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    RoundRect(f, 0.10f, 0.16f, 0.22f, 0.68f, 0.05f, 0.0f, tint);
    RoundRect(f, 0.39f, 0.16f, 0.22f, 0.68f, 0.05f, 0.0f, UiAlpha(tint, 0.75f));
    RoundRect(f, 0.68f, 0.28f, 0.22f, 0.56f, 0.05f, 0.0f, UiAlpha(tint, 0.5f));
}

void UiIconSpeaker(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    Vector2 body[3] = {P(f, 0.46f, 0.22f), P(f, 0.46f, 0.78f), P(f, 0.20f, 0.60f)};
    DrawTriangle(body[0], body[2], body[1], tint);
    DrawRectangleRec((Rectangle){P(f, 0.16f, 0.40f).x, P(f, 0.16f, 0.40f).y, S(f, 0.16f), S(f, 0.20f)}, tint);
    DrawRing(P(f, 0.48f, 0.50f), S(f, 0.20f), S(f, 0.26f), -55.0f, 55.0f, 16, UiAlpha(tint, 0.85f));
    DrawRing(P(f, 0.48f, 0.50f), S(f, 0.33f), S(f, 0.39f), -55.0f, 55.0f, 16, UiAlpha(tint, 0.55f));
}

void UiIconExpand(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    float t = 0.075f;
    Stroke(f, 0.16f, 0.16f, 0.42f, 0.16f, t, tint);
    Stroke(f, 0.16f, 0.16f, 0.16f, 0.42f, t, tint);
    Stroke(f, 0.84f, 0.84f, 0.58f, 0.84f, t, tint);
    Stroke(f, 0.84f, 0.84f, 0.84f, 0.58f, t, tint);
}

void UiIconTrophy(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    RoundRect(f, 0.30f, 0.16f, 0.40f, 0.36f, 0.06f, 0.0f, tint);
    DrawCircleSector(P(f, 0.50f, 0.48f), S(f, 0.20f), 0.0f, 180.0f, 20, tint);
    DrawRing(P(f, 0.26f, 0.28f), S(f, 0.09f), S(f, 0.14f), 90.0f, 270.0f, 14, UiAlpha(tint, 0.8f));
    DrawRing(P(f, 0.74f, 0.28f), S(f, 0.09f), S(f, 0.14f), -90.0f, 90.0f, 14, UiAlpha(tint, 0.8f));
    DrawRectangleRec((Rectangle){P(f, 0.44f, 0.64f).x, P(f, 0.44f, 0.64f).y, S(f, 0.12f), S(f, 0.14f)}, tint);
    RoundRect(f, 0.30f, 0.78f, 0.40f, 0.09f, 0.04f, 0.0f, tint);
}

void UiIconCounter(Rectangle area, Color tint)
{
    IconFrame f = Frame(area);
    for (int i = 0; i < 3; i++) {
        float h = 0.26f + i * 0.20f;
        Rectangle bar = {P(f, 0.16f + i * 0.26f, 0.86f - h).x, P(f, 0.16f + i * 0.26f, 0.86f - h).y,
                         S(f, 0.16f), S(f, h)};
        DrawRectangleRounded(bar, 0.4f, 6, UiAlpha(tint, 0.55f + i * 0.22f));
    }
}
