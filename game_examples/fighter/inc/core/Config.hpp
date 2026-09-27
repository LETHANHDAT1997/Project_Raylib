#pragma once
#include "raylib.h"

// ============================================================================
// Hằng số toàn cục của game. Mọi toạ độ trong code đều nằm trong "canvas ảo"
// 1280x720; lớp main/runner chịu trách nhiệm letterbox ra cửa sổ thật.
// ============================================================================
namespace fighter {

inline constexpr int   kCanvasWidth  = 1280;
inline constexpr int   kCanvasHeight = 720;

// Mặt đất: nhân vật đứng bằng "chân", nên position.y chính là đường này.
inline constexpr float kGroundY      = 592.0f;

inline constexpr float kGravity      = 2600.0f;
inline constexpr float kStageLeft    = 70.0f;
inline constexpr float kStageRight   = kCanvasWidth - 70.0f;

// Khoảng cách tối thiểu giữa hai nhân vật (đẩy nhau ra, không cho chồng lên).
inline constexpr float kBodyRadius   = 34.0f;

inline constexpr int   kMaxHealth    = 1000;
inline constexpr float kMaxMeter     = 100.0f;
inline constexpr float kRoundSeconds = 99.0f;

// Cửa sổ thời gian tính combo: đánh tiếp trong khoảng này thì combo tăng.
inline constexpr float kComboWindow  = 1.2f;

} // namespace fighter
