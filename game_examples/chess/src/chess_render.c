#include "chess_render.h"
#include "chess_assets.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Mô hình "Chess Set" của Poly Haven: một file glTF chứa bàn cờ và đủ 32 quân
// đặt sẵn ở thế khởi đầu. raylib nạp mỗi primitive thành một Mesh đã nhân sẵn
// ma trận node, nên ta nhận diện từng mesh theo khối bao (chiều cao -> loại
// quân, phía z -> màu) rồi dời nó về gốc toạ độ khi vẽ. Nhờ vậy không phụ
// thuộc vào thứ tự node trong file.
// ---------------------------------------------------------------------------

#define MODEL_SQUARE   0.0578816f                 // Cạnh ô cờ trong file gốc (mét)
#define MODEL_TOP      0.0173926f                 // Mặt bàn trong file gốc
#define WORLD_SCALE    (1.0f / MODEL_SQUARE)      // Đổi sang 1 ô = 1 đơn vị
#define BOARD_TOP      (MODEL_TOP * WORLD_SCALE)

#define SHADOW_SIZE_HIGH 2048
#define SHADOW_SIZE_LOW  1024
#define SSAA_HIGH        2.0f
#define SSAA_LOW         1.0f

// Dời khung hình sang trái để bàn cờ nằm giữa vùng không bị bảng bên phải che.
#define LENS_SHIFT       0.26f
#define CAMERA_FOVY      34.0f

typedef struct {
    int meshes[2];
    int meshCount;
    Vector3 origin;             // Vị trí gốc của quân trong file (để dời về 0)
    float height;               // Chiều cao (đơn vị thế giới) - dùng khi bắt chuột
} PieceTemplate;

typedef struct {
    int refCount;
    bool hasModel;
    Model model;
    int boardMesh;
    PieceTemplate pieces[2][CHESS_PIECE_TYPES];

    Texture2D boardDiff, boardNor, boardArm;
    Texture2D pieceDiff[2], pieceNor[2], pieceArm[2];
    Texture2D tableDiff;
    Texture2D softSquare, disc, ring, glow;

    Shader lit;
    bool litReady;
    int locViewPos, locLightDir, locLightColor, locFillDir, locFillColor, locSky, locGround;
    int locFogColor, locFogDensity, locFogStart, locUseNormal, locUseArm, locUseShadow, locShadowTexel;
    int locUvScale, locRoughness, locEnv, locHighlight, locExposure, locLightVP;

    Material boardMat, pieceMat[2], tableMat, depthMat;
    Mesh tableMesh;

    RenderTexture2D shadow;
    int shadowSize;
    bool shadowReady;
    RenderTexture2D scene;
    float ssaa;
    bool highQuality;
} RenderState;

static RenderState R = {0};

static const Vector3 LIGHT_DIR  = {0.42f, -0.80f, 0.30f};    // Hướng ánh sáng chính (đi xuống, chếch)
static const Color   BG_TOP     = {30, 32, 40, 255};
static const Color   BG_BOTTOM  = {9, 10, 13, 255};

// ------------------------------------------------------------------ hình học công khai

float ChessBoardTop(void) { return BOARD_TOP; }

Vector3 ChessSquareWorld(int sq)
{
    // Cột a nằm bên trái người cầm trắng (người cầm trắng ngồi phía -z nhìn về +z).
    float x = (3.5f - (float)CHESS_FILE(sq)) * CHESS_SQUARE_SIZE;
    float z = ((float)CHESS_RANK(sq) - 3.5f) * CHESS_SQUARE_SIZE;
    return (Vector3){x, BOARD_TOP, z};
}

Vector3 ChessGraveWorld(int color, int index, int viewer)
{
    // Quân bị bắt xếp trên mặt bàn gỗ, phía TAY TRÁI người xem - phía phải
    // đã bị bảng thông tin che. Quân đối phương mình ăn được nằm ở nửa gần
    // mình, quân mình bị ăn nằm ở nửa xa. Mỗi nửa 3 cột x 5 hàng (đủ 15 quân).
    int col = index / 5, row = index % 5;
    float left = (viewer == CHESS_WHITE) ? 1.0f : -1.0f;      // Tay trái người cầm trắng là +x
    float near = (viewer == CHESS_WHITE) ? -1.0f : 1.0f;      // Phía người xem ngồi
    float half = (color != viewer) ? near : -near;
    float x = left * (5.45f + col * 0.72f);
    float z = half * (0.45f + row * 0.78f);
    return (Vector3){x, 0.0f, z};
}

Camera3D ChessCameraFromRig(const ChessCameraRig *rig)
{
    Camera3D cam = {0};
    cam.target = (Vector3){0.0f, BOARD_TOP, 0.0f};
    float cp = cosf(rig->pitch), sp = sinf(rig->pitch);
    cam.position = (Vector3){
        cam.target.x + sinf(rig->yaw) * cp * rig->distance,
        cam.target.y + sp * rig->distance,
        cam.target.z - cosf(rig->yaw) * cp * rig->distance
    };
    cam.up = (Vector3){0.0f, 1.0f, 0.0f};
    cam.fovy = CAMERA_FOVY;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}

static Matrix SceneProjection(void)
{
    double aspect = (double)CHESS_CANVAS_W / (double)CHESS_CANVAS_H;
    double nearP = 0.1, farP = 200.0;
    double top = nearP * tan(CAMERA_FOVY * 0.5 * DEG2RAD);
    double right = top * aspect;
    double shift = right * LENS_SHIFT;
    return MatrixFrustum(-right + shift, right + shift, -top, top, nearP, farP);
}

Ray ChessScreenRay(const ChessGame *game, Vector2 p)
{
    Camera3D cam = ChessCameraFromRig(&game->cam);
    Matrix view = MatrixLookAt(cam.position, cam.target, cam.up);
    Matrix proj = SceneProjection();
    float x = 2.0f * p.x / CHESS_CANVAS_W - 1.0f;
    float y = 1.0f - 2.0f * p.y / CHESS_CANVAS_H;
    Vector3 nearP = Vector3Unproject((Vector3){x, y, 0.0f}, proj, view);
    Vector3 farP = Vector3Unproject((Vector3){x, y, 1.0f}, proj, view);
    Ray ray = {cam.position, Vector3Normalize(Vector3Subtract(farP, nearP))};
    return ray;
}

