#ifndef TETRIS_TYPES_H
#define TETRIS_TYPES_H

#include "raylib.h"
#include <stdbool.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define CELL_SIZE 32

#define BOARD_OFFSET_X 240
#define BOARD_OFFSET_Y 50

#define SCREEN_WIDTH 780
#define SCREEN_HEIGHT 740

#define MAX_PARTICLES 256
#define PREVIEW_COUNT 3

typedef enum {
    PIECE_NONE = 0,
    PIECE_I,
    PIECE_O,
    PIECE_T,
    PIECE_S,
    PIECE_Z,
    PIECE_J,
    PIECE_L,
    PIECE_COUNT
} PieceType;

typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

typedef struct {
    PieceType type;
    int rotation; // 0 to 3
    int x;        // Grid coordinates
    int y;
} TetrisPiece;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    Color color;
    float life;
    float maxLife;
    float size;
} Particle;

typedef struct {
    int grid[BOARD_HEIGHT][BOARD_WIDTH];
    Color colors[BOARD_HEIGHT][BOARD_WIDTH];
    int linesClearing[4];
    int linesClearingCount;
    float clearAnimTimer;
} TetrisBoard;

typedef struct {
    TetrisBoard board;
    TetrisPiece current;
    int ghostY;
    PieceType holdPiece;
    bool canHold;

    PieceType nextPieces[PREVIEW_COUNT];
    int bag[7];
    int bagIndex;

    int score;
    int highScore;
    int lines;
    int level;
    int combo;
    int lastClearCount;

    float dropTimer;
    float dropInterval;

    // Delayed Auto Shift (DAS)
    float keyLeftTimer;
    float keyRightTimer;
    float keyDownTimer;

    GameState state;
    bool soundEnabled;

    Particle particles[MAX_PARTICLES];
    int particleCount;
} TetrisGame;

#endif // TETRIS_TYPES_H
