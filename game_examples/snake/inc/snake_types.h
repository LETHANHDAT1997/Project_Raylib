#ifndef SNAKE_TYPES_H
#define SNAKE_TYPES_H

#include "raylib.h"
#include <stdbool.h>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 720

#define GRID_COLS 20
#define GRID_ROWS 20
#define CELL_SIZE 32

#define BOARD_OFFSET_X 40
#define BOARD_OFFSET_Y 40
#define BOARD_WIDTH (GRID_COLS * CELL_SIZE)   // 640
#define BOARD_HEIGHT (GRID_ROWS * CELL_SIZE)  // 640

#define MAX_SNAKE_LENGTH (GRID_COLS * GRID_ROWS)
#define MAX_PARTICLES 300
#define MAX_FLOATING_TEXTS 16

typedef enum {
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,
    DIR_NONE
} SnakeDirection;

typedef enum {
    FOOD_NORMAL = 0,   // Táo đỏ: +100 điểm, dài thêm 1 đốt
    FOOD_GOLDEN,       // Táo vàng: +300 điểm, tồn tại có thời hạn, combo x2
    FOOD_FROST,        // Quả băng tuyết: +150 điểm, làm chậm rắn trong 5s
    FOOD_COUNT
} FoodType;

typedef enum {
    SNAKE_STATE_MENU = 0,
    SNAKE_STATE_PLAYING,
    SNAKE_STATE_PAUSED,
    SNAKE_STATE_GAME_OVER
} SnakeGameState;

typedef enum {
    MODE_CLASSIC = 0,   // Đâm tường sẽ chết
    MODE_NO_WALLS       // Xuyên tường (xuất hiện ở phía đối diện)
} SnakeGameMode;

typedef enum {
    DIFF_NORMAL = 0,    // Tốc độ tiêu chuẩn
    DIFF_FAST,          // Tốc độ nhanh
    DIFF_INSANE         // Tốc độ điên cuồng
} SnakeDifficulty;

typedef struct {
    int x;
    int y;
} GridPos;

typedef struct {
    GridPos pos;
    Vector2 visualPos;
} SnakeSegment;

typedef struct {
    SnakeSegment segments[MAX_SNAKE_LENGTH];
    int length;
    SnakeDirection dir;
    SnakeDirection nextDir;
    SnakeDirection queuedDir; // Bộ đệm 2 bước phím để không bị khựng khi bấm nhanh
    float moveTimer;
    float moveInterval;
    float stepProgress;       // 0.0f đến 1.0f để nội suy mượt mà
    bool alive;
    float eyeBlinkTimer;
    float tongueTimer;
    float tongueLength;
    int growPending;
} Snake;

typedef struct {
    GridPos pos;
    FoodType type;
    bool active;
    float timer;
    float maxTime;
    float pulse;
} FoodItem;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float size;
    float life;
    float maxLife;
    bool active;
} SnakeParticle;

typedef struct {
    char text[32];
    Vector2 pos;
    Color color;
    float life;
    float maxLife;
    bool active;
} FloatingText;

typedef struct {
    Snake snake;
    FoodItem normalFood;
    FoodItem bonusFood;

    int score;
    int highScore;
    int applesEaten;
    int goldenEaten;
    int combo;
    float comboTimer;

    SnakeGameState state;
    SnakeGameMode mode;
    SnakeDifficulty difficulty;

    float frostTimer;
    float shakeTimer;
    float shakeMagnitude;
    bool soundEnabled;

    SnakeParticle particles[MAX_PARTICLES];
    FloatingText floatingTexts[MAX_FLOATING_TEXTS];

    float globalTime;
    float stateTime;
} SnakeGame;

#endif // SNAKE_TYPES_H