Vector2 ChessWorldToCanvas(const ChessGame *game, Vector3 w, bool *visible)
{
    Camera3D cam = ChessCameraFromRig(&game->cam);
    Matrix vp = MatrixMultiply(MatrixLookAt(cam.position, cam.target, cam.up), SceneProjection());
    float cx = vp.m0 * w.x + vp.m4 * w.y + vp.m8 * w.z + vp.m12;
    float cy = vp.m1 * w.x + vp.m5 * w.y + vp.m9 * w.z + vp.m13;
    float cw = vp.m3 * w.x + vp.m7 * w.y + vp.m11 * w.z + vp.m15;
    if (visible) *visible = cw > 0.0001f;
    if (cw <= 0.0001f) return (Vector2){-1000.0f, -1000.0f};
    return (Vector2){(cx / cw + 1.0f) * 0.5f * CHESS_CANVAS_W, (1.0f - cy / cw) * 0.5f * CHESS_CANVAS_H};
}

static float PieceHeight(int type)
{
    static const float H[CHESS_PIECE_TYPES] = {0.0f, 0.93f, 1.30f, 1.48f, 1.05f, 1.56f, 1.64f};
    return (type > 0 && type < CHESS_PIECE_TYPES) ? H[type] : 1.0f;
}

// Tia chuột cắt khối trụ bao quanh quân (trúng đầu quân cao phía sau vẫn chọn đúng).
static bool RayHitsPiece(Ray ray, Vector3 base, float height, float radius, float *tOut)
{
    float ox = ray.position.x - base.x, oz = ray.position.z - base.z;
    float a = ray.direction.x * ray.direction.x + ray.direction.z * ray.direction.z;
    float b = 2.0f * (ray.direction.x * ox + ray.direction.z * oz);
    float c = ox * ox + oz * oz - radius * radius;
    float best = -1.0f;
    if (a > 1e-6f) {
        float disc = b * b - 4.0f * a * c;
        if (disc >= 0.0f) {
            float t = (-b - sqrtf(disc)) / (2.0f * a);
            float y = ray.position.y + ray.direction.y * t;
            if (t > 0.0f && y >= base.y && y <= base.y + height) best = t;
        }
    }
    if (fabsf(ray.direction.y) > 1e-6f) {
        float t = (base.y + height - ray.position.y) / ray.direction.y;
        float px = ox + ray.direction.x * t, pz = oz + ray.direction.z * t;
        if (t > 0.0f && px * px + pz * pz <= radius * radius && (best < 0.0f || t < best)) best = t;
    }
    if (best < 0.0f) return false;
    *tOut = best;
    return true;
}

int ChessPickSquare(const ChessGame *game, Vector2 p)
{
    Ray ray = ChessScreenRay(game, p);
    float bestT = 1e9f;
    int bestSq = -1;
    for (int i = 0; i < CHESS_MAX_VISUALS; i++) {
        const ChessVisual *v = &game->visuals[i];
        if (!v->alive || v->captured || v->square < 0) continue;
        float t;
        if (RayHitsPiece(ray, v->pos, PieceHeight(v->type), 0.36f, &t) && t < bestT) {
            bestT = t;
            bestSq = v->square;
        }
    }
    if (bestSq >= 0) return bestSq;

    if (fabsf(ray.direction.y) < 1e-6f) return -1;
    float t = (BOARD_TOP - ray.position.y) / ray.direction.y;
    if (t <= 0.0f) return -1;
    float x = ray.position.x + ray.direction.x * t;
    float z = ray.position.z + ray.direction.z * t;
    int file = (int)floorf(3.5f - x + 0.5f);
    int rank = (int)floorf(z + 3.5f + 0.5f);
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return -1;
    return CHESS_SQ(file, rank);
}

// ------------------------------------------------------------------ shader

static const char *VS_330 = "#version 330\n#define ATTR in\n#define VARY out\n";
static const char *VS_120 = "#version 120\n#define ATTR attribute\n#define VARY varying\n";
static const char *VS_300ES = "#version 300 es\nprecision highp float;\n#define ATTR in\n#define VARY out\n";
static const char *VS_100 = "#version 100\nprecision highp float;\n#define ATTR attribute\n#define VARY varying\n";

static const char *FS_330 = "#version 330\n#define VARY in\nout vec4 finalColor;\n";
static const char *FS_120 = "#version 120\n#define VARY varying\n#define texture texture2D\n#define finalColor gl_FragColor\n";
static const char *FS_300ES = "#version 300 es\nprecision highp float;\n#define VARY in\nout vec4 finalColor;\n";
static const char *FS_100 =
    "#version 100\n#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n"
    "#define VARY varying\n#define texture texture2D\n#define finalColor gl_FragColor\n";

static const char *LIT_VS =
"ATTR vec3 vertexPosition;\n"
"ATTR vec2 vertexTexCoord;\n"
"ATTR vec3 vertexNormal;\n"
"ATTR vec4 vertexTangent;\n"
"uniform mat4 mvp;\n"
"uniform mat4 matModel;\n"
"uniform mat4 matNormal;\n"
"uniform mat4 lightVP;\n"
"VARY vec3 fragPos;\n"
"VARY vec2 fragTexCoord;\n"
"VARY vec3 fragNormal;\n"
"VARY vec4 fragTangent;\n"
"VARY vec4 fragLightPos;\n"
"void main() {\n"
"    vec4 world = matModel * vec4(vertexPosition, 1.0);\n"
"    mat3 nm = mat3(matNormal[0].xyz, matNormal[1].xyz, matNormal[2].xyz);\n"
"    mat3 mm = mat3(matModel[0].xyz, matModel[1].xyz, matModel[2].xyz);\n"
"    fragPos = world.xyz;\n"
"    fragTexCoord = vertexTexCoord;\n"
"    fragNormal = normalize(nm * vertexNormal);\n"
"    fragTangent = vec4(normalize(mm * vertexTangent.xyz + vec3(1e-5)), vertexTangent.w);\n"
"    fragLightPos = lightVP * world;\n"
"    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
"}\n";

