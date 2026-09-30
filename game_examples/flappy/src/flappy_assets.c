#include "flappy_assets.h"
#include <stdio.h>
#include <string.h>

// Bộ sprite là "Tappy Plane" của Kenney (CC0) - xem assets/flappy/LICENSES.md.
static const BiomeDef s_biomeDefs[] = {
    {"Đồng Cỏ",   "rockGrass.png", "rockGrassDown.png", "groundGrass.png", {255, 255, 255, 255}},
    {"Hẻm Núi",   "rock.png",      "rockDown.png",      "groundDirt.png",  {255, 236, 214, 255}},
    {"Núi Tuyết", "rockSnow.png",  "rockSnowDown.png",  "groundSnow.png",  {226, 236, 255, 255}},
    {"Sông Băng", "rockIce.png",   "rockIceDown.png",   "groundIce.png",   {206, 228, 255, 255}},
    {"Vách Đá",   "rock.png",      "rockDown.png",      "groundRock.png",  {236, 222, 240, 255}},
};
static const int s_biomeCount = (int)(sizeof(s_biomeDefs) / sizeof(s_biomeDefs[0]));

static const PlaneSkinDef s_planeSkins[] = {
    {"Đỏ Rực",    "planeRed",    {232, 76, 61, 255}},
    {"Vàng Nắng", "planeYellow", {242, 196, 48, 255}},
    {"Xanh Biển", "planeBlue",   {72, 148, 230, 255}},
    {"Lục Bảo",   "planeGreen",  {96, 190, 92, 255}},
};
static const int s_planeSkinCount = (int)(sizeof(s_planeSkins) / sizeof(s_planeSkins[0]));

static FlappyAssets s_assets;
static int s_refCount = 0;
static char s_root[256] = "";

static void ResolveRoot(void)
{
    if (s_root[0] != '\0') return;

    // Game có thể được chạy từ thư mục build của nó, từ gốc repo hoặc từ Arcade Hub,
    // nên dò ngược vài cấp thay vì cố định một đường dẫn.
    static const char *candidates[] = {
        "assets/flappy/",
        "../assets/flappy/",
        "../../assets/flappy/",
        "../../../assets/flappy/",
        "game_examples/assets/flappy/",
        "../game_examples/assets/flappy/",
        "../../game_examples/assets/flappy/",
        NULL
    };
    for (int i = 0; candidates[i] != NULL; i++) {
        if (DirectoryExists(TextFormat("%ssprites", candidates[i]))) {
            strncpy(s_root, candidates[i], sizeof(s_root) - 1);
            return;
        }
    }
    TraceLog(LOG_WARNING, "FLAPPY: không tìm thấy thư mục assets/flappy");
    strncpy(s_root, "assets/flappy/", sizeof(s_root) - 1);
}

const char *FlappyAssetPath(const char *relative)
{
    static char buffer[512];
    ResolveRoot();
    snprintf(buffer, sizeof(buffer), "%s%s", s_root, relative);
    return buffer;
}

static Texture2D LoadSprite(const char *relative)
{
    const char *path = FlappyAssetPath(relative);
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "FLAPPY: thiếu asset '%s'", path);
        Image img = GenImageColor(4, 4, MAGENTA);
        Texture2D fallback = LoadTextureFromImage(img);
        UnloadImage(img);
        return fallback;
    }
    Texture2D tex = LoadTexture(path);
    // Tranh Kenney là vector xuất PNG, lọc song tuyến để phóng to vẫn mềm mại.
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);
    return tex;
}

// Dò mép trên của dải đất từng cột một, để va chạm bám đúng đồi núi trong ảnh
// chứ không phải một đường thẳng.
static void BuildGroundProfile(const char *relative, unsigned char *profile, Color *base)
{
    for (int x = 0; x < GROUND_TEX_W; x++) profile[x] = 36;
    *base = (Color){150, 110, 70, 255};

    Image img = LoadImage(FlappyAssetPath(relative));
    if (img.data == NULL) return;
    *base = GetImageColor(img, img.width / 2, img.height - 1);

    int w = img.width < GROUND_TEX_W ? img.width : GROUND_TEX_W;
    for (int x = 0; x < w; x++) {
        int top = img.height - 1;
        for (int y = 0; y < img.height; y++) {
            if (GetImageColor(img, x, y).a > 128) { top = y; break; }
        }
        profile[x] = (unsigned char)top;
    }
    UnloadImage(img);
}

