#include "ui_glass.h"
#include "ui_glass_shaders.h"
#include "ui_theme.h"
#include "rlgl.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

// Nền được thu nhỏ trước khi blur: rẻ hơn nhiều mà mắt không phân biệt được,
// vì kết quả vốn đã là một lớp mờ.
#define GLASS_BLUR_DIV 4

typedef struct {
    int quadPos, quadSize, resolution, rectCenter, rectHalf, radius;
    int tint, refraction, edgeWidth, highlight, innerShadow, alpha;
    int targetLum, level, saturation;
    int sampleMode, flipY;
} PanelLocs;

typedef struct {
    int quadPos, quadSize, rectCenter, rectHalf, radius, thickness, spread, mode, color;
} StrokeLocs;

static struct {
    bool ready;
    float blurAmount;   // 0..1, do người dùng chỉnh trong Cài đặt
    float levelScale;   // Hệ số nhân mức "đục" của các preset kính
    int width, height;

    RenderTexture2D backdrop;
    RenderTexture2D blurA;
    RenderTexture2D blurB;

    Shader blurShader;
    int blurDirLoc;

    Shader panelShader;
    PanelLocs panelLoc;

    Shader strokeShader;
    StrokeLocs strokeLoc;

    Texture2D whitePixel;
    bool shaderReady;
} G = {0};

// ---------------------------------------------------------------- Tiện ích