// PBR kim loại/độ nhám (GGX), một đèn chính có bóng đổ PCF 5x5, một đèn phụ,
// ánh sáng nền hai bán cầu, phản chiếu môi trường giả lập theo Fresnel,
// tone map ACES và sương mù hoà vào nền.
static const char *LIT_FS =
"VARY vec3 fragPos;\n"
"VARY vec2 fragTexCoord;\n"
"VARY vec3 fragNormal;\n"
"VARY vec4 fragTangent;\n"
"VARY vec4 fragLightPos;\n"
"uniform sampler2D texture0;\n"
"uniform sampler2D texture1;\n"
"uniform sampler2D texture2;\n"
"uniform sampler2D shadowMap;\n"
"uniform vec4 colDiffuse;\n"
"uniform vec3 viewPos;\n"
"uniform vec3 lightDir;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 fillDir;\n"
"uniform vec3 fillColor;\n"
"uniform vec3 skyColor;\n"
"uniform vec3 groundColor;\n"
"uniform vec3 fogColor;\n"
"uniform float fogDensity;\n"
"uniform float fogStart;\n"
"uniform float useNormalMap;\n"
"uniform float useArm;\n"
"uniform float useShadow;\n"
"uniform float shadowTexel;\n"
"uniform float uvScale;\n"
"uniform float roughness;\n"
"uniform float envStrength;\n"
"uniform vec4 highlight;\n"
"uniform float exposure;\n"
"const float PI = 3.14159265;\n"
"float ShadowFactor(float ndl) {\n"
"    if (useShadow < 0.5) return 1.0;\n"
"    vec3 p = fragLightPos.xyz / fragLightPos.w;\n"
"    p = p * 0.5 + 0.5;\n"
"    if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z > 1.0) return 1.0;\n"
"    float bias = max(0.0022 * (1.0 - ndl), 0.0007);\n"
"    float lit = 0.0;\n"
"    for (int x = -2; x <= 2; x++) {\n"
"        for (int y = -2; y <= 2; y++) {\n"
"            float d = texture(shadowMap, p.xy + vec2(float(x), float(y)) * shadowTexel).r;\n"
"            lit += (p.z - bias > d) ? 0.0 : 1.0;\n"
"        }\n"
"    }\n"
"    return lit / 25.0;\n"
"}\n"
"vec3 Brdf(vec3 N, vec3 V, vec3 L, vec3 albedo, vec3 F0, float rough, float metal) {\n"
"    vec3 H = normalize(V + L);\n"
"    float NdotL = max(dot(N, L), 0.0001);\n"
"    float NdotV = max(dot(N, V), 0.0001);\n"
"    float NdotH = max(dot(N, H), 0.0);\n"
"    float VdotH = max(dot(V, H), 0.0);\n"
"    float a = rough * rough;\n"
"    float a2 = a * a;\n"
"    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;\n"
"    float D = a2 / (PI * d * d);\n"
"    float k = (rough + 1.0) * (rough + 1.0) / 8.0;\n"
"    float G = (NdotV / (NdotV * (1.0 - k) + k)) * (NdotL / (NdotL * (1.0 - k) + k));\n"
"    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);\n"
"    vec3 spec = D * G * F / (4.0 * NdotV * NdotL + 0.0001);\n"
"    vec3 kd = (vec3(1.0) - F) * (1.0 - metal);\n"
"    return kd * albedo / PI + spec;\n"
"}\n"
"void main() {\n"
"    vec2 uv = fragTexCoord * uvScale;\n"
"    vec4 tex = texture(texture0, uv);\n"
"    vec3 albedo = pow(tex.rgb * colDiffuse.rgb, vec3(2.2));\n"
"    vec3 N = normalize(fragNormal);\n"
"    if (useNormalMap > 0.5) {\n"
"        vec3 T = normalize(fragTangent.xyz - N * dot(N, fragTangent.xyz));\n"
"        vec3 B = cross(N, T) * fragTangent.w;\n"
"        vec3 nm = texture(texture2, uv).xyz * 2.0 - 1.0;\n"
"        N = normalize(T * nm.x + B * nm.y + N * nm.z);\n"
"    }\n"
"    float ao = 1.0;\n"
"    float rough = roughness;\n"
"    float metal = 0.0;\n"
"    if (useArm > 0.5) {\n"
"        vec3 arm = texture(texture1, uv).rgb;\n"
"        ao = arm.r;\n"
"        rough = arm.g * roughness;\n"
"        metal = arm.b;\n"
"    }\n"
"    rough = clamp(rough, 0.06, 1.0);\n"
"    vec3 V = normalize(viewPos - fragPos);\n"
"    float NdotV = max(dot(N, V), 0.001);\n"
"    vec3 F0 = mix(vec3(0.04), albedo, metal);\n"
"    vec3 L = normalize(-lightDir);\n"
"    float ndl = max(dot(N, L), 0.0);\n"
"    vec3 color = Brdf(N, V, L, albedo, F0, rough, metal) * lightColor * ndl * ShadowFactor(ndl);\n"
"    vec3 L2 = normalize(-fillDir);\n"
"    color += Brdf(N, V, L2, albedo, F0, rough, metal) * fillColor * max(dot(N, L2), 0.0);\n"
"    vec3 amb = mix(groundColor, skyColor, N.y * 0.5 + 0.5);\n"
"    color += amb * albedo * (1.0 - metal) * ao;\n"
"    vec3 R = reflect(-V, N);\n"
"    vec3 env = mix(groundColor * 0.5, skyColor * 2.2, smoothstep(-0.15, 0.75, R.y));\n"
"    env += vec3(1.6, 1.5, 1.4) * smoothstep(0.93, 0.99, dot(R, normalize(vec3(-0.35, 0.8, -0.45))));\n"
"    vec3 Fr = F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(1.0 - NdotV, 5.0);\n"
"    color += env * Fr * envStrength * ao * (1.0 - rough * 0.8);\n"
"    float rim = pow(1.0 - NdotV, 2.0);\n"
"    color += highlight.rgb * highlight.a * (0.25 + 1.4 * rim);\n"
"    color *= exposure;\n"
"    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);\n"
"    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));\n"
"    float dist = length(viewPos - fragPos);\n"
"    float fog = exp(-pow(fogDensity * max(dist - fogStart, 0.0), 2.0));\n"
"    color = mix(fogColor, color, fog);\n"
"    finalColor = vec4(color, tex.a * colDiffuse.a);\n"
"}\n";

