#include "caro_assets.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Vân gỗ: Poly Haven "Oak Veneer 01" (CC0). Âm thanh: Kenney (CC0).
// Xem assets/caro/LICENSES.md.

static CaroAssets s_assets;
static int s_refCount = 0;
static char s_root[256] = "";

static void ResolveRoot(void)
{
    if (s_root[0] != '\0') return;
    // Game có thể chạy từ thư mục build, từ gốc repo hoặc từ Arcade Hub.
    static const char *candidates[] = {
        "assets/caro/",
        "../assets/caro/",
        "../../assets/caro/",
        "../../../assets/caro/",
        "game_examples/assets/caro/",
        "../game_examples/assets/caro/",
        "../../game_examples/assets/caro/",
        NULL
    };
    for (int i = 0; candidates[i] != NULL; i++) {
        char probe[300];
        snprintf(probe, sizeof(probe), "%stextures", candidates[i]);
        if (DirectoryExists(probe)) {
            strncpy(s_root, candidates[i], sizeof(s_root) - 1);
            return;
        }
    }
    TraceLog(LOG_WARNING, "CARO: không tìm thấy thư mục assets/caro");
    strncpy(s_root, "assets/caro/", sizeof(s_root) - 1);
}

const char *CaroAssetPath(const char *relative)
{
    static char buffer[512];
    ResolveRoot();
    snprintf(buffer, sizeof(buffer), "%s%s", s_root, relative);
    return buffer;
}

// ------------------------------------------------------------------ shader quân cờ

static const char *GLSL_330 =
"#version 330\n"
"#define VARYING in\n"
"out vec4 finalColor;\n";

static const char *GLSL_120 =
"#version 120\n"
"#define VARYING varying\n"
"#define finalColor gl_FragColor\n";

static const char *GLSL_300ES =
"#version 300 es\n"
"precision highp float;\n"
"#define VARYING in\n"
"out vec4 finalColor;\n";

static const char *GLSL_100 =
"#version 100\n"
"#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
"precision highp float;\n"
"#else\n"
"precision mediump float;\n"
"#endif\n"
"#define VARYING varying\n"
"#define finalColor gl_FragColor\n";

