#include "ui_theme.h"
#include <math.h>

// Bảng màu lấy cảm hứng từ Liquid Glass của Apple: nền xanh lam sâu,
// lớp kính gần như trong suốt, chữ trắng ngả xanh nhạt.
const UiTheme UI = {
    // Gần như trung tính: chữ không được góp thêm sắc màu nào vào giao diện,
    // màu phải đến từ hậu cảnh và từ màu nhấn của game.
    .textPrimary   = (Color){250, 251, 253, 255},
    .textSecondary = (Color){228, 232, 239, 238},
    .textMuted     = (Color){210, 215, 224, 198},
    .textOnAccent  = (Color){14, 18, 24, 255},

    .strokeSoft      = (Color){255, 255, 255, 54},
    .strokeStrong    = (Color){255, 255, 255, 104},
    .strokeHighlight = (Color){255, 255, 255, 180},

    .accent  = (Color){126, 184, 255, 255},
    .success = (Color){94, 222, 172, 255},
    .warning = (Color){255, 206, 120, 255},
    .danger  = (Color){255, 128, 132, 255},
};

static unsigned char MixByte(unsigned char a, unsigned char b, float t)
{
    float v = (float)a + ((float)b - (float)a) * t;
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    return (unsigned char)(v + 0.5f);
}

Color UiMix(Color a, Color b, float t)
{
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return (Color){
        MixByte(a.r, b.r, t),
        MixByte(a.g, b.g, t),
        MixByte(a.b, b.b, t),
        MixByte(a.a, b.a, t)
    };
}

Color UiAlpha(Color c, float alpha)
{
    float v = (float)c.a * alpha;
    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    c.a = (unsigned char)(v + 0.5f);
    return c;
}

Color UiShade(Color c, float amount)
{
    Color target = (amount >= 0.0f) ? (Color){255, 255, 255, c.a} : (Color){0, 0, 0, c.a};
    return UiMix(c, target, fabsf(amount));
}
