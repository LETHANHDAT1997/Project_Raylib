/**
 * caro_assets.h - Tài nguyên của Cờ Caro: vân gỗ, shader vẽ quân, âm thanh.
 *
 * Nạp theo bộ đếm tham chiếu để Arcade Hub (vẽ artwork) và game dùng chung.
 */
#ifndef CARO_ASSETS_H
#define CARO_ASSETS_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    CARO_SFX_PLACE_X = 0,
    CARO_SFX_PLACE_O,
    CARO_SFX_UNDO,
    CARO_SFX_WIN,
    CARO_SFX_LOSE,
    CARO_SFX_SELECT,
    CARO_SFX_CONFIRM,
    CARO_SFX_ERROR,
    CARO_SFX_HINT,
    CARO_SFX_COUNT
} CaroSfx;

// Kiểu nét vẽ của shader quân cờ.
typedef enum {
    CARO_SHAPE_X = 0,
    CARO_SHAPE_O = 1,
    CARO_SHAPE_BAR = 2      // Vạch bo tròn nằm ngang (dùng cho vạch thắng)
} CaroShape;

typedef struct {
    Texture2D wood;         // Vân gỗ sồi Poly Haven (CC0)
    Texture2D white;        // Texture 2x2 trắng để shader nhận toạ độ UV 0..1
    Shader pieceShader;
    bool shaderReady;
    int locShape, locColor, locEdge, locProgress, locGlow, locPixel, locAspect, locShadow;
} CaroAssets;

void LoadCaroAssets(void);
void UnloadCaroAssets(void);
const CaroAssets *CaroAssetsGet(void);
const char *CaroAssetPath(const char *relative);

void InitCaroAudio(void);
void CloseCaroAudio(void);
void PlayCaroSfx(CaroSfx sfx, bool enabled, float pitchJitter);

// Vẽ một quân / vạch bằng shader SDF (khử răng cưa, bóng đổ, hoạt ảnh nét).
// `rect` là ô vuông chứa quân; progress 0..1 là phần nét đã vẽ; glow 0..1
// làm quân phát sáng (hàng thắng, nước vừa đi).
void CaroDrawShape(CaroShape shape, Rectangle rect, float rotation, Color color, Color edge,
                   float progress, float glow, float shadow);

#endif // CARO_ASSETS_H
