#ifndef BOOT_TYPES_H
#define BOOT_TYPES_H

#include "raylib.h"
#include <stdbool.h>

#define HUB_VIRTUAL_WIDTH  1280
#define HUB_VIRTUAL_HEIGHT 720

typedef enum {
    BOOT_SCREEN_HUB = 0,
    BOOT_SCREEN_TETRIS,
    BOOT_SCREEN_SPACE_INVADER,
    BOOT_SCREEN_SNAKE,
    BOOT_SCREEN_FIGHTER
} BootScreen;

typedef struct {
    RenderTexture2D target;
    int virtualWidth;
    int virtualHeight;
    Rectangle sourceRec;
    Rectangle destRec;
    float scale;
} BootCanvas;

typedef struct {
    BootScreen currentScreen;
    BootScreen targetScreen;
    float fadeAlpha;
    bool isTransitioning;
    int selectedGameIndex; // 0: Tetris, 1: Space Invader, 2: Snake
    BootCanvas canvas;
    float globalTime;
} BootApp;

#endif // BOOT_TYPES_H
