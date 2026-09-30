#ifndef FLAPPY_TYPES_H
#define FLAPPY_TYPES_H

#include "raylib.h"
#include <stdbool.h>

// Thế giới game được thiết kế theo kích thước gốc của bộ sprite Tappy Plane
// (nền 800x480). Canvas ảo lớn hơn 1.6 lần để chữ HUD luôn sắc nét khi
// phóng to lên Full HD / 4K - còn thế giới được vẽ qua Camera2D có zoom.
#define WORLD_W 800
#define WORLD_H 480
#define CANVAS_W 1280
#define CANVAS_H 768
#define WORLD_ZOOM 1.6f

#define GROUND_TEX_W 808
#define GROUND_TEX_H 71
#define GROUND_Y (WORLD_H - GROUND_TEX_H)   // Mép trên của dải đất (409)

#define ROCK_W 108
#define ROCK_H 239

#define PLANE_TEX_W 88
#define PLANE_TEX_H 73
#define PLANE_SCALE 0.6f
#define PLANE_X 190.0f
#define PLANE_RADIUS 15.0f

#define MAX_ROCK_PAIRS 8
#define MAX_STARS 8
#define MAX_PARTICLES 200
#define MAX_FLOATING_TEXTS 12
#define GROUND_TILES 2

typedef enum {
    FLAPPY_STATE_MENU = 0,
    FLAPPY_STATE_READY,       // Máy bay lơ lửng chờ cú nhấn đầu tiên
    FLAPPY_STATE_PLAYING,
    FLAPPY_STATE_PAUSED,
    FLAPPY_STATE_DYING,       // Đã va đá, đang rơi xoay vòng xuống đất
    FLAPPY_STATE_GAME_OVER
} FlappyState;

typedef enum {
    FLAPPY_DIFF_EASY = 0,
    FLAPPY_DIFF_NORMAL,
    FLAPPY_DIFF_HARD,
    FLAPPY_DIFF_COUNT
} FlappyDifficulty;

typedef enum {
    STAR_BRONZE = 0,
    STAR_SILVER,
    STAR_GOLD,
    STAR_KIND_COUNT
} StarKind;

typedef enum {
    MEDAL_NONE = -1,
    MEDAL_BRONZE = 0,
    MEDAL_SILVER,
    MEDAL_GOLD
} MedalKind;

// Thông số từng mức độ khó: chỉnh ở flappy_world.c, không rải hằng số khắp nơi.
typedef struct {
    const char *name;
    float scrollSpeed;     // Tốc độ cuộn ban đầu (px/s)
    float speedPerPoint;   // Tăng tốc theo mỗi điểm
    float maxSpeedBonus;
    float gap;             // Khe hở dọc giữa hai mũi đá lúc đầu
    float minGap;          // Khe hở nhỏ nhất khi điểm cao
    float gapShrinkPerPoint;
    float spacing;         // Khoảng cách ngang giữa các cặp đá
    float maxGapDelta;     // Độ lệch tâm khe tối đa giữa hai cặp liên tiếp
    Color accent;
} DifficultyDef;

typedef struct {
    float x;           // Mép trái của cặp đá (toạ độ thế giới)
    float gapCenter;
    float gap;
    float topH;        // Chiều cao vẽ (có thể kéo dài hơn sprite gốc)
    float bottomH;
    int biome;
    bool hasTop;
    bool hasBottom;
    bool passed;
    bool active;
} RockPair;

typedef struct {
    Vector2 pos;
    StarKind kind;
    float phase;
    bool active;
} Star;

typedef enum {
    PARTICLE_PUFF = 0,   // Khói trắng (sprite puff)
    PARTICLE_SPARK,      // Tia lấp lánh khi nhặt sao
    PARTICLE_DEBRIS      // Mảnh vụn khi va chạm
} ParticleKind;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float size;
    float rotation;
    float spin;
    float life;
    float maxLife;
    ParticleKind kind;
    bool large;
    bool active;
} FlappyParticle;

typedef struct {
    char text[24];
    Vector2 pos;          // Toạ độ thế giới
    Color color;
    float life;
    float maxLife;
    bool active;
} FlappyFloatText;

typedef struct {
    Vector2 pos;
    float vy;
    float angle;          // Độ, dương = chúc mũi xuống
    float propTimer;
    int propFrame;
    float trailTimer;
    float spinSpeed;      // Dùng khi rơi sau va chạm
    bool alive;
} Plane;

typedef struct {
    float x;
    int biome;
} GroundTile;

typedef struct {
    FlappyState state;
    FlappyState pausedFrom;
    FlappyDifficulty difficulty;
    int planeSkin;

    Plane plane;
    RockPair rocks[MAX_ROCK_PAIRS];
    Star stars[MAX_STARS];
    FlappyParticle particles[MAX_PARTICLES];
    FlappyFloatText floatTexts[MAX_FLOATING_TEXTS];
    GroundTile ground[GROUND_TILES];

    float scrollSpeed;
    float bgOffset;
    float distanceToNextRock;
    float lastGapCenter;

    int biome;             // Vùng hiện tại (đổi mỗi BIOME_EVERY điểm)
    float biomeBanner;     // Thời gian còn lại của dải thông báo đổi vùng
    Vector3 skyTint;       // RGB dạng float để chuyển màu mượt, không bị làm tròn kẹt

    int score;
    int starsCollected;
    int rocksPassed;
    int best[FLAPPY_DIFF_COUNT];
    bool newBest;

    float shakeTimer;
    float shakeMagnitude;
    float flashTimer;
    float stateTime;
    float globalTime;
    float gameOverAnim;

    float scorePop;        // Nhịp "nảy" của con số điểm mỗi khi tăng
    int shownScore;        // Điểm đang hiển thị ở bảng tổng kết (đếm dần lên)
    float countTimer;
    bool soundEnabled;
    bool quitRequested;    // Bản chạy độc lập dùng để đóng cửa sổ từ menu
} FlappyGame;

#endif // FLAPPY_TYPES_H
