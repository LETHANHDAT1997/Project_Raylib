#include "ui_anim.h"
#include <math.h>

// Nội suy mũ: kết quả không phụ thuộc vào tốc độ khung hình, khác hẳn
// cách lerp cố định hệ số vốn chạy nhanh hơn khi FPS cao.
float UiApproach(float current, float target, float speed, float dt)
{
    if (speed <= 0.0f) return target;
    float t = 1.0f - expf(-speed * dt);
    return current + (target - current) * t;
}

Vector2 UiApproachVec(Vector2 current, Vector2 target, float speed, float dt)
{
    return (Vector2){
        UiApproach(current.x, target.x, speed, dt),
        UiApproach(current.y, target.y, speed, dt)
    };
}

Rectangle UiApproachRect(Rectangle current, Rectangle target, float speed, float dt)
{
    return (Rectangle){
        UiApproach(current.x, target.x, speed, dt),
        UiApproach(current.y, target.y, speed, dt),
        UiApproach(current.width, target.width, speed, dt),
        UiApproach(current.height, target.height, speed, dt)
    };
}

Color UiApproachColor(Color current, Color target, float speed, float dt)
{
    return (Color){
        (unsigned char)(UiApproach((float)current.r, (float)target.r, speed, dt) + 0.5f),
        (unsigned char)(UiApproach((float)current.g, (float)target.g, speed, dt) + 0.5f),
        (unsigned char)(UiApproach((float)current.b, (float)target.b, speed, dt) + 0.5f),
        (unsigned char)(UiApproach((float)current.a, (float)target.a, speed, dt) + 0.5f)
    };
}

float UiEaseOutCubic(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

float UiEaseInOutCubic(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.5f) return 4.0f * t * t * t;
    float f = -2.0f * t + 2.0f;
    return 1.0f - (f * f * f) / 2.0f;
}

float UiEaseOutBack(float t)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float f = t - 1.0f;
    return 1.0f + c3 * f * f * f + c1 * f * f;
}

float UiPulse(float time, float speed, float lo, float hi)
{
    float s = 0.5f + 0.5f * sinf(time * speed);
    return lo + (hi - lo) * s;
}
