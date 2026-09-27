#include "core/Input.hpp"
#include "core/Config.hpp"
#include "core/Difficulty.hpp"
#include "entities/Fighter.hpp"
#include <cmath>
#include <cstdlib>

namespace fighter {

KeyBindings KeyBindings::PlayerOne()
{
    KeyBindings k{};
    k.left = KEY_A;  k.right = KEY_D; k.up = KEY_W; k.down = KEY_S;
    k.light = KEY_J; k.heavy = KEY_K; k.special = KEY_L; k.super = KEY_U;
    k.block = KEY_LEFT_SHIFT;
    return k;
}

KeyBindings KeyBindings::PlayerTwo()
{
    KeyBindings k{};
    k.left = KEY_LEFT; k.right = KEY_RIGHT; k.up = KEY_UP; k.down = KEY_DOWN;
    k.light = KEY_KP_1; k.heavy = KEY_KP_2; k.special = KEY_KP_3; k.super = KEY_KP_5;
    k.block = KEY_KP_0;
    return k;
}

// ---------------------------------------------------------------------------
// Người chơi thật
// ---------------------------------------------------------------------------
void HumanController::Update(const Fighter &self, const Fighter &opponent, float dt)
{
    (void)self; (void)opponent; (void)dt;
    state_.Clear();

    state_.left  = IsKeyDown(keys_.left);
    state_.right = IsKeyDown(keys_.right);
    state_.up    = IsKeyDown(keys_.up);
    state_.down  = IsKeyDown(keys_.down);
    state_.block = IsKeyDown(keys_.block);

    state_.lightPressed   = IsKeyPressed(keys_.light);
    state_.heavyPressed   = IsKeyPressed(keys_.heavy);
    state_.specialPressed = IsKeyPressed(keys_.special);
    state_.superPressed   = IsKeyPressed(keys_.super);
    state_.jumpPressed    = IsKeyPressed(keys_.up);
}

// ---------------------------------------------------------------------------
// Máy
// ---------------------------------------------------------------------------
AIController::AIController(Difficulty level) : level_(level) {}

namespace {
float Chance() { return (float)GetRandomValue(0, 1000) / 1000.0f; }
}

void AIController::ChoosePlan(const Fighter &self, const Fighter &opponent)
{
    const DifficultyProfile &p = GetDifficulty(level_);
    const float dist = std::fabs(opponent.Position().x - self.Position().x);

    // Ưu tiên 1: đối thủ đang vung đòn ở cự ly gần -> đỡ.
    if (opponent.IsAttacking() && dist < 190.0f && Chance() < p.blockChance) {
        plan_ = Plan::Block;
        holdTimer_ = 0.22f + Chance() * 0.18f;
        return;
    }

    // Ưu tiên 2: đầy thanh Super và ở trong tầm -> tung chiêu cuối.
    if (self.MeterRatio() >= 0.999f && dist < 260.0f && Chance() < p.aggression) {
        plan_ = Plan::Special;   // Special ở đây hiểu là "tung chiêu", chọn loại bên dưới
        holdTimer_ = 0.3f;
        return;
    }

    // Ưu tiên 3: đối thủ nhảy cao -> lùi lại hoặc đánh chặn.
    if (!opponent.OnGround() && dist < 240.0f) {
        plan_ = (Chance() < p.aggression) ? Plan::Attack : Plan::Retreat;
        holdTimer_ = 0.3f;
        return;
    }

    if (dist > 430.0f) {
        // Xa: áp sát, thỉnh thoảng bắn chiêu tầm xa.
        plan_ = (Chance() < 0.25f * p.aggression && self.SpecialCooldown() <= 0.0f)
                ? Plan::Special : Plan::Approach;
    } else if (dist > 175.0f) {
        const float r = Chance();
        if (r < p.aggression * 0.7f)      plan_ = Plan::Approach;
        else if (r < p.aggression * 0.85f) plan_ = Plan::Jump;
        else                               plan_ = Plan::Wait;
    } else {
        const float r = Chance();
        if (r < p.aggression)              plan_ = Plan::Attack;
        else if (r < p.aggression + 0.2f)  plan_ = Plan::Block;
        else                               plan_ = Plan::Retreat;
    }
    holdTimer_ = 0.18f + Chance() * 0.3f;
}

void AIController::Update(const Fighter &self, const Fighter &opponent, float dt)
{
    const DifficultyProfile &p = GetDifficulty(level_);
    state_.Clear();

    // Máy đã thua/thắng thì thôi bấm phím.
    if (self.IsDefeated() || opponent.IsDefeated()) return;

    thinkTimer_  -= dt;
    holdTimer_   -= dt;
    actionTimer_ -= dt;

    if (thinkTimer_ <= 0.0f || holdTimer_ <= 0.0f) {
        ChoosePlan(self, opponent);
        thinkTimer_ = p.reaction;
    }

    const float dx   = opponent.Position().x - self.Position().x;
    const bool  oppRight = dx > 0.0f;
    const float dist = std::fabs(dx);

    switch (plan_) {
        case Plan::Approach:
            state_.right = oppRight;
            state_.left  = !oppRight;
            break;

        case Plan::Retreat:
            state_.right = !oppRight;
            state_.left  = oppRight;
            break;

        case Plan::Block:
            state_.block = true;
            break;

        case Plan::Jump:
            state_.right = oppRight;
            state_.left  = !oppRight;
            if (self.OnGround() && actionTimer_ <= 0.0f) {
                state_.up = state_.jumpPressed = true;
                actionTimer_ = 0.7f;
            }
            break;

        case Plan::Attack:
            if (actionTimer_ <= 0.0f && dist < 200.0f) {
                // Đánh nhẹ để mở đòn, nặng khi chắc ăn - tỉ lệ nối đòn theo độ khó.
                if (Chance() < p.comboChance) state_.heavyPressed = true;
                else                           state_.lightPressed = true;
                actionTimer_ = 0.34f - p.aggression * 0.12f;
            } else if (dist >= 200.0f) {
                state_.right = oppRight;
                state_.left  = !oppRight;
            }
            break;

        case Plan::Special:
            if (actionTimer_ <= 0.0f) {
                if (self.MeterRatio() >= 0.999f) state_.superPressed = true;
                else                             state_.specialPressed = true;
                actionTimer_ = 0.8f;
            }
            break;

        case Plan::Wait:
        default:
            break;
    }
}

} // namespace fighter