static char *Concat(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);
    char *out = (char *)MemAlloc((unsigned int)(la + lb + 1));
    memcpy(out, a, la);
    memcpy(out + la, b, lb + 1);
    return out;
}

static void LoadLitShader(void)
{
    const char *vh = VS_330, *fh = FS_330;
    switch (rlGetVersion()) {
        case RL_OPENGL_ES_20: vh = VS_100; fh = FS_100; break;
        case RL_OPENGL_ES_30: vh = VS_300ES; fh = FS_300ES; break;
        case RL_OPENGL_21:    vh = VS_120; fh = FS_120; break;
        default: break;
    }
    char *vs = Concat(vh, LIT_VS);
    char *fs = Concat(fh, LIT_FS);
    R.lit = LoadShaderFromMemory(vs, fs);
    MemFree(vs);
    MemFree(fs);

    R.litReady = R.lit.id > 0 && R.lit.id != rlGetShaderIdDefault();
    if (!R.litReady) {
        TraceLog(LOG_WARNING, "CHESS: shader ánh sáng không biên dịch được, dùng shader mặc định");
        R.lit = (Shader){rlGetShaderIdDefault(), rlGetShaderLocsDefault()};
        return;
    }
    Shader s = R.lit;
    s.locs[SHADER_LOC_MAP_BRDF] = GetShaderLocation(s, "shadowMap");
    R.locViewPos     = GetShaderLocation(s, "viewPos");
    R.locLightDir    = GetShaderLocation(s, "lightDir");
    R.locLightColor  = GetShaderLocation(s, "lightColor");
    R.locFillDir     = GetShaderLocation(s, "fillDir");
    R.locFillColor   = GetShaderLocation(s, "fillColor");
    R.locSky         = GetShaderLocation(s, "skyColor");
    R.locGround      = GetShaderLocation(s, "groundColor");
    R.locFogColor    = GetShaderLocation(s, "fogColor");
    R.locFogDensity  = GetShaderLocation(s, "fogDensity");
    R.locFogStart    = GetShaderLocation(s, "fogStart");
    R.locUseNormal   = GetShaderLocation(s, "useNormalMap");
    R.locUseArm      = GetShaderLocation(s, "useArm");
    R.locUseShadow   = GetShaderLocation(s, "useShadow");
    R.locShadowTexel = GetShaderLocation(s, "shadowTexel");
    R.locUvScale     = GetShaderLocation(s, "uvScale");
    R.locRoughness   = GetShaderLocation(s, "roughness");
    R.locEnv         = GetShaderLocation(s, "envStrength");
    R.locHighlight   = GetShaderLocation(s, "highlight");
    R.locExposure    = GetShaderLocation(s, "exposure");
    R.locLightVP     = GetShaderLocation(s, "lightVP");
}

static void SetF(int loc, float v) { if (R.litReady) SetShaderValue(R.lit, loc, &v, SHADER_UNIFORM_FLOAT); }
static void SetV3(int loc, Vector3 v) { if (R.litReady) SetShaderValue(R.lit, loc, &v, SHADER_UNIFORM_VEC3); }
static void SetV4(int loc, Vector4 v) { if (R.litReady) SetShaderValue(R.lit, loc, &v, SHADER_UNIFORM_VEC4); }

// ------------------------------------------------------------------ texture

static Texture2D LoadTex(const char *relative, bool repeat)
{
    const char *path = ChessAssetPath(relative);
    Texture2D t = {0};
    if (FileExists(path)) t = LoadTexture(path);
    if (t.id == 0) {
        TraceLog(LOG_WARNING, "CHESS: thiếu texture '%s'", path);
        Image img = GenImageColor(4, 4, (Color){128, 128, 255, 255});
        t = LoadTextureFromImage(img);
        UnloadImage(img);
        return t;
    }
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_ANISOTROPIC_8X);
    SetTextureWrap(t, repeat ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);
    return t;
}

// Texture phủ ô cờ: vẽ bằng hàm khoảng cách để mép luôn mềm.
typedef enum { OVL_SOFT_SQUARE, OVL_DISC, OVL_RING, OVL_GLOW } OverlayShape;

static Texture2D MakeOverlay(OverlayShape shape)
{
    const int size = 128;
    Image img = GenImageColor(size, size, BLANK);
    Color *px = (Color *)img.data;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = ((float)x + 0.5f) / size * 2.0f - 1.0f;
            float v = ((float)y + 0.5f) / size * 2.0f - 1.0f;
            float a = 0.0f;
            float r = sqrtf(u * u + v * v);
            switch (shape) {
                case OVL_SOFT_SQUARE: {
                    float d = fmaxf(fabsf(u), fabsf(v));
                    a = Clamp((0.97f - d) / 0.05f, 0.0f, 1.0f);
                    break;
                }
                case OVL_DISC:  a = Clamp((0.62f - r) / 0.06f, 0.0f, 1.0f); break;
                case OVL_RING:  a = Clamp((0.93f - r) / 0.05f, 0.0f, 1.0f) * Clamp((r - 0.74f) / 0.05f, 0.0f, 1.0f); break;
                case OVL_GLOW:  a = powf(Clamp(1.0f - r, 0.0f, 1.0f), 1.6f); break;
            }
            px[y * size + x] = (Color){255, 255, 255, (unsigned char)(a * 255.0f)};
        }
    }
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    return t;
}

// ------------------------------------------------------------------ mô hình

