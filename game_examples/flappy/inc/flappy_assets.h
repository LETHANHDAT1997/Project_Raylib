#ifndef FLAPPY_ASSETS_H
#define FLAPPY_ASSETS_H

#include "flappy_types.h"

#define PLANE_FRAMES 3
#define BIOME_EVERY 10     // Mỗi 10 điểm chuyển sang vùng địa hình mới
#define FLAPPY_MAX_BIOMES 8
#define FLAPPY_MAX_SKINS 8

// Một "vùng" gồm bộ đá + dải đất + sắc trời riêng.
// Thêm vùng mới = thêm một phần tử trong s_biomeDefs (flappy_assets.c).
typedef struct {
    const char *name;
    const char *rockFile;      // Đá mọc từ dưới lên
    const char *rockDownFile;  // Đá treo từ trên xuống
    const char *groundFile;
    Color sky;
} BiomeDef;

// Máy bay người chơi chọn được ở menu - cũng hoàn toàn theo dữ liệu.
typedef struct {
    const char *name;
    const char *filePrefix;    // "planeRed" -> planeRed1..3.png
    Color accent;
} PlaneSkinDef;

typedef struct {
    Texture2D background;
    Texture2D rock[FLAPPY_MAX_BIOMES];
    Texture2D rockDown[FLAPPY_MAX_BIOMES];
    Texture2D ground[FLAPPY_MAX_BIOMES];
    unsigned char groundProfile[FLAPPY_MAX_BIOMES][GROUND_TEX_W];  // Độ sâu tới pixel đất đầu tiên theo cột
    Color groundBase[FLAPPY_MAX_BIOMES];   // Màu đáy dải đất, để lấp mép dưới khi rung màn hình

    Texture2D plane[FLAPPY_MAX_SKINS][PLANE_FRAMES];

    Texture2D puffSmall;
    Texture2D puffLarge;
    Texture2D star[STAR_KIND_COUNT];

    Texture2D uiPanel;
    Texture2D buttonLarge;
    Texture2D buttonSmall;
    Texture2D medal[3];
    Texture2D tap;
    Texture2D tapLeft;
    Texture2D tapRight;
    Texture2D textGameOver;
    Texture2D textGetReady;

    Texture2D numbers[10];
    Texture2D letters[26];
} FlappyAssets;

void LoadFlappyAssets(void);
void UnloadFlappyAssets(void);
const FlappyAssets *FlappyAssetsGet(void);

// Trả về đường dẫn đầy đủ tới file trong assets/flappy (NULL nếu không tìm thấy thư mục).
const char *FlappyAssetPath(const char *relative);

int FlappyBiomeCount(void);
const BiomeDef *FlappyBiome(int index);   // index được quay vòng

int FlappyPlaneSkinCount(void);
const PlaneSkinDef *FlappyPlaneSkin(int index);

// Độ cao mặt đất (toạ độ thế giới) tại cột x của một ô đất đang ở vị trí tileX.
float FlappyGroundTopAt(int biome, float tileX, float worldX);

#endif // FLAPPY_ASSETS_H
