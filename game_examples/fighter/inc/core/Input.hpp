#pragma once
#include "raylib.h"
#include "core/Difficulty.hpp"
#include <deque>

namespace fighter {

class Fighter;

// ============================================================================
// InputState - "ý định" của một người chơi trong 1 frame. Fighter chỉ đọc
// struct này, không bao giờ gọi IsKeyDown trực tiếp, nhờ vậy người thật và AI
// dùng chung y hệt một đường xử lý.
// ============================================================================
struct InputState {
    bool left = false, right = false, up = false, down = false;
    bool block = false;

    // Cạnh lên (vừa bấm trong frame này)
    bool lightPressed   = false;   // đấm nhanh
    bool heavyPressed   = false;   // đòn mạnh
    bool specialPressed = false;   // chiêu riêng của nhân vật
    bool superPressed   = false;   // chiêu cuối (tốn thanh Super)
    bool jumpPressed    = false;
    bool dashPressed    = false;

    void Clear() { *this = InputState{}; }
};

// Bộ phím của một người chơi.
struct KeyBindings {
    int left, right, up, down;
    int light, heavy, special, super, block;

    static KeyBindings PlayerOne();   // A D W S · J K L U · Space
    static KeyBindings PlayerTwo();   // ← → ↑ ↓ · Num1 2 3 4 · Num0
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

// ============================================================================
// HumanController - đọc bàn phím thật.
// ============================================================================
class HumanController : public Controller {
public:
    explicit HumanController(const KeyBindings &keys) : keys_(keys) {}
    void Update(const Fighter &self, const Fighter &opponent, float dt) override;
    bool IsHuman() const override { return true; }

private:
    KeyBindings keys_;
};

// ============================================================================
// AIController - máy đánh. Hành vi lấy hết từ DifficultyProfile.
// ============================================================================
class AIController : public Controller {
public:
    explicit AIController(Difficulty level);
    void Update(const Fighter &self, const Fighter &opponent, float dt) override;

    void SetDifficulty(Difficulty level) { level_ = level; }

private:
    enum class Plan { Approach, Retreat, Attack, Block, Jump, Special, Wait };

    void ChoosePlan(const Fighter &self, const Fighter &opponent);

    Difficulty level_;
    Plan  plan_        = Plan::Approach;
    float thinkTimer_  = 0.0f;
    float actionTimer_ = 0.0f;
    float holdTimer_   = 0.0f;
};

} // namespace fighter