// Quân X/O dựng bằng hàm khoảng cách (SDF): nét luôn mịn ở mọi cỡ, có bóng
// đổ mềm, khối "ống" sáng ở giữa nét và hoạt ảnh vẽ nét theo uProgress.
static const char *PIECE_FS =
"VARYING vec2 fragTexCoord;\n"
"VARYING vec4 fragColor;\n"
"uniform float uShape;\n"
"uniform vec4 uColor;\n"
"uniform vec4 uEdge;\n"
"uniform float uProgress;\n"
"uniform float uGlow;\n"
"uniform float uPixel;\n"
"uniform float uAspect;\n"
"uniform float uShadow;\n"
"const float PI2 = 6.2831853;\n"
"float sdSeg(vec2 p, vec2 a, vec2 b) {\n"
"    vec2 pa = p - a, ba = b - a;\n"
"    float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);\n"
"    return length(pa - ba * h);\n"
"}\n"
"float halfWidth() {\n"
"    if (uShape < 0.5) return 0.16;\n"
"    if (uShape < 1.5) return 0.145;\n"
"    return 0.42;\n"
"}\n"
"float shapeDist(vec2 p) {\n"
"    float t = halfWidth();\n"
"    if (uShape < 0.5) {\n"
"        float a = 0.60;\n"
"        float p1 = clamp(uProgress * 2.0, 0.0, 1.0);\n"
"        float p2 = clamp(uProgress * 2.0 - 1.0, 0.0, 1.0);\n"
"        float d = 1e3;\n"
"        if (p1 > 0.001) d = min(d, sdSeg(p, vec2(-a, -a), vec2(-a, -a) + vec2(2.0 * a, 2.0 * a) * p1));\n"
"        if (p2 > 0.001) d = min(d, sdSeg(p, vec2(a, -a), vec2(a, -a) + vec2(-2.0 * a, 2.0 * a) * p2));\n"
"        return d - t;\n"
"    } else if (uShape < 1.5) {\n"
"        float r = 0.58;\n"
"        if (uProgress >= 0.999) return abs(length(p) - r) - t;\n"
"        float ang = atan(p.x, -p.y);\n"
"        if (ang < 0.0) ang += PI2;\n"
"        if (ang / PI2 <= uProgress) return abs(length(p) - r) - t;\n"
"        vec2 s = vec2(0.0, -r);\n"
"        vec2 e = vec2(sin(uProgress * PI2), -cos(uProgress * PI2)) * r;\n"
"        return min(length(p - s), length(p - e)) - t;\n"
"    }\n"
"    float len = max(uAspect - 0.55, 0.01);\n"
"    vec2 q = vec2(p.x * uAspect, p.y);\n"
"    return sdSeg(q, vec2(-len, 0.0), vec2(-len + 2.0 * len * uProgress, 0.0)) - t;\n"
"}\n"
"void main() {\n"
"    vec2 p = fragTexCoord * 2.0 - 1.0;\n"
"    float t = halfWidth();\n"
"    float d = shapeDist(p);\n"
"    float aa = uPixel * 1.1;\n"
"    float fill = 1.0 - smoothstep(-aa, aa, d);\n"
"    float s = clamp(-d / t, 0.0, 1.0);\n"
"    vec3 base = mix(uEdge.rgb, uColor.rgb, smoothstep(0.0, 0.55, s));\n"
"    float side = clamp(0.5 - (p.x + p.y) * 0.45, 0.0, 1.0);\n"
"    base += vec3(0.30) * pow(s, 4.0) * side;\n"
"    float ds = shapeDist(p - vec2(0.045, 0.075));\n"
"    float shadowA = (1.0 - smoothstep(-0.06, 0.16, ds)) * uShadow * 0.42;\n"
"    float glowA = exp(-max(d, 0.0) * 7.0) * uGlow * 0.75 * (1.0 - fill);\n"
"    vec4 acc = vec4(0.0, 0.0, 0.0, shadowA);\n"
"    acc = acc * (1.0 - glowA) + vec4(uColor.rgb * glowA, glowA);\n"
"    acc = acc * (1.0 - fill) + vec4(base * fill, fill);\n"
"    float alpha = acc.a * uColor.a * fragColor.a;\n"
"    finalColor = vec4(acc.rgb / max(acc.a, 0.0001), alpha);\n"
"}\n";

static const char *ShaderHeader(void)
{
    switch (rlGetVersion()) {
        case RL_OPENGL_ES_20: return GLSL_100;
        case RL_OPENGL_ES_30: return GLSL_300ES;
        case RL_OPENGL_21:    return GLSL_120;
        default:              return GLSL_330;
    }
}

static void LoadPieceShader(CaroAssets *a)
{
    const char *header = ShaderHeader();
    size_t hl = strlen(header), bl = strlen(PIECE_FS);
    char *code = (char *)MemAlloc((unsigned int)(hl + bl + 1));
    memcpy(code, header, hl);
    memcpy(code + hl, PIECE_FS, bl + 1);
    a->pieceShader = LoadShaderFromMemory(NULL, code);
    MemFree(code);

    a->shaderReady = a->pieceShader.id > 0 && a->pieceShader.id != rlGetShaderIdDefault();
    if (!a->shaderReady) {
        TraceLog(LOG_WARNING, "CARO: shader quân cờ không biên dịch được, dùng nét vẽ dự phòng");
        return;
    }
    a->locShape    = GetShaderLocation(a->pieceShader, "uShape");
    a->locColor    = GetShaderLocation(a->pieceShader, "uColor");
    a->locEdge     = GetShaderLocation(a->pieceShader, "uEdge");
    a->locProgress = GetShaderLocation(a->pieceShader, "uProgress");
    a->locGlow     = GetShaderLocation(a->pieceShader, "uGlow");
    a->locPixel    = GetShaderLocation(a->pieceShader, "uPixel");
    a->locAspect   = GetShaderLocation(a->pieceShader, "uAspect");
    a->locShadow   = GetShaderLocation(a->pieceShader, "uShadow");
}