static void ClassifyModel(void)
{
    Model *m = &R.model;
    R.boardMesh = -1;
    int accessories[8];
    int accessoryCount = 0;

    for (int i = 0; i < m->meshCount; i++) {
        BoundingBox bb = GetMeshBoundingBox(m->meshes[i]);
        float w = bb.max.x - bb.min.x;
        if (w > MODEL_SQUARE * 4.0f) {
            R.boardMesh = i;
            continue;
        }
        float lift = bb.min.y - MODEL_TOP;
        if (lift > 0.03f) {                           // Chi tiết rời phía trên (khe mũ tượng)
            if (accessoryCount < 8) accessories[accessoryCount++] = i;
            continue;
        }
        float h = bb.max.y - MODEL_TOP;
        int type;
        if (h < 0.057f) type = CHESS_PAWN;
        else if (h < 0.068f) type = CHESS_ROOK;
        else if (h < 0.080f) type = CHESS_KNIGHT;
        else if (h < 0.088f) type = CHESS_BISHOP;
        else if (h < 0.0928f) type = CHESS_QUEEN;
        else type = CHESS_KING;

        float cx = (bb.min.x + bb.max.x) * 0.5f, cz = (bb.min.z + bb.max.z) * 0.5f;
        int color = (cz < 0.0f) ? CHESS_WHITE : CHESS_BLACK;
        PieceTemplate *t = &R.pieces[color][type];
        if (t->meshCount > 0) continue;              // Đã có mẫu cho loại này
        t->meshes[0] = i;
        t->meshCount = 1;
        // Khối bao của mã lệch về phía đầu ngựa: bắt tâm về ô gần nhất.
        t->origin = (Vector3){
            (floorf(cx / MODEL_SQUARE) + 0.5f) * MODEL_SQUARE,
            MODEL_TOP,
            (floorf(cz / MODEL_SQUARE) + 0.5f) * MODEL_SQUARE
        };
        t->height = h * WORLD_SCALE;
    }

    // Gắn chi tiết rời vào đúng quân tượng mẫu đứng cùng ô.
    for (int a = 0; a < accessoryCount; a++) {
        BoundingBox bb = GetMeshBoundingBox(m->meshes[accessories[a]]);
        float cx = (bb.min.x + bb.max.x) * 0.5f, cz = (bb.min.z + bb.max.z) * 0.5f;
        for (int c = 0; c < 2; c++) {
            for (int type = 1; type < CHESS_PIECE_TYPES; type++) {
                PieceTemplate *t = &R.pieces[c][type];
                if (t->meshCount != 1) continue;
                if (fabsf(t->origin.x - cx) < MODEL_SQUARE * 0.5f && fabsf(t->origin.z - cz) < MODEL_SQUARE * 0.5f) {
                    t->meshes[t->meshCount++] = accessories[a];
                }
            }
        }
    }

    bool complete = R.boardMesh >= 0;
    for (int c = 0; c < 2; c++)
        for (int type = 1; type < CHESS_PIECE_TYPES; type++)
            if (R.pieces[c][type].meshCount == 0) complete = false;
    if (!complete) {
        TraceLog(LOG_WARNING, "CHESS: không nhận diện đủ quân trong mô hình, dùng hình dự phòng");
        R.hasModel = false;
    }
}

static void ReleaseModelTextures(void)
{
    // Dùng bộ texture tự nạp (board 2K) nên bỏ các texture raylib nạp kèm glTF.
    // raylib cấp phát đúng 12 map cho mỗi material (MAX_MATERIAL_MAPS trong rmodels.c).
    for (int i = 0; i < R.model.materialCount; i++) {
        for (int k = 0; k < 12; k++) {
            Texture2D *t = &R.model.materials[i].maps[k].texture;
            if (t->id > 0 && t->id != rlGetTextureIdDefault()) UnloadTexture(*t);
            t->id = rlGetTextureIdDefault();
        }
    }
}

static Material MakeLitMaterial(Texture2D diff, Texture2D arm, Texture2D nor)
{
    Material mat = LoadMaterialDefault();
    mat.shader = R.lit;
    mat.maps[MATERIAL_MAP_ALBEDO].texture = diff;
    mat.maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    if (arm.id > 0) mat.maps[MATERIAL_MAP_METALNESS].texture = arm;
    if (nor.id > 0) mat.maps[MATERIAL_MAP_NORMAL].texture = nor;
    return mat;
}

static void FreeMaterial(Material *mat)
{
    // Texture được giải phóng riêng; chỉ trả mảng maps.
    if (mat->maps) MemFree(mat->maps);
    mat->maps = NULL;
}

// ------------------------------------------------------------------ render target

static void UnloadShadowTarget(void)
{
    if (R.shadow.id > 0) {
        if (R.shadow.depth.id > 0) rlUnloadTexture(R.shadow.depth.id);
        rlUnloadFramebuffer(R.shadow.id);
    }
    R.shadow = (RenderTexture2D){0};
    R.shadowReady = false;
}

