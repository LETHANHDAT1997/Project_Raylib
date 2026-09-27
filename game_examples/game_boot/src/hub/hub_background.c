#include "hub_background.h"
#include "hub_wallpaper.h"
#include "ui_theme.h"
#include "ui_anim.h"
#include <math.h>

// Hình nền là một "mesh gradient": vài quầng màu lớn trôi chậm chồng lên nhau.
// Vì lớp kính phía trên sẽ làm mờ nền, các quầng này chỉ cần mềm và rộng.
#define BLOB_COUNT 6

typedef struct {
    float baseX, baseY;   // Vị trí gốc (0..1)
    float driftX, driftY; // Biên độ trôi
    float speed;          // Tốc độ trôi
    float phase;
    float radius;         // Bán kính theo tỉ lệ cạnh ngắn
    Color color;
} Blob;

static Blob s_blobs[BLOB_COUNT] = {
    {0.18f, 0.22f, 0.06f, 0.05f, 0.21f, 0.0f, 0.62f, {170, 196, 255, 255}},
    {0.82f, 0.18f, 0.05f, 0.07f, 0.17f, 1.7f, 0.55f, {236, 186, 226, 255}},
    {0.72f, 0.78f, 0.07f, 0.05f, 0.13f, 3.2f, 0.70f, {150, 214, 240, 255}},
    {0.26f, 0.84f, 0.06f, 0.06f, 0.19f, 4.6f, 0.58f, {255, 198, 190, 255}},
    {0.50f, 0.46f, 0.09f, 0.07f, 0.11f, 2.4f, 0.80f, {124, 158, 230, 255}},
    {0.94f, 0.55f, 0.05f, 0.08f, 0.23f, 5.1f, 0.45f, {206, 226, 255, 255}}
};

static Color s_accent = {126, 184, 255, 255};
static float s_motion = 1.0f;

void HubBackgroundUpdate(Color accent, float dt, bool reduceMotion)
{
    // Màu nhấn của game đang chọn thấm dần vào nền, không đổi đột ngột.
    s_accent = UiApproachColor(s_accent, accent, 3.0f, dt);
    s_motion = UiApproach(s_motion, reduceMotion ? 0.0f : 1.0f, 4.0f, dt);
}

void HubBackgroundDraw(int width, int height, float time)
{
    // Nếu người dùng chọn một ảnh nền thì dùng ảnh đó; gradient dựng bằng mã
    // chỉ là phương án mặc định khi không có ảnh nào.
    if (HubWallpaperDraw(width, height, time, s_accent, s_motion < 0.5f)) return;

    float w = (float)width;
    float h = (float)height;
    float unit = fminf(w, h);

    // Nền gốc: chuyển từ xanh đêm sang tím than.
    Color top    = UiMix((Color){38, 56, 108, 255}, s_accent, 0.18f);
    Color bottom = UiMix((Color){14, 22, 48, 255}, s_accent, 0.10f);
    DrawRectangleGradientV(0, 0, width, height, top, bottom);

    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < BLOB_COUNT; i++) {
        const Blob *b = &s_blobs[i];
        float t = time * b->speed * s_motion + b->phase;
        Vector2 c = {
            (b->baseX + sinf(t) * b->driftX) * w,
            (b->baseY + cosf(t * 1.3f) * b->driftY) * h
        };

        // Quầng nào cũng ngả nhẹ về màu nhấn để cả khung hình ăn nhập với nhau.
        Color col = UiMix(b->color, s_accent, 0.28f);
        float radius = b->radius * unit * (1.0f + 0.05f * sinf(t * 0.7f));
        DrawCircleGradient(c, radius, UiAlpha(col, 0.30f), UiAlpha(col, 0.0f));
    }
    EndBlendMode();

    // Làm tối bốn góc để nội dung ở giữa nổi lên (vignette).
    BeginBlendMode(BLEND_MULTIPLIED);
        DrawCircleGradient((Vector2){w * 0.5f, h * 0.5f}, unit * 1.05f,
                           (Color){255, 255, 255, 255}, (Color){168, 176, 208, 255});
    EndBlendMode();
}