static Texture2D MakeWhitePixel(void)
{
    Image img = GenImageColor(1, 1, WHITE);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Vector4 ColorToVec4(Color c)
{
    return (Vector4){c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
}

static void CachePanelLocs(void)
{
    Shader s = G.panelShader;
    G.panelLoc.quadPos     = GetShaderLocation(s, "uQuadPos");
    G.panelLoc.quadSize    = GetShaderLocation(s, "uQuadSize");
    G.panelLoc.resolution  = GetShaderLocation(s, "uResolution");
    G.panelLoc.rectCenter  = GetShaderLocation(s, "uRectCenter");
    G.panelLoc.rectHalf    = GetShaderLocation(s, "uRectHalf");
    G.panelLoc.radius      = GetShaderLocation(s, "uRadius");
    G.panelLoc.tint        = GetShaderLocation(s, "uTint");
    G.panelLoc.refraction  = GetShaderLocation(s, "uRefraction");
    G.panelLoc.edgeWidth   = GetShaderLocation(s, "uEdgeWidth");
    G.panelLoc.highlight   = GetShaderLocation(s, "uHighlight");
    G.panelLoc.innerShadow = GetShaderLocation(s, "uInnerShadow");
    G.panelLoc.targetLum   = GetShaderLocation(s, "uTargetLum");
    G.panelLoc.level       = GetShaderLocation(s, "uLevel");
    G.panelLoc.saturation  = GetShaderLocation(s, "uSaturation");
    G.panelLoc.alpha       = GetShaderLocation(s, "uAlpha");
    G.panelLoc.sampleMode  = GetShaderLocation(s, "uSampleMode");
    G.panelLoc.flipY       = GetShaderLocation(s, "uFlipY");
}

static void CacheStrokeLocs(void)
{
    Shader s = G.strokeShader;
    G.strokeLoc.quadPos    = GetShaderLocation(s, "uQuadPos");
    G.strokeLoc.quadSize   = GetShaderLocation(s, "uQuadSize");
    G.strokeLoc.rectCenter = GetShaderLocation(s, "uRectCenter");
    G.strokeLoc.rectHalf   = GetShaderLocation(s, "uRectHalf");
    G.strokeLoc.radius     = GetShaderLocation(s, "uRadius");
    G.strokeLoc.thickness  = GetShaderLocation(s, "uThickness");
    G.strokeLoc.spread     = GetShaderLocation(s, "uSpread");
    G.strokeLoc.mode       = GetShaderLocation(s, "uMode");
    G.strokeLoc.color      = GetShaderLocation(s, "uColor");
}

// ------------------------------------------------------------- Vòng đời

void UiGlassInit(int width, int height)
{
    if (G.ready && G.width == width && G.height == height) return;
    if (G.ready) UiGlassShutdown();

    G.width = width;
    G.height = height;
    G.blurAmount = 0.62f;
    G.levelScale = 0.80f;

    G.backdrop = LoadRenderTexture(width, height);
    SetTextureFilter(G.backdrop.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(G.backdrop.texture, TEXTURE_WRAP_CLAMP);

    int bw = width / GLASS_BLUR_DIV;
    int bh = height / GLASS_BLUR_DIV;
    if (bw < 2) bw = 2;
    if (bh < 2) bh = 2;

    G.blurA = LoadRenderTexture(bw, bh);
    G.blurB = LoadRenderTexture(bw, bh);
    SetTextureFilter(G.blurA.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(G.blurB.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(G.blurA.texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(G.blurB.texture, TEXTURE_WRAP_CLAMP);

    G.blurShader   = LoadShaderFromMemory(NULL, UI_GLASS_BLUR_FS);
    G.panelShader  = LoadShaderFromMemory(NULL, UI_GLASS_PANEL_FS);
    G.strokeShader = LoadShaderFromMemory(NULL, UI_GLASS_STROKE_FS);

    G.shaderReady = IsShaderValid(G.blurShader) &&
                    IsShaderValid(G.panelShader) &&
                    IsShaderValid(G.strokeShader);

    if (G.shaderReady) {
        G.blurDirLoc = GetShaderLocation(G.blurShader, "uDir");
        CachePanelLocs();
        CacheStrokeLocs();
    } else {
        TraceLog(LOG_WARNING, "UI_GLASS: shader không biên dịch được, dùng chế độ vẽ dự phòng");
    }

    G.whitePixel = MakeWhitePixel();
    G.ready = true;
}

void UiGlassShutdown(void)
{
    if (!G.ready) return;

    if (IsShaderValid(G.blurShader))   UnloadShader(G.blurShader);
    if (IsShaderValid(G.panelShader))  UnloadShader(G.panelShader);
    if (IsShaderValid(G.strokeShader)) UnloadShader(G.strokeShader);

    if (IsRenderTextureValid(G.backdrop)) UnloadRenderTexture(G.backdrop);
    if (IsRenderTextureValid(G.blurA))    UnloadRenderTexture(G.blurA);
    if (IsRenderTextureValid(G.blurB))    UnloadRenderTexture(G.blurB);
    if (IsTextureValid(G.whitePixel))     UnloadTexture(G.whitePixel);

    memset(&G, 0, sizeof(G));
}

void UiGlassSetBlurAmount(float amount)
{
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    G.blurAmount = amount;
}

float GlassLevelScale(float slider01)
{
    if (slider01 < 0.0f) slider01 = 0.0f;
    if (slider01 > 1.0f) slider01 = 1.0f;
    return slider01 * 0.98f;
}

void UiGlassSetLevelScale(float scale)
{
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 1.0f) scale = 1.0f;
    G.levelScale = scale;
}

bool UiGlassIsShaderReady(void)
{
    return G.ready && G.shaderReady;
}

// ---------------------------------------------------------------- Hình nền

void UiGlassBeginBackdrop(void)
{
    if (!G.ready) return;
    BeginTextureMode(G.backdrop);
}

// Chạy một lượt blur: src -> dst theo hướng dir (đơn vị texel).
static void BlurPass(RenderTexture2D src, RenderTexture2D dst, Vector2 dir)
{
    BeginTextureMode(dst);
        ClearBackground(BLANK);
        BeginShaderMode(G.blurShader);
            SetShaderValue(G.blurShader, G.blurDirLoc, &dir, SHADER_UNIFORM_VEC2);
            DrawTexturePro(src.texture,
                           (Rectangle){0, 0, (float)src.texture.width, -(float)src.texture.height},
                           (Rectangle){0, 0, (float)dst.texture.width, (float)dst.texture.height},
                           (Vector2){0, 0}, 0.0f, WHITE);
        EndShaderMode();
    EndTextureMode();
}

void UiGlassEndBackdrop(void)
{
    if (!G.ready) return;
    EndTextureMode();

    if (!G.shaderReady) return;

    // Thu nhỏ nền về kích thước blur trước khi lọc.
    BeginTextureMode(G.blurA);
        ClearBackground(BLANK);
        DrawTexturePro(G.backdrop.texture,
                       (Rectangle){0, 0, (float)G.backdrop.texture.width, -(float)G.backdrop.texture.height},
                       (Rectangle){0, 0, (float)G.blurA.texture.width, (float)G.blurA.texture.height},
                       (Vector2){0, 0}, 0.0f, WHITE);
    EndTextureMode();

    // Ở mức 0 thì bỏ hẳn các vòng lọc: tấm kính sẽ lấy mẫu thẳng hậu cảnh
    // sắc nét (xem UiGlassPanel), cho cảm giác kính trong thay vì kính mờ.
    if (G.blurAmount <= 0.01f) return;

    float tx = 1.0f / (float)G.blurA.texture.width;
    float ty = 1.0f / (float)G.blurA.texture.height;

    // Bán kính tăng liên tục theo thanh trượt; số vòng lọc tăng kèm để
    // kernel 5 tap không bị thưa mẫu sinh bóng ma ở bán kính lớn.
    float radius = 0.35f + G.blurAmount * 2.0f;
    int iterations = 1 + (int)(G.blurAmount * 2.99f);

    for (int i = 0; i < iterations; i++) {
        float step = radius * (1.0f + (float)i * 1.35f);
        BlurPass(G.blurA, G.blurB, (Vector2){tx * step, 0.0f});
        BlurPass(G.blurB, G.blurA, (Vector2){0.0f, ty * step});
    }
}

Texture2D UiGlassBlurredTexture(void)
{
    return G.shaderReady ? G.blurA.texture : G.backdrop.texture;
}

// ----------------------------------------------------------------- Preset

// Mức kéo cuối cùng của một preset sau khi áp hệ số người dùng.
static float LevelOf(float base)
{
    float v = base * G.levelScale;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return v;
}

UiGlassStyle UiGlassStyleWindow(void)
{
    return (UiGlassStyle){
        .targetLum = 0.30f, .level = LevelOf(0.94f), .saturation = 1.10f,
        .tint = BLANK, .tintStrength = 0.0f,
        .refraction = 16.0f, .edgeWidth = 22.0f,
        .highlight = 0.85f, .innerShadow = 0.8f, .alpha = 0.97f
    };
}

UiGlassStyle UiGlassStylePanel(void)
{
    return (UiGlassStyle){
        .targetLum = 0.34f, .level = LevelOf(1.0f), .saturation = 1.12f,
        .tint = BLANK, .tintStrength = 0.0f,
        .refraction = 10.0f, .edgeWidth = 14.0f,
        .highlight = 0.95f, .innerShadow = 0.7f, .alpha = 0.95f
    };
}

UiGlassStyle UiGlassStyleRaised(void)
{
    return (UiGlassStyle){
        .targetLum = 0.48f, .level = LevelOf(1.0f), .saturation = 1.14f,
        .tint = BLANK, .tintStrength = 0.0f,
        .refraction = 8.0f, .edgeWidth = 10.0f,
        .highlight = 1.15f, .innerShadow = 0.55f, .alpha = 0.93f
    };
}

UiGlassStyle UiGlassStyleSunken(void)
{
    return (UiGlassStyle){
        .targetLum = 0.18f, .level = LevelOf(1.0f), .saturation = 1.05f,
        .tint = BLANK, .tintStrength = 0.0f,
        .refraction = 5.0f, .edgeWidth = 8.0f,
        // Giữ highlight đủ mạnh: vùng lõm mà không thấy mép sẽ trông như
        // một hình chữ nhật đặc bị dán vào, không ra chất kính.
        .highlight = 0.85f, .innerShadow = 1.3f, .alpha = 0.92f
    };
}

UiGlassStyle UiGlassStyleAccent(Color accent)
{
    // Bề mặt màu nhấn là ngoại lệ có chủ đích: nút bấm phải giữ đúng màu
    // của game bất kể hậu cảnh sáng hay tối, nên ở đây mới dùng tint.
    return (UiGlassStyle){
        .targetLum = 0.46f, .level = LevelOf(1.0f), .saturation = 1.0f,
        .tint = accent, .tintStrength = 0.80f,
        .refraction = 9.0f, .edgeWidth = 11.0f,
        .highlight = 1.25f, .innerShadow = 0.5f, .alpha = 0.97f
    };
}

// -------------------------------------------------------------------- Vẽ

// Đường dự phòng khi GPU không biên dịch được shader: vẫn ra một giao diện
// bo góc, bán trong suốt, chỉ thiếu phần khúc xạ và highlight động.
static void DrawFallbackPanel(Rectangle rec, float radius, UiGlassStyle style)
{
    float roundness = (rec.height > 0.0f) ? (radius * 2.0f) / fminf(rec.width, rec.height) : 0.2f;
    if (roundness > 1.0f) roundness = 1.0f;

    // Không có shader thì không lấy mẫu được hậu cảnh; xấp xỉ bằng một lớp
    // xám trung tính đúng độ sáng đích, vẫn không áp màu nào lên giao diện.
    unsigned char v = (unsigned char)(style.targetLum * 255.0f);
    Color fill = (Color){v, v, v, (unsigned char)(style.alpha * style.level * 235.0f)};
    DrawRectangleRounded(rec, roundness, 12, fill);
    if (style.tintStrength > 0.01f) {
        DrawRectangleRounded(rec, roundness, 12, UiAlpha(style.tint, style.tintStrength));
    }
    DrawRectangleRounded((Rectangle){rec.x, rec.y, rec.width, rec.height * 0.5f},
                         roundness, 12, UiAlpha(WHITE, 0.05f * style.highlight));
    DrawRectangleRoundedLinesEx(rec, roundness, 12, 1.2f, UiAlpha(UI.strokeSoft, style.highlight));
}

static void SetPanelUniforms(Rectangle quad, Rectangle rec, float radius,
                             UiGlassStyle style, int sampleMode, int flipY)
{
    Vector2 quadPos  = {quad.x, quad.y};
    Vector2 quadSize = {quad.width, quad.height};
    Vector2 res      = {(float)G.width, (float)G.height};
    Vector2 center   = {rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f};
    Vector2 half     = {rec.width * 0.5f, rec.height * 0.5f};

    Vector4 tint = ColorToVec4(style.tint);
    tint.w = style.tintStrength;

    Shader s = G.panelShader;
    SetShaderValue(s, G.panelLoc.quadPos,     &quadPos,  SHADER_UNIFORM_VEC2);
    SetShaderValue(s, G.panelLoc.quadSize,    &quadSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(s, G.panelLoc.resolution,  &res,      SHADER_UNIFORM_VEC2);
    SetShaderValue(s, G.panelLoc.rectCenter,  &center,   SHADER_UNIFORM_VEC2);
    SetShaderValue(s, G.panelLoc.rectHalf,    &half,     SHADER_UNIFORM_VEC2);
    SetShaderValue(s, G.panelLoc.radius,      &radius,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.tint,        &tint,     SHADER_UNIFORM_VEC4);
    SetShaderValue(s, G.panelLoc.refraction,  &style.refraction,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.edgeWidth,   &style.edgeWidth,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.highlight,   &style.highlight,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.innerShadow, &style.innerShadow, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.targetLum,   &style.targetLum,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.level,       &style.level,       SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.saturation,  &style.saturation,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.alpha,       &style.alpha,       SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, G.panelLoc.sampleMode,  &sampleMode, SHADER_UNIFORM_INT);
    SetShaderValue(s, G.panelLoc.flipY,       &flipY,      SHADER_UNIFORM_INT);
}

void UiGlassPanel(Rectangle rec, float radius, UiGlassStyle style)
{
    if (rec.width <= 0.0f || rec.height <= 0.0f) return;

    if (!UiGlassIsShaderReady()) {
        DrawFallbackPanel(rec, radius, style);
        return;
    }

    // Nới quad thêm vài pixel để dải khử răng cưa của SDF không bị cắt cụt.
    const float pad = 2.0f;
    Rectangle quad = {rec.x - pad, rec.y - pad, rec.width + pad * 2.0f, rec.height + pad * 2.0f};

    Texture2D src = (G.blurAmount <= 0.01f) ? G.backdrop.texture : G.blurA.texture;
    BeginShaderMode(G.panelShader);
        SetPanelUniforms(quad, rec, radius, style, 0, 1);
        DrawTexturePro(src,
                       (Rectangle){0, 0, (float)src.width, (float)src.height},
                       quad, (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

void UiGlassImagePanel(Rectangle rec, float radius, Texture2D tex, bool flipY, UiGlassStyle style)
{
    if (rec.width <= 0.0f || rec.height <= 0.0f) return;

    if (!UiGlassIsShaderReady() || !IsTextureValid(tex)) {
        DrawFallbackPanel(rec, radius, style);
        return;
    }

    const float pad = 2.0f;
    Rectangle quad = {rec.x - pad, rec.y - pad, rec.width + pad * 2.0f, rec.height + pad * 2.0f};

    BeginShaderMode(G.panelShader);
        SetPanelUniforms(quad, rec, radius, style, 1, flipY ? 1 : 0);
        DrawTexturePro(tex,
                       (Rectangle){0, 0, (float)tex.width, (float)tex.height},
                       quad, (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

static void DrawStrokeQuad(Rectangle rec, float radius, float thickness,
                           float spread, int mode, Color color, float expand)
{
    Rectangle quad = {rec.x - expand, rec.y - expand,
                      rec.width + expand * 2.0f, rec.height + expand * 2.0f};

    Vector2 quadPos  = {quad.x, quad.y};
    Vector2 quadSize = {quad.width, quad.height};
    Vector2 center   = {rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f};
    Vector2 half     = {rec.width * 0.5f, rec.height * 0.5f};
    Vector4 col      = ColorToVec4(color);

    Shader s = G.strokeShader;
    BeginShaderMode(s);
        SetShaderValue(s, G.strokeLoc.quadPos,    &quadPos,   SHADER_UNIFORM_VEC2);
        SetShaderValue(s, G.strokeLoc.quadSize,   &quadSize,  SHADER_UNIFORM_VEC2);
        SetShaderValue(s, G.strokeLoc.rectCenter, &center,    SHADER_UNIFORM_VEC2);
        SetShaderValue(s, G.strokeLoc.rectHalf,   &half,      SHADER_UNIFORM_VEC2);
        SetShaderValue(s, G.strokeLoc.radius,     &radius,    SHADER_UNIFORM_FLOAT);
        SetShaderValue(s, G.strokeLoc.thickness,  &thickness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(s, G.strokeLoc.spread,     &spread,    SHADER_UNIFORM_FLOAT);
        SetShaderValue(s, G.strokeLoc.mode,       &mode,      SHADER_UNIFORM_INT);
        SetShaderValue(s, G.strokeLoc.color,      &col,       SHADER_UNIFORM_VEC4);
        DrawTexturePro(G.whitePixel, (Rectangle){0, 0, 1, 1}, quad, (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

void UiGlassStroke(Rectangle rec, float radius, float thickness, Color color)
{
    if (rec.width <= 0.0f || rec.height <= 0.0f) return;

    if (!UiGlassIsShaderReady()) {
        float roundness = (radius * 2.0f) / fminf(rec.width, rec.height);
        if (roundness > 1.0f) roundness = 1.0f;
        DrawRectangleRoundedLinesEx(rec, roundness, 12, thickness, color);
        return;
    }
    DrawStrokeQuad(rec, radius, thickness, 0.0f, 0, color, thickness + 2.0f);
}

void UiGlassGlow(Rectangle rec, float radius, float spread, Color color)
{
    if (!UiGlassIsShaderReady() || rec.width <= 0.0f) return;

    BeginBlendMode(BLEND_ADDITIVE);
        DrawStrokeQuad(rec, radius, 0.0f, spread, 1, color, spread + 2.0f);
    EndBlendMode();
}

void UiGlassShadow(Rectangle rec, float radius, float spread, float alpha)
{
    if (rec.width <= 0.0f || rec.height <= 0.0f) return;

    Color shadow = (Color){6, 14, 32, (unsigned char)(alpha * 255.0f)};
    if (!UiGlassIsShaderReady()) {
        float roundness = (radius * 2.0f) / fminf(rec.width, rec.height);
        if (roundness > 1.0f) roundness = 1.0f;
        DrawRectangleRounded((Rectangle){rec.x, rec.y + 4, rec.width, rec.height}, roundness, 12,
                             UiAlpha(shadow, 0.5f));
        return;
    }
    // Đẩy bóng xuống dưới một chút để nguồn sáng nhất quán với highlight.
    Rectangle shifted = {rec.x, rec.y + spread * 0.22f, rec.width, rec.height};
    DrawStrokeQuad(shifted, radius, 0.0f, spread, 2, shadow, spread + 2.0f);
}
