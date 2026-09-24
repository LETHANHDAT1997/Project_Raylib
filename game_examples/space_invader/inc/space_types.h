#ifndef SPACE_TYPES_H
#define SPACE_TYPES_H

#include "raylib.h"
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 880

#define ALIEN_ROWS 5
#define ALIEN_COLS 11
#define TOTAL_ALIENS (ALIEN_ROWS * ALIEN_COLS)

#define MAX_PLAYER_BULLETS 5
#define MAX_ALIEN_BULLETS 8

#define MAX_BUNKERS 4
#define BUNKER_ROWS 5
#define BUNKER_COLS 6
#define BUNKER_BLOCK_SIZE 12

#define MAX_PARTICLES 300
#define MAX_STARS 75

typedef enum {
    ALIEN_SQUID = 0,   // 30 pts (Row 0)
    ALIEN_CRAB = 1,    // 20 pts (Row 1, 2)
    ALIEN_OCTOPUS = 2  // 10 pts (Row 3, 4)
} AlienType;

typedef enum {
    SPACE_STATE_MENU = 0,
    SPACE_STATE_PLAYING,
    SPACE_STATE_PLAYER_DEAD,
    SPACE_STATE_GAME_OVER,
    SPACE_STATE_WAVE_CLEAR
} SpaceGameState;

typedef struct {
    float x;
    float y;
    float vy;
    bool active;
    bool isPlayer;
} SpaceBullet;

typedef struct {
    float x;
    float y;
    float speed;
    int width;
    int height;
    int lives;
    float hitTimer;
    bool isAlive;
    float shootCooldown;
} SpacePlayer;

typedef struct {
    float x;
    float y;
    AlienType type;
    bool alive;
    int animFrame; // 0 or 1
} SpaceAlien;

typedef struct {
    float x;
    float y;
    int grid[BUNKER_ROWS][BUNKER_COLS]; // 0: destroyed, 1..3: health
} SpaceBunker;

typedef struct {
    float x;
    float y;
    float speed;
    bool active;
    int scoreValue;
    float spawnTimer;
} SpaceUfo;

typedef struct {
    float x;
    float y;
    float speed;
    float brightness;
} SpaceStar;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    Color color;
    float life;
    float maxLife;
    float size;
} SpaceParticle;

typedef struct {
    SpacePlayer player;
    SpaceBullet playerBullets[MAX_PLAYER_BULLETS];
    SpaceBullet alienBullets[MAX_ALIEN_BULLETS];
    SpaceAlien aliens[ALIEN_ROWS][ALIEN_COLS];
    SpaceBunker bunkers[MAX_BUNKERS];
    SpaceUfo ufo;
    SpaceStar stars[MAX_STARS];

    int alienDir; // 1 = right, -1 = left
    float alienStepTimer;
    float alienStepInterval;
    float alienShootTimer;
    int alienBeatIndex; // 0 to 3 for 4-note march
    int aliensRemaining;

    int score;
    int highScore;
    int wave;

    SpaceGameState state;
    float stateTimer;
    bool soundEnabled;

    SpaceParticle particles[MAX_PARTICLES];
    int particleCount;
} SpaceGame;

#endif // SPACE_TYPES_H