void LoadFlappyAssets(void)
{
    s_refCount++;
    if (s_refCount > 1) return;

    memset(&s_assets, 0, sizeof(s_assets));

    s_assets.background = LoadSprite("sprites/background.png");
    SetTextureWrap(s_assets.background, TEXTURE_WRAP_REPEAT);

    for (int i = 0; i < s_biomeCount && i < FLAPPY_MAX_BIOMES; i++) {
        s_assets.rock[i]     = LoadSprite(TextFormat("sprites/%s", s_biomeDefs[i].rockFile));
        s_assets.rockDown[i] = LoadSprite(TextFormat("sprites/%s", s_biomeDefs[i].rockDownFile));
        s_assets.ground[i]   = LoadSprite(TextFormat("sprites/%s", s_biomeDefs[i].groundFile));
        BuildGroundProfile(TextFormat("sprites/%s", s_biomeDefs[i].groundFile), s_assets.groundProfile[i], &s_assets.groundBase[i]);
    }

    for (int i = 0; i < s_planeSkinCount && i < FLAPPY_MAX_SKINS; i++) {
        for (int f = 0; f < PLANE_FRAMES; f++) {
            s_assets.plane[i][f] = LoadSprite(TextFormat("sprites/planes/%s%d.png", s_planeSkins[i].filePrefix, f + 1));
        }
    }

    s_assets.puffSmall = LoadSprite("sprites/puffSmall.png");
    s_assets.puffLarge = LoadSprite("sprites/puffLarge.png");
    s_assets.star[STAR_BRONZE] = LoadSprite("sprites/starBronze.png");
    s_assets.star[STAR_SILVER] = LoadSprite("sprites/starSilver.png");
    s_assets.star[STAR_GOLD]   = LoadSprite("sprites/starGold.png");

    s_assets.uiPanel      = LoadSprite("sprites/ui/UIbg.png");
    s_assets.buttonLarge  = LoadSprite("sprites/ui/buttonLarge.png");
    s_assets.buttonSmall  = LoadSprite("sprites/ui/buttonSmall.png");
    s_assets.medal[MEDAL_BRONZE] = LoadSprite("sprites/ui/medalBronze.png");
    s_assets.medal[MEDAL_SILVER] = LoadSprite("sprites/ui/medalSilver.png");
    s_assets.medal[MEDAL_GOLD]   = LoadSprite("sprites/ui/medalGold.png");
    s_assets.tap          = LoadSprite("sprites/ui/tap.png");
    s_assets.tapLeft      = LoadSprite("sprites/ui/tapLeft.png");
    s_assets.tapRight     = LoadSprite("sprites/ui/tapRight.png");
    s_assets.textGameOver = LoadSprite("sprites/ui/textGameOver.png");
    s_assets.textGetReady = LoadSprite("sprites/ui/textGetReady.png");

    for (int i = 0; i < 10; i++) {
        s_assets.numbers[i] = LoadSprite(TextFormat("sprites/numbers/number%d.png", i));
    }
    for (int i = 0; i < 26; i++) {
        s_assets.letters[i] = LoadSprite(TextFormat("sprites/letters/letter%c.png", 'A' + i));
    }
}

static void UnloadIfLoaded(Texture2D *tex)
{
    if (tex->id > 0) UnloadTexture(*tex);
    tex->id = 0;
}

void UnloadFlappyAssets(void)
{
    if (s_refCount <= 0) return;
    s_refCount--;
    if (s_refCount > 0) return;

    UnloadIfLoaded(&s_assets.background);
    for (int i = 0; i < FLAPPY_MAX_BIOMES; i++) {
        UnloadIfLoaded(&s_assets.rock[i]);
        UnloadIfLoaded(&s_assets.rockDown[i]);
        UnloadIfLoaded(&s_assets.ground[i]);
    }
    for (int i = 0; i < FLAPPY_MAX_SKINS; i++) {
        for (int f = 0; f < PLANE_FRAMES; f++) UnloadIfLoaded(&s_assets.plane[i][f]);
    }
    UnloadIfLoaded(&s_assets.puffSmall);
    UnloadIfLoaded(&s_assets.puffLarge);
    for (int i = 0; i < STAR_KIND_COUNT; i++) UnloadIfLoaded(&s_assets.star[i]);
    UnloadIfLoaded(&s_assets.uiPanel);
    UnloadIfLoaded(&s_assets.buttonLarge);
    UnloadIfLoaded(&s_assets.buttonSmall);
    for (int i = 0; i < 3; i++) UnloadIfLoaded(&s_assets.medal[i]);
    UnloadIfLoaded(&s_assets.tap);
    UnloadIfLoaded(&s_assets.tapLeft);
    UnloadIfLoaded(&s_assets.tapRight);
    UnloadIfLoaded(&s_assets.textGameOver);
    UnloadIfLoaded(&s_assets.textGetReady);
    for (int i = 0; i < 10; i++) UnloadIfLoaded(&s_assets.numbers[i]);
    for (int i = 0; i < 26; i++) UnloadIfLoaded(&s_assets.letters[i]);
}

const FlappyAssets *FlappyAssetsGet(void)
{
    return &s_assets;
}

int FlappyBiomeCount(void)
{
    return s_biomeCount;
}

const BiomeDef *FlappyBiome(int index)
{
    if (index < 0) index = 0;
    return &s_biomeDefs[index % s_biomeCount];
}

int FlappyPlaneSkinCount(void)
{
    return s_planeSkinCount;
}

const PlaneSkinDef *FlappyPlaneSkin(int index)
{
    if (index < 0) index = 0;
    return &s_planeSkins[index % s_planeSkinCount];
}

float FlappyGroundTopAt(int biome, float tileX, float worldX)
{
    int col = (int)(worldX - tileX);
    if (col < 0) col = 0;
    if (col >= GROUND_TEX_W) col = GROUND_TEX_W - 1;
    int b = biome % s_biomeCount;
    return (float)GROUND_Y + (float)s_assets.groundProfile[b][col];
}
