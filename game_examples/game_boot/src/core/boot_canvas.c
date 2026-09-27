#include "boot_canvas.h"
#include "raymath.h"
#include <stdbool.h>
#include <math.h>

void InitBootCanvas(BootCanvas *canvas, int virtualWidth, int virtualHeight)
{
    canvas->virtualWidth = virtualWidth;
    canvas->virtualHeight = virtualHeight;
    canvas->target = LoadRenderTexture(virtualWidth, virtualHeight);
    SetTextureFilter(canvas->target.texture, TEXTURE_FILTER_BILINEAR);

    canvas->sourceRec = (Rectangle){0.0f, 0.0f, (float)virtualWidth, -(float)virtualHeight};
    canvas->destRec = (Rectangle){0.0f, 0.0f, (float)virtualWidth, (float)virtualHeight};
    canvas->scale = 1.0f;

    UpdateBootCanvas(canvas);
}

void SetBootCanvasSize(BootCanvas *canvas, int virtualWidth, int virtualHeight)
{
    // Cũng phải cấp phát lại khi render texture đã bị giải phóng trước đó
    // (Hub không dùng canvas nên nó được thu hồi mỗi lần thoát game).
    bool sameSize = (canvas->virtualWidth == virtualWidth && canvas->virtualHeight == virtualHeight);
    if (sameSize && canvas->target.id > 0) {
        return;
    }

    if (canvas->target.id > 0) {
        UnloadRenderTexture(canvas->target);
    }

    canvas->virtualWidth = virtualWidth;
    canvas->virtualHeight = virtualHeight;
    canvas->target = LoadRenderTexture(virtualWidth, virtualHeight);
    SetTextureFilter(canvas->target.texture, TEXTURE_FILTER_BILINEAR);

    canvas->sourceRec = (Rectangle){0.0f, 0.0f, (float)virtualWidth, -(float)virtualHeight};
    UpdateBootCanvas(canvas);
}

void UpdateBootCanvas(BootCanvas *canvas)
{
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // Tính tỷ lệ Scale đồng dạng (giữ nguyên aspect ratio)
    float scale = fminf(screenW / (float)canvas->virtualWidth, screenH / (float)canvas->virtualHeight);
    if (scale <= 0.0f) scale = 1.0f;
    canvas->scale = scale;

    float destW = (float)canvas->virtualWidth * scale;
    float destH = (float)canvas->virtualHeight * scale;

    canvas->destRec.x = (screenW - destW) * 0.5f;
    canvas->destRec.y = (screenH - destH) * 0.5f;
    canvas->destRec.width = destW;
    canvas->destRec.height = destH;

    // Cập nhật tọa độ chuột ảo tự động cho Raylib!
    SetMouseOffset((int)-canvas->destRec.x, (int)-canvas->destRec.y);
    SetMouseScale(1.0f / scale, 1.0f / scale);
}

void BeginBootCanvas(BootCanvas *canvas)
{
    UpdateBootCanvas(canvas);
    BeginTextureMode(canvas->target);
}

void EndBootCanvas(BootCanvas *canvas)
{
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK); // Viền đen Letterbox/Pillarbox đối xứng
        DrawTexturePro(canvas->target.texture, canvas->sourceRec, canvas->destRec, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}

void CloseBootCanvas(BootCanvas *canvas)
{
    if (canvas->target.id > 0) {
        UnloadRenderTexture(canvas->target);
    }
    canvas->target = (RenderTexture2D){0};
    canvas->virtualWidth = 0;
    canvas->virtualHeight = 0;
}