static void LoadShadowTarget(int size)
{
    UnloadShadowTarget();
    R.shadowSize = size;
    RenderTexture2D t = {0};
    t.id = rlLoadFramebuffer();
    t.texture.width = size;
    t.texture.height = size;
    if (t.id > 0) {
        rlEnableFramebuffer(t.id);
        t.depth.id = rlLoadTextureDepth(size, size, false);
        t.depth.width = size;
        t.depth.height = size;
        t.depth.format = 19;
        t.depth.mipmaps = 1;
        rlFramebufferAttach(t.id, t.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
        R.shadowReady = rlFramebufferComplete(t.id) && t.depth.id > 0;
        rlDisableFramebuffer();
    }
    R.shadow = t;
    if (!R.shadowReady) TraceLog(LOG_WARNING, "CHESS: không tạo được shadow map, tắt bóng đổ");
}

static void LoadSceneTarget(float ssaa)
{
    if (R.scene.id > 0) UnloadRenderTexture(R.scene);
    R.ssaa = ssaa;
    R.scene = LoadRenderTexture((int)(CHESS_CANVAS_W * ssaa), (int)(CHESS_CANVAS_H * ssaa));
    if (R.scene.id == 0 && ssaa > 1.0f) {          // GPU không đủ bộ nhớ: lùi về 1x
        R.ssaa = 1.0f;
        R.scene = LoadRenderTexture(CHESS_CANVAS_W, CHESS_CANVAS_H);
    }
    SetTextureFilter(R.scene.texture, TEXTURE_FILTER_BILINEAR);
}

void ChessRenderSetQuality(bool high)
{
    if (R.refCount <= 0) return;
    if (R.scene.id > 0 && R.highQuality == high) return;
    R.highQuality = high;
    LoadSceneTarget(high ? SSAA_HIGH : SSAA_LOW);
    LoadShadowTarget(high ? SHADOW_SIZE_HIGH : SHADOW_SIZE_LOW);
}

// ------------------------------------------------------------------ nạp / giải phóng

void ChessRenderInit(void)
{
    if (R.refCount++ > 0) return;

    LoadLitShader();

    const char *modelPath = ChessAssetPath("model/chess_set.gltf");
    R.hasModel = FileExists(modelPath);
    if (R.hasModel) {
        R.model = LoadModel(modelPath);
        R.hasModel = R.model.meshCount > 0;
    }
    if (R.hasModel) {
        ReleaseModelTextures();
        ClassifyModel();
        for (int i = 0; i < R.model.meshCount; i++) {
            if (R.model.meshes[i].tangents == NULL) GenMeshTangents(&R.model.meshes[i]);
        }
    }

    R.boardDiff    = LoadTex("model/textures/chess_set_board_diff_2k.jpg", false);
    R.boardNor     = LoadTex("model/textures/chess_set_board_nor_gl_1k.jpg", false);
    R.boardArm     = LoadTex("model/textures/chess_set_board_arm_1k.jpg", false);
    R.pieceDiff[0] = LoadTex("model/textures/chess_set_pieces_white_diff_1k.jpg", false);
    R.pieceNor[0]  = LoadTex("model/textures/chess_set_pieces_white_nor_gl_1k.jpg", false);
    R.pieceArm[0]  = LoadTex("model/textures/chess_set_pieces_white_arm_1k.jpg", false);
    R.pieceDiff[1] = LoadTex("model/textures/chess_set_pieces_black_diff_1k.jpg", false);
    R.pieceNor[1]  = LoadTex("model/textures/chess_set_pieces_black_nor_gl_1k.jpg", false);
    R.pieceArm[1]  = LoadTex("model/textures/chess_set_pieces_black_arm_1k.jpg", false);
    R.tableDiff    = LoadTex("textures/wood_table.jpg", true);

    R.softSquare = MakeOverlay(OVL_SOFT_SQUARE);
    R.disc = MakeOverlay(OVL_DISC);
    R.ring = MakeOverlay(OVL_RING);
    R.glow = MakeOverlay(OVL_GLOW);

    R.boardMat = MakeLitMaterial(R.boardDiff, R.boardArm, R.boardNor);
    R.pieceMat[0] = MakeLitMaterial(R.pieceDiff[0], R.pieceArm[0], R.pieceNor[0]);
    R.pieceMat[1] = MakeLitMaterial(R.pieceDiff[1], R.pieceArm[1], R.pieceNor[1]);
    R.tableMat = MakeLitMaterial(R.tableDiff, (Texture2D){0}, (Texture2D){0});
    R.tableMat.maps[MATERIAL_MAP_ALBEDO].color = (Color){150, 132, 122, 255};   // Gỗ trầm hơn, đỡ át bàn cờ
    R.depthMat = LoadMaterialDefault();

    R.tableMesh = GenMeshPlane(70.0f, 70.0f, 1, 1);
    GenMeshTangents(&R.tableMesh);

    R.highQuality = true;
    LoadSceneTarget(SSAA_HIGH);
    LoadShadowTarget(SHADOW_SIZE_HIGH);
}

void ChessRenderClose(void)
{
    if (R.refCount <= 0 || --R.refCount > 0) return;

    if (R.hasModel || R.model.meshCount > 0) UnloadModel(R.model);
    Texture2D texs[] = {
        R.boardDiff, R.boardNor, R.boardArm, R.pieceDiff[0], R.pieceNor[0], R.pieceArm[0],
        R.pieceDiff[1], R.pieceNor[1], R.pieceArm[1], R.tableDiff, R.softSquare, R.disc, R.ring, R.glow
    };
    for (size_t i = 0; i < sizeof(texs) / sizeof(texs[0]); i++) {
        if (texs[i].id > 0) UnloadTexture(texs[i]);
    }
    FreeMaterial(&R.boardMat);
    FreeMaterial(&R.pieceMat[0]);
    FreeMaterial(&R.pieceMat[1]);
    FreeMaterial(&R.tableMat);
    FreeMaterial(&R.depthMat);
    UnloadMesh(R.tableMesh);
    if (R.litReady) UnloadShader(R.lit);
    UnloadShadowTarget();
    if (R.scene.id > 0) UnloadRenderTexture(R.scene);
    memset(&R, 0, sizeof(R));
}

bool ChessRenderHasModel(void)
{
    return R.hasModel;
}

// ------------------------------------------------------------------ vẽ cảnh

static Matrix PieceTransform(const PieceTemplate *t, Vector3 pos, float scale)
{
    Matrix toOrigin = MatrixTranslate(-t->origin.x, -t->origin.y, -t->origin.z);
    Matrix s = MatrixScale(WORLD_SCALE * scale, WORLD_SCALE * scale, WORLD_SCALE * scale);
    Matrix to = MatrixTranslate(pos.x, pos.y, pos.z);
    return MatrixMultiply(MatrixMultiply(toOrigin, s), to);
}

static void DrawPieceFallback(int type, int color, Vector3 pos, float scale)
{
    Color c = (color == CHESS_WHITE) ? (Color){236, 232, 222, 255} : (Color){40, 40, 46, 255};
    float h = PieceHeight(type) * scale;
    DrawCylinder(pos, 0.34f * scale, 0.38f * scale, 0.14f * scale, 24, c);
    DrawCylinder((Vector3){pos.x, pos.y + 0.14f * scale, pos.z}, 0.16f * scale, 0.28f * scale, h * 0.6f, 20, c);
    DrawSphere((Vector3){pos.x, pos.y + h * 0.8f, pos.z}, 0.2f * scale + type * 0.015f, c);
}

static void DrawPiece(int type, int color, Vector3 pos, float scale, Vector4 highlight)
{
    if (!R.hasModel) {
        DrawPieceFallback(type, color, pos, scale);
        return;
    }
    const PieceTemplate *t = &R.pieces[color][type];
    Matrix m = PieceTransform(t, pos, scale);
    SetV4(R.locHighlight, highlight);
    for (int i = 0; i < t->meshCount; i++) DrawMesh(R.model.meshes[t->meshes[i]], R.pieceMat[color], m);
}

static void DrawPieceDepth(int type, int color, Vector3 pos, float scale)
{
    if (!R.hasModel) return;
    const PieceTemplate *t = &R.pieces[color][type];
    Matrix m = PieceTransform(t, pos, scale);
    for (int i = 0; i < t->meshCount; i++) DrawMesh(R.model.meshes[t->meshes[i]], R.depthMat, m);
}

static float VisualScale(const ChessVisual *v)
{
    return v->captured && !v->moving ? 0.78f : 1.0f;
}

// Quad phẳng nằm ngang ngay trên mặt bàn (y cố định), dùng cho các lớp đánh dấu ô.
static void DrawFlatQuad(Texture2D tex, Vector3 c, float size, Color color)
{
    float h = size * 0.5f;
    float y = c.y + 0.004f;
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(c.x - h, y, c.z - h);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(c.x - h, y, c.z + h);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(c.x + h, y, c.z + h);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(c.x + h, y, c.z - h);
    rlEnd();
    rlSetTexture(0);
}

static bool IsLegalTarget(const ChessGame *g, int from, int to, bool *capture)
{
    for (int i = 0; i < g->legalCount; i++) {
        if (CHESS_MOVE_FROM(g->legal[i]) == from && CHESS_MOVE_TO(g->legal[i]) == to) {
            if (capture) *capture = (CHESS_MOVE_FLAGS(g->legal[i]) & CHESS_FLAG_CAPTURE) != 0;
            return true;
        }
    }
    return false;
}

static bool InteractiveState(const ChessGame *g)
{
    return g->state == CHESS_STATE_PLAYING || g->state == CHESS_STATE_PROMOTION;
}

static void DrawOverlays(const ChessGame *g)
{
    float t = g->globalTime;
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ALPHA);

    if (g->lastMove != CHESS_MOVE_NONE && g->state != CHESS_STATE_MENU) {
        Color c = {255, 208, 96, 110};
        DrawFlatQuad(R.softSquare, ChessSquareWorld(CHESS_MOVE_FROM(g->lastMove)), 0.98f, c);
        DrawFlatQuad(R.softSquare, ChessSquareWorld(CHESS_MOVE_TO(g->lastMove)), 0.98f, c);
    }

    if (g->hintMove != CHESS_MOVE_NONE && g->state == CHESS_STATE_PLAYING) {
        float pulse = 0.55f + 0.45f * sinf(g->hintTimer * 6.0f);
        Color c = {110, 240, 150, (unsigned char)(90 + 100 * pulse)};
        DrawFlatQuad(R.softSquare, ChessSquareWorld(CHESS_MOVE_FROM(g->hintMove)), 0.98f, c);
        DrawFlatQuad(R.softSquare, ChessSquareWorld(CHESS_MOVE_TO(g->hintMove)), 0.98f, c);
    }

    // Vua đang bị chiếu: quầng đỏ nhịp đập
    if (g->state != CHESS_STATE_MENU && ChessInCheck(&g->board, g->board.side)) {
        float pulse = 0.6f + 0.4f * sinf(t * 7.0f);
        Vector3 k = ChessSquareWorld(g->board.king[g->board.side]);
        DrawFlatQuad(R.glow, k, 1.9f, (Color){255, 40, 40, (unsigned char)(200 * pulse)});
    }

    if (InteractiveState(g)) {
        if (g->selected >= 0) {
            DrawFlatQuad(R.softSquare, ChessSquareWorld(g->selected), 0.98f, (Color){90, 190, 255, 150});
            for (int sq = 0; sq < 64; sq++) {
                bool capture = false;
                if (!IsLegalTarget(g, g->selected, sq, &capture)) continue;
                Vector3 p = ChessSquareWorld(sq);
                if (capture) DrawFlatQuad(R.ring, p, 0.98f, (Color){255, 96, 80, 210});
                else DrawFlatQuad(R.disc, p, 0.42f, (Color){60, 220, 170, 200});
            }
        }
        if (g->hoverSquare >= 0 && !g->keyboardCursor) {
            DrawFlatQuad(R.softSquare, ChessSquareWorld(g->hoverSquare), 0.98f, (Color){255, 255, 255, 46});
        }
        if (g->keyboardCursor && g->cursor >= 0) {
            float pulse = 0.6f + 0.4f * sinf(t * 6.0f);
            Vector3 p = ChessSquareWorld(g->cursor);
            DrawFlatQuad(R.softSquare, p, 0.98f, (Color){255, 214, 110, (unsigned char)(70 * pulse)});
            DrawFlatQuad(R.ring, p, 1.0f, (Color){255, 214, 110, (unsigned char)(230 * pulse)});
        }
    }

    EndBlendMode();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}

