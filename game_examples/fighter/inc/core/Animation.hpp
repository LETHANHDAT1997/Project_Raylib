#pragma once
#include "raylib.h"

namespace fighter {

// ============================================================================
// Danh sách animation mà MỌI nhân vật đều phải có. Roster nạp đúng 8 file này.
// Các tư thế còn thiếu so với game đối kháng thật (ngồi, lùi, đỡ, đứng dậy)
// được dựng lại từ 8 dải này: ngồi = idle nén dọc, lùi = run phát ngược,
// đứng dậy = death phát ngược...
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

// Cách phát: đoạn frame con, lặp hay không, xuôi hay ngược.
struct PlayMode {
    int  from    = 0;
    int  to      = -1;      // -1 = tới frame cuối
    bool loop    = true;
    bool reverse = false;
};

// ============================================================================
// Animator - con trỏ thời gian chạy trên một Animation.
//
// Hỗ trợ phát một ĐOẠN frame (đòn nhẹ chỉ dùng nửa đầu cú chém) và phát
// NGƯỢC (lùi bước = chạy ngược, đứng dậy = ngã ngược) - hai mẹo chính để có
// nhiều động tác từ một bộ sprite ít animation.
// ============================================================================
class Animator {
public:
    // Đổi animation. Gọi lại cùng anim + cùng chế độ thì giữ nguyên tiến độ
    // (tránh idle bị giật vì mỗi frame lại reset), trừ khi restart = true.
    void Play(const Animation *anim, bool restart = false);
    void Play(const Animation *anim, const PlayMode &mode, bool restart);
    void Update(float dt);

    const Animation *Current() const { return anim_; }
    int   Frame()     const { return frame_; }
    bool  Finished()  const { return finished_; }
    int   ClipLength() const;

    // Tốc độ phát riêng (nhân vật nhanh/chậm khác nhau, hoặc khớp frame data).
    void  SetSpeed(float s) { speed_ = s; }
    float Speed() const     { return speed_; }

private:
    int First() const;
    int Last() const;

    const Animation *anim_ = nullptr;
    PlayMode mode_{};
    float timer_    = 0.0f;
    int   frame_    = 0;
    bool  finished_ = false;
    float speed_    = 1.0f;
};

} // namespace fighter
