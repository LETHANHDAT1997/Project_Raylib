#pragma once
#include "raylib.h"

// ============================================================================
// Hằng số toàn cục. Toạ độ màn hình nằm trong "canvas ảo" 1280x720; sàn đấu
// rộng hơn màn hình và camera chạy theo hai võ sĩ như Street Fighter.
// ============================================================================
namespace fighter {

inline constexpr int   kCanvasWidth  = 1280;
inline constexpr int   kCanvasHeight = 720;

// Nhân vật đứng bằng "chân", nên position.y chính là đường này.
inline constexpr float kGroundY      = 600.0f;
inline constexpr float kGravity      = 2700.0f;

// Sàn đấu (toạ độ thế giới) và tường hai đầu.
inline constexpr float kStageWidth   = 2240.0f;
inline constexpr float kStageLeft    = 60.0f;
inline constexpr float kStageRight   = kStageWidth - 60.0f;
// Camera phóng to thế giới quanh mặt đất để nhân vật chiếm khoảng nửa chiều
// cao màn hình như Street Fighter; nền parallax vẽ riêng nên không bị phóng.
inline constexpr float kCameraZoom   = 1.32f;
inline constexpr float kViewWidth    = kCanvasWidth / kCameraZoom;   // bề ngang thế giới thấy được
// Hai người không được cách nhau quá một khung nhìn (camera luôn thấy cả hai).
inline constexpr float kMaxSeparation = kViewWidth - 150.0f;

// Khoảng cách tối thiểu giữa hai thân người.
inline constexpr float kBodyRadius   = 34.0f;

inline constexpr int   kMaxHealth    = 1000;
inline constexpr float kRoundSeconds = 99.0f;

} // namespace fighter