static Vector4 VisualHighlight(const ChessGame *g, const ChessVisual *v)
{
    float t = g->globalTime;
    if (!InteractiveState(g) || v->captured) return (Vector4){0, 0, 0, 0};
    if (v->square == g->selected) {
        float pulse = 0.7f + 0.3f * sinf(t * 5.0f);
        return (Vector4){0.25f, 0.62f, 1.0f, 0.55f * pulse};
    }
    int focus = g->keyboardCursor ? g->cursor : g->hoverSquare;
    if (v->square == focus) {
        bool ownTurn = v->color == g->board.side;
        if (ownTurn) return (Vector4){1.0f, 0.85f, 0.5f, 0.28f};
    }
    return (Vector4){0, 0, 0, 0};
}

static void SetSceneUniforms(Vector3 viewPos, float camDistance, Matrix lightVP)
{
    if (!R.litReady) return;
    Vector3 ld = Vector3Normalize(LIGHT_DIR);
    SetV3(R.locViewPos, viewPos);
    SetV3(R.locLightDir, ld);
    SetV3(R.locLightColor, (Vector3){3.6f, 3.35f, 3.0f});
    SetV3(R.locFillDir, Vector3Normalize((Vector3){-0.5f, -0.35f, -0.6f}));
    SetV3(R.locFillColor, (Vector3){0.55f, 0.65f, 0.9f});
    SetV3(R.locSky, (Vector3){0.30f, 0.32f, 0.38f});
    SetV3(R.locGround, (Vector3){0.10f, 0.08f, 0.06f});
    SetV3(R.locFogColor, (Vector3){BG_BOTTOM.r / 255.0f * 1.3f, BG_BOTTOM.g / 255.0f * 1.3f, BG_BOTTOM.b / 255.0f * 1.3f});
    SetF(R.locFogDensity, 0.065f);
    SetF(R.locFogStart, camDistance + 2.0f);
    SetF(R.locUseShadow, R.shadowReady ? 1.0f : 0.0f);
    SetF(R.locShadowTexel, 1.0f / (float)(R.shadowSize > 0 ? R.shadowSize : 1024));
    SetF(R.locExposure, 1.0f);
    SetShaderValueMatrix(R.lit, R.locLightVP, lightVP);
}

