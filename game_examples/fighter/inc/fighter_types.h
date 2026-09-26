#ifndef FIGHTER_TYPES_H
#define FIGHTER_TYPES_H

#include "raylib.h"

// ============================================================================
// CẤU HÌNH MÀN HÌNH & GAME
// ============================================================================
#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT  600
#define GROUND_Y       480    // Vị trí mặt đất (tính từ top)
#define GRAVITY       1800.0f // Trọng lực
#define MAX_PARTICLES   256
#define MAX_PROJECTILES   8
#define ROUND_TIME       99   // Thời gian mỗi round (giây)
#define MAX_ROUNDS        3   // Số round tối đa
#define WINS_NEEDED       2   // Số round cần thắng

// ============================================================================
// TRẠNG THÁI FIGHTER
// ============================================================================
typedef enum {
    FIGHTER_IDLE = 0,
    FIGHTER_WALK_FORWARD,
    FIGHTER_WALK_BACKWARD,
    FIGHTER_JUMP,
    FIGHTER_CROUCH,
    FIGHTER_PUNCH,
    FIGHTER_KICK,
    FIGHTER_SPECIAL,
    FIGHTER_BLOCK,
    FIGHTER_HIT,
    FIGHTER_KNOCKDOWN,
    FIGHTER_VICTORY,
    FIGHTER_DEFEAT,
} FighterState;

// ============================================================================
// LOẠI NHÂN VẬT
// ============================================================================
typedef enum {
    CHAR_RYU = 0,
    CHAR_KEN,
    CHAR_COUNT
} CharacterType;

// ============================================================================
// CẤU TRÚC HITBOX
// ============================================================================
typedef struct {
    Rectangle rect;     // Vùng va chạm tương đối so với vị trí fighter
    bool active;
    int damage;
    float knockback;    // Lực đẩy lùi
} Hitbox;

// ============================================================================
// CẤU TRÚC PROJECTILE (Hadouken)
// ============================================================================
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    int damage;
    int owner;          // 0 = Player 1, 1 = Player 2
    bool active;
    float lifetime;
    Color color;
    float animTimer;
} Projectile;

// ============================================================================
// CẤU TRÚC PARTICLE (Hiệu ứng)
// ============================================================================
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float maxLifetime;
    Color color;
    float size;
    bool active;
    int type;           // 0 = spark, 1 = dust, 2 = energy
} Particle;

// ============================================================================
// CẤU TRÚC FIGHTER
// ============================================================================
typedef struct {
    // Vị trí & vật lý
    Vector2 position;
    Vector2 velocity;
    bool facingRight;
    bool onGround;

    // Thuộc tính
    CharacterType character;
    int health;
    int maxHealth;
    float superMeter;   // Thanh năng lượng đặc biệt (0.0 - 100.0)

    // Trạng thái
    FighterState state;
    float stateTimer;
    float attackTimer;
    float hitStun;
    float blockStun;
    int comboCount;
    float comboTimer;

    // Animation
    float animFrame;
    float animSpeed;
    int currentFrame;

    // Hitbox
    Hitbox attackHitbox;
    Rectangle hurtbox;   // Vùng nhận sát thương

    // Kích thước vẽ
    float width;
    float height;

    // Chiến thắng
    int roundsWon;

    // Input cooldown
    float specialCooldown;
    bool hasHitThisAttack;  // Tránh hit nhiều lần trong 1 đòn

    // Visual effects
    float flashTimer;
    float shakeOffset;

} Fighter;

// ============================================================================
// TRẠNG THÁI GAME
// ============================================================================
typedef enum {
    GAME_STATE_TITLE,
    GAME_STATE_INTRO,       // "Round X - FIGHT!"
    GAME_STATE_PLAYING,
    GAME_STATE_ROUND_END,
    GAME_STATE_MATCH_END,
} GameState;

// ============================================================================
// CẤU TRÚC GAME CHÍNH
// ============================================================================
typedef struct {
    Fighter player1;
    Fighter player2;

    GameState state;
    float stateTimer;

    int currentRound;
    float roundTimer;       // Đếm ngược thời gian

    // Camera shake
    float screenShake;
    Vector2 shakeOffset;

    // Particles & Projectiles
    Particle particles[MAX_PARTICLES];
    Projectile projectiles[MAX_PROJECTILES];

    // Hiệu ứng
    float hitPauseTimer;    // "Hit stop" - dừng sesaat khi đánh trúng
    float slowMotion;       // Slow motion khi kết thúc round

    // Background
    float bgScrollX;
    float cloudOffset;

    // AI cho Player 2
    bool p2IsAI;
    float aiThinkTimer;
    float aiReactionTime;

} FighterGame;

#endif // FIGHTER_TYPES_H
