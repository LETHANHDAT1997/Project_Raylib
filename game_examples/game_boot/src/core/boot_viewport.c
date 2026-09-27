#include "boot_viewport.h"
#include "rlgl.h"
#include <math.h>

static float s_scale = 1.0f;

void BootViewportUpdate(BootViewport *vp, int designWidth, int designHeight)
{
    vp->designWidth = designWidth;
    vp->designHeight = designHeight;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float scale = fminf(screenW / (float)designWidth, screenH / (float)designHeight);
    if (scale <= 0.0f) scale = 1.0f;

    vp->scale = scale;
    vp->offset = (Vector2){
        (screenW - designWidth * scale) * 0.5f,
        (screenH - designHeight * scale) * 0.5f
    };
    s_scale = scale;

    // Chuột được quy đổi về hệ toạ độ thiết kế để mã giao diện dùng trực tiếp.
    SetMouseOffset((int)(-vp->offset.x), (int)(-vp->offset.y));
    SetMouseScale(1.0f / scale, 1.0f / scale);
}

void BootViewportBegin(BootViewport *vp)
{
    BeginDrawing();
    ClearBackground(BLACK);

    rlPushMatrix();
    rlTranslatef(vp->offset.x, vp->offset.y, 0.0f);
    rlScalef(vp->scale, vp->scale, 1.0f);
}

void BootViewportEnd(BootViewport *vp)
{
    (void)vp;
    rlPopMatrix();
    EndDrawing();
}

float BootViewportScale(void)
{
    return s_scale;
}