static void SetSurface(float useNormal, float useArm, float uvScale, float roughness, float env)
{
    SetF(R.locUseNormal, useNormal);
    SetF(R.locUseArm, useArm);
    SetF(R.locUvScale, uvScale);
    SetF(R.locRoughness, roughness);
    SetF(R.locEnv, env);
    SetV4(R.locHighlight, (Vector4){0, 0, 0, 0});
}

static void DrawBoardAndTable(bool depthOnly)
{
    Matrix boardM = MatrixScale(WORLD_SCALE, WORLD_SCALE, WORLD_SCALE);
    if (depthOnly) {
        if (R.hasModel) DrawMesh(R.model.meshes[R.boardMesh], R.depthMat, boardM);
        return;
    }
    // Mặt bàn gỗ lớn dưới bàn cờ
    SetSurface(0.0f, 0.0f, 7.0f, 0.55f, 0.3f);
    DrawMesh(R.tableMesh, R.tableMat, MatrixIdentity());

    if (R.hasModel) {
        SetSurface(1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawMesh(R.model.meshes[R.boardMesh], R.boardMat, boardM);
    } else {
        for (int sq = 0; sq < 64; sq++) {
            Vector3 p = ChessSquareWorld(sq);
            bool dark = ((CHESS_FILE(sq) + CHESS_RANK(sq)) & 1) == 0;
            DrawCube((Vector3){p.x, BOARD_TOP * 0.5f, p.z}, 1.0f, BOARD_TOP, 1.0f,
                     dark ? (Color){60, 62, 66, 255} : (Color){226, 224, 218, 255});
        }
    }
}

static void DrawAllPieces(const ChessGame *g, bool depthOnly)
{
    if (!depthOnly) SetSurface(1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    for (int i = 0; i < CHESS_MAX_VISUALS; i++) {
        const ChessVisual *v = &g->visuals[i];
        if (!v->alive) continue;
        float scale = VisualScale(v);
        if (depthOnly) DrawPieceDepth(v->type, v->color, v->pos, scale);
        else DrawPiece(v->type, v->color, v->pos, scale, VisualHighlight(g, v));
    }
}

void ChessRenderScene(const ChessGame *g)
{
    if (R.refCount <= 0 || R.scene.id == 0) return;

    Camera3D cam = ChessCameraFromRig(&g->cam);
    Matrix lightVP = MatrixIdentity();

    // Lượt 1: độ sâu nhìn từ nguồn sáng (chiếu trực giao ôm trọn bàn và quân bị bắt)
    if (R.shadowReady) {
        Vector3 ld = Vector3Normalize(LIGHT_DIR);
        Camera3D lightCam = {0};
        lightCam.target = (Vector3){0.0f, 0.0f, 0.0f};
        lightCam.position = Vector3Scale(ld, -22.0f);
        lightCam.up = (Vector3){0.0f, 1.0f, 0.0f};
        lightCam.fovy = 20.0f;
        lightCam.projection = CAMERA_ORTHOGRAPHIC;

        BeginTextureMode(R.shadow);
            rlClearScreenBuffers();
            BeginMode3D(lightCam);
                rlMatrixMode(RL_PROJECTION);
                rlLoadIdentity();
                rlOrtho(-8.2, 8.2, -8.2, 8.2, 8.0, 40.0);
                rlMatrixMode(RL_MODELVIEW);
                Matrix lightView = rlGetMatrixModelview();
                Matrix lightProj = rlGetMatrixProjection();
                lightVP = MatrixMultiply(lightView, lightProj);
                DrawBoardAndTable(true);
                DrawAllPieces(g, true);
            EndMode3D();
        EndTextureMode();
        R.depthMat.maps[MATERIAL_MAP_BRDF].texture = (Texture2D){0};
        R.boardMat.maps[MATERIAL_MAP_BRDF].texture = R.shadow.depth;
        R.pieceMat[0].maps[MATERIAL_MAP_BRDF].texture = R.shadow.depth;
        R.pieceMat[1].maps[MATERIAL_MAP_BRDF].texture = R.shadow.depth;
        R.tableMat.maps[MATERIAL_MAP_BRDF].texture = R.shadow.depth;
    }

    // Lượt 2: cảnh chính vào render texture siêu lấy mẫu
    BeginTextureMode(R.scene);
        ClearBackground(BG_BOTTOM);
        DrawRectangleGradientV(0, 0, R.scene.texture.width, R.scene.texture.height, BG_TOP, BG_BOTTOM);

        BeginMode3D(cam);
            rlMatrixMode(RL_PROJECTION);
            rlLoadIdentity();
            {
                Matrix p = SceneProjection();
                rlMultMatrixf(MatrixToFloatV(p).v);
            }
            rlMatrixMode(RL_MODELVIEW);

            SetSceneUniforms(cam.position, g->cam.distance, lightVP);
            DrawBoardAndTable(false);
            DrawOverlays(g);
            DrawAllPieces(g, false);
        EndMode3D();
    EndTextureMode();
}

void ChessRenderBlit(void)
{
    if (R.scene.id == 0) {
        ClearBackground(BG_BOTTOM);
        return;
    }
    Rectangle src = {0.0f, 0.0f, (float)R.scene.texture.width, -(float)R.scene.texture.height};
    Rectangle dst = {0.0f, 0.0f, (float)CHESS_CANVAS_W, (float)CHESS_CANVAS_H};
    DrawTexturePro(R.scene.texture, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
}
