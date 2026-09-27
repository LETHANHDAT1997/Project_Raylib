/**
 * ui_anim.h - Bộ công cụ nội suy và easing cho hoạt ảnh giao diện.
 */
#ifndef UI_ANIM_H
#define UI_ANIM_H

#include "raylib.h"

// Nội suy độc lập khung hình: kéo current về target với tốc độ `speed`.
float     UiApproach(float current, float target, float speed, float dt);
Vector2   UiApproachVec(Vector2 current, Vector2 target, float speed, float dt);
Rectangle UiApproachRect(Rectangle current, Rectangle target, float speed, float dt);
Color     UiApproachColor(Color current, Color target, float speed, float dt);

// Easing cơ bản.
float UiEaseOutCubic(float t);
float UiEaseInOutCubic(float t);
float UiEaseOutBack(float t);

// Dao động hình sin trong khoảng [lo, hi].
float UiPulse(float time, float speed, float lo, float hi);

#endif // UI_ANIM_H
