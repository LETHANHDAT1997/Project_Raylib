#pragma once
#include "raylib.h"

namespace fighter {

class Fighter;

// ============================================================================
// InputState - "ý định" của một người chơi trong 1 frame, theo hướng TUYỆT ĐỐI
// (trái/phải của màn hình). Fighter tự quy đổi ra hướng tương đối (tiến/lùi)
// dựa trên việc đang quay mặt về phía nào.
//
// Fighter không bao giờ gọi IsKeyDown: người thật và AI đi chung đường xử lý.
// ============================================================================
struct InputState {
    bool left = false, right = false, up = false, down = false;
    bool block = false;                // phím đỡ phụ (ngoài cách giữ lùi)

    // Cạnh lên: vừa bấm trong frame này
    bool lightPressed   = false;
    bool mediumPressed  = false;
    bool heavyPressed   = false;
    bool specialPressed = false;       // nút chiêu nhanh (kiểu "Modern")
    bool superPressed   = false;

    bool AnyAttackPressed() const { return lightPressed || mediumPressed || heavyPressed; }
    void Clear() { *this = InputState{}; }
};

// Bộ phím của một người chơi. Mỗi thao tác nhận tối đa 2 phím (0 = bỏ trống),
// để khi chơi một mình có thể dùng cả WASD lẫn phím mũi tên.
struct KeyBindings {
    static constexpr int kSlots = 2;
    int left[kSlots]{}, right[kSlots]{}, up[kSlots]{}, down[kSlots]{};
    int light[kSlots]{}, medium[kSlots]{}, heavy[kSlots]{};
    int special[kSlots]{}, super[kSlots]{}, block[kSlots]{};

    // Chơi 1 người / luyện tập: WASD + J K L I U, HOẶC mũi tên + Z X C V B.
    static KeyBindings PlayerOneSolo();
    // Chơi 2 người: người 1 chỉ WASD + J K L I U, nhường mũi tên cho người 2.
    static KeyBindings PlayerOne();
    // Người 2: mũi tên · Num1 2 3 · Num4 · Num5 · Num0
    static KeyBindings PlayerTwo();
};

// ============================================================================
// InputBuffer - lịch sử hướng bấm (ký hiệu numpad, TƯƠNG ĐỐI với mặt nhân vật)
// để nhận diện lệnh quay tay của game đối kháng:
//
//     7 8 9        236  = xuống, xuống-tiến, tiến        (Hadouken)
//     4 5 6        623  = tiến, xuống, xuống-tiến        (Shoryuken)
//     1 2 3        214  = xuống, xuống-lùi, lùi          (Tatsumaki)
//                  236236 = hai lần 236                  (Super)
// ============================================================================
class InputBuffer {
public:
    void Clear();
    void Record(int dir, float now);          // chỉ lưu khi hướng thay đổi

    // Chuỗi hướng khớp theo thứ tự trong `window` giây, hướng cuối phải còn
    // mới (bấm trong ~0.2 giây gần nhất hoặc vẫn đang giữ).
    bool Motion(const char *seq, float now, float window) const;

    int Current() const { return count_ ? hist_[(head_ + count_ - 1) % kSize].dir : 5; }

private:
    struct Entry { int dir; float time; };
    static constexpr int kSize = 32;

    const Entry &At(int fromNewest) const;    // 0 = mới nhất
    static bool Matches(char token, int dir);

    Entry hist_[kSize]{};
    int head_  = 0;
    int count_ = 0;
};

// ============================================================================
// Controller - lớp trừu tượng sinh ra InputState.
// ============================================================================
class Controller {
public:
    virtual ~Controller() = default;
    virtual void Update(const Fighter &self, const Fighter &opponent, float dt) = 0;
    virtual bool IsHuman() const { return false; }

    const InputState &State() const { return state_; }

protected:
    InputState state_{};
};

// Người chơi thật - đọc bàn phím.
class HumanController : public Controller {
public:
    explicit HumanController(const KeyBindings &keys) : keys_(keys) {}
    void Update(const Fighter &self, const Fighter &opponent, float dt) override;
    bool IsHuman() const override { return true; }

private:
    KeyBindings keys_;
};

// Hình nộm cho chế độ luyện tập: đứng yên / tự đỡ / nhảy liên tục.
class DummyController : public Controller {
public:
    enum class Mode { Stand, Crouch, Block, Jump, Count };
    void Update(const Fighter &self, const Fighter &opponent, float dt) override;

    void  Cycle() { mode_ = (Mode)(((int)mode_ + 1) % (int)Mode::Count); }
    Mode  CurrentMode() const { return mode_; }
    static const char *ModeName(Mode m);

private:
    Mode  mode_ = Mode::Stand;
    float timer_ = 0.0f;
};

} // namespace fighter
