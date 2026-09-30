/**
 * hub_art.h - Artwork vẽ tay cho từng game (icon sidebar + tranh hero).
 *
 * Tách khỏi game_registry.c để phần dữ liệu luôn ngắn gọn, dễ đọc.
 */
#ifndef HUB_ART_H
#define HUB_ART_H

#include "raylib.h"

void HubArtTetrisIcon(Rectangle area, float time);
void HubArtTetrisHero(Rectangle area, float time);

void HubArtSpaceIcon(Rectangle area, float time);
void HubArtSpaceHero(Rectangle area, float time);

void HubArtSnakeIcon(Rectangle area, float time);
void HubArtSnakeHero(Rectangle area, float time);

void HubArtFighterIcon(Rectangle area, float time);
void HubArtFighterHero(Rectangle area, float time);

void HubArtFlappyIcon(Rectangle area, float time);
void HubArtFlappyHero(Rectangle area, float time);

// Giải phóng texture mà artwork dùng sprite thật (Flappy Plane) đã nạp.
void HubArtRelease(void);

#endif // HUB_ART_H