// ------------------------------------------------------------------ nạp / giải phóng

void LoadCaroAssets(void)
{
    if (s_refCount++ > 0) return;
    memset(&s_assets, 0, sizeof(s_assets));

    const char *woodPath = CaroAssetPath("textures/oak_veneer.jpg");
    if (FileExists(woodPath)) {
        s_assets.wood = LoadTexture(woodPath);
        GenTextureMipmaps(&s_assets.wood);
        SetTextureFilter(s_assets.wood, TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(s_assets.wood, TEXTURE_WRAP_MIRROR_REPEAT);
    }
    if (s_assets.wood.id == 0) {
        TraceLog(LOG_WARNING, "CARO: thiếu vân gỗ '%s', dùng màu trơn", woodPath);
        Image img = GenImageColor(8, 8, (Color){214, 176, 122, 255});
        s_assets.wood = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    Image white = GenImageColor(2, 2, WHITE);
    s_assets.white = LoadTextureFromImage(white);
    UnloadImage(white);
    SetTextureFilter(s_assets.white, TEXTURE_FILTER_BILINEAR);

    LoadPieceShader(&s_assets);
}

void UnloadCaroAssets(void)
{
    if (s_refCount <= 0 || --s_refCount > 0) return;
    if (s_assets.wood.id > 0) UnloadTexture(s_assets.wood);
    if (s_assets.white.id > 0) UnloadTexture(s_assets.white);
    if (s_assets.shaderReady) UnloadShader(s_assets.pieceShader);
    memset(&s_assets, 0, sizeof(s_assets));
}

const CaroAssets *CaroAssetsGet(void)
{
    return &s_assets;
}

// ------------------------------------------------------------------ vẽ quân

static void SetVec4(int loc, Color c)
{
    float v[4] = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
    SetShaderValue(s_assets.pieceShader, loc, v, SHADER_UNIFORM_VEC4);
}

static void SetFloat(int loc, float f)
{
    SetShaderValue(s_assets.pieceShader, loc, &f, SHADER_UNIFORM_FLOAT);
}

static void DrawShapeFallback(CaroShape shape, Rectangle r, float rotation, Color color, float progress)
{
    Vector2 c = {r.x + r.width * 0.5f, r.y + r.height * 0.5f};
    float u = r.height * 0.5f;
    if (shape == CARO_SHAPE_X) {
        float a = u * 0.5f, t = u * 0.27f;
        float p1 = progress * 2.0f > 1.0f ? 1.0f : progress * 2.0f;
        float p2 = progress * 2.0f - 1.0f;
        if (p1 > 0.0f) DrawLineEx((Vector2){c.x - a, c.y - a}, (Vector2){c.x - a + 2 * a * p1, c.y - a + 2 * a * p1}, t, color);
        if (p2 > 0.0f) DrawLineEx((Vector2){c.x + a, c.y - a}, (Vector2){c.x + a - 2 * a * p2, c.y - a + 2 * a * p2}, t, color);
    } else if (shape == CARO_SHAPE_O) {
        DrawRing(c, u * 0.35f, u * 0.6f, 180.0f, 180.0f + 360.0f * progress, 40, color);
    } else {
        float len = (r.width - r.height) * 0.5f + r.height * 0.2f;
        Vector2 dir = {cosf(rotation * DEG2RAD), sinf(rotation * DEG2RAD)};
        Vector2 a = {c.x - dir.x * len, c.y - dir.y * len};
        Vector2 b = {a.x + dir.x * 2 * len * progress, a.y + dir.y * 2 * len * progress};
        DrawLineEx(a, b, r.height * 0.4f, color);
    }
}

void CaroDrawShape(CaroShape shape, Rectangle rect, float rotation, Color color, Color edge,
                   float progress, float glow, float shadow)
{
    if (!s_assets.shaderReady) {
        DrawShapeFallback(shape, rect, rotation, color, progress);
        return;
    }
    BeginShaderMode(s_assets.pieceShader);
        SetFloat(s_assets.locShape, (float)shape);
        SetVec4(s_assets.locColor, color);
        SetVec4(s_assets.locEdge, edge);
        SetFloat(s_assets.locProgress, progress);
        SetFloat(s_assets.locGlow, glow);
        SetFloat(s_assets.locPixel, 2.0f / (rect.height > 1.0f ? rect.height : 1.0f));
        SetFloat(s_assets.locAspect, rect.width / (rect.height > 1.0f ? rect.height : 1.0f));
        SetFloat(s_assets.locShadow, shadow);
        Rectangle src = {0.0f, 0.0f, 2.0f, 2.0f};
        Rectangle dst = {rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f, rect.width, rect.height};
        DrawTexturePro(s_assets.white, src, dst, (Vector2){rect.width * 0.5f, rect.height * 0.5f}, rotation, WHITE);
    EndShaderMode();
}

// ------------------------------------------------------------------ âm thanh

static const char *s_sfxFiles[CARO_SFX_COUNT] = {
    [CARO_SFX_PLACE_X] = "audio/place1.ogg",
    [CARO_SFX_PLACE_O] = "audio/place2.ogg",
    [CARO_SFX_UNDO]    = "audio/undo.ogg",
    [CARO_SFX_WIN]     = "audio/win.ogg",
    [CARO_SFX_LOSE]    = "audio/lose.ogg",
    [CARO_SFX_SELECT]  = "audio/select.ogg",
    [CARO_SFX_CONFIRM] = "audio/confirm.ogg",
    [CARO_SFX_ERROR]   = "audio/error.ogg",
    [CARO_SFX_HINT]    = "audio/hint.ogg",
};

static const float s_sfxVolume[CARO_SFX_COUNT] = {
    [CARO_SFX_PLACE_X] = 0.75f,
    [CARO_SFX_PLACE_O] = 0.75f,
    [CARO_SFX_UNDO]    = 0.55f,
    [CARO_SFX_WIN]     = 0.60f,
    [CARO_SFX_LOSE]    = 0.55f,
    [CARO_SFX_SELECT]  = 0.45f,
    [CARO_SFX_CONFIRM] = 0.50f,
    [CARO_SFX_ERROR]   = 0.40f,
    [CARO_SFX_HINT]    = 0.50f,
};

static Sound s_sounds[CARO_SFX_COUNT];
static bool s_audioReady = false;

void InitCaroAudio(void)
{
    if (s_audioReady) return;
    if (!IsAudioDeviceReady()) InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    for (int i = 0; i < CARO_SFX_COUNT; i++) {
        const char *path = CaroAssetPath(s_sfxFiles[i]);
        s_sounds[i] = (Sound){0};
        if (!FileExists(path)) {
            TraceLog(LOG_WARNING, "CARO: thiếu âm thanh '%s'", path);
            continue;
        }
        s_sounds[i] = LoadSound(path);
        SetSoundVolume(s_sounds[i], s_sfxVolume[i]);
    }
    s_audioReady = true;
}

void CloseCaroAudio(void)
{
    if (!s_audioReady) return;
    for (int i = 0; i < CARO_SFX_COUNT; i++) {
        if (s_sounds[i].frameCount > 0) UnloadSound(s_sounds[i]);
        s_sounds[i] = (Sound){0};
    }
    s_audioReady = false;
}

void PlayCaroSfx(CaroSfx sfx, bool enabled, float pitchJitter)
{
    if (!s_audioReady || !enabled || sfx < 0 || sfx >= CARO_SFX_COUNT) return;
    Sound s = s_sounds[sfx];
    if (s.frameCount == 0) return;
    float pitch = 1.0f;
    if (pitchJitter > 0.0f) pitch += ((float)GetRandomValue(-100, 100) / 100.0f) * pitchJitter;
    SetSoundPitch(s, pitch);
    PlaySound(s);
}
