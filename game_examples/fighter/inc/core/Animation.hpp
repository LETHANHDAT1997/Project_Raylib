#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace fighter {

// ============================================================================
// Danh sách animation mà MỌI nhân vật đều phải có. Roster nạp đúng 8 file này.
// ============================================================================
enum class AnimId {
    Idle = 0,
    Run,
    Jump,
    Fall,
    Attack1,
    Attack2,
    TakeHit,
    Death,
    Count
};

const char *AnimFileName(AnimId id);   // "idle", "run", ...

// ============================================================================
// Animation - một sprite sheet dải ngang, mỗi frame là hình vuông cạnh = chiều
// cao ảnh. Toàn bộ pack sprite CC0 đang dùng đều theo quy ước này.
// ============================================================================
struct Animation {
    Texture2D texture{};
    int   frameCount = 1;
    float fps        = 10.0f;
    bool  loop       = true;

    float Duration() const { return frameCount / fps; }
    Rectangle FrameRect(int index) const;
};

// ============================================================================
// Animator - con trỏ thời gian chạy trên một Animation.
// ============================================================================
class Animator {
public:
    // Đổi animation. resetIfSame=false giữ nguyên tiến độ khi gọi lại cùng anim
    // (tránh idle bị giật vì mỗi frame lại reset).
    void Play(const Animation *anim, bool resetIfSame = false);
    void Update(float dt);

    const Animation *Current() const { return anim_; }
    int   Frame()     const { return frame_; }
    float Progress()  const;               // 0..1 trong một vòng
    bool  Finished()  const { return finished_; }

    // Tốc độ phát riêng (nhân vật nhanh/chậm khác nhau, hoặc hitstop).
    void  SetSpeed(float s) { speed_ = s; }

private:
    const Animation *anim_ = nullptr;
    float timer_    = 0.0f;
    int   frame_    = 0;
    bool  finished_ = false;
    float speed_    = 1.0f;
};

} // namespace fighter
