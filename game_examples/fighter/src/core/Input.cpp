#include "core/Input.hpp"
#include "entities/Fighter.hpp"
#include <cmath>
#include <cstring>

namespace fighter {

KeyBindings KeyBindings::PlayerOne()
{
    KeyBindings k{};
    k.left[0] = KEY_A;  k.right[0] = KEY_D; k.up[0] = KEY_W; k.down[0] = KEY_S;
    k.light[0] = KEY_J; k.medium[0] = KEY_K; k.heavy[0] = KEY_L;
    k.special[0] = KEY_I; k.super[0] = KEY_U;
    k.block[0] = KEY_LEFT_SHIFT;
    return k;
}

KeyBindings KeyBindings::PlayerOneSolo()
{
    KeyBindings k = PlayerOne();
    k.left[1] = KEY_LEFT; k.right[1] = KEY_RIGHT; k.up[1] = KEY_UP; k.down[1] = KEY_DOWN;
    k.light[1] = KEY_Z; k.medium[1] = KEY_X; k.heavy[1] = KEY_C;
    k.special[1] = KEY_V; k.super[1] = KEY_B;
    k.block[1] = KEY_RIGHT_SHIFT;
    return k;
}

KeyBindings KeyBindings::PlayerTwo()
{
    KeyBindings k{};
    k.left[0] = KEY_LEFT; k.right[0] = KEY_RIGHT; k.up[0] = KEY_UP; k.down[0] = KEY_DOWN;
    k.light[0] = KEY_KP_1; k.medium[0] = KEY_KP_2; k.heavy[0] = KEY_KP_3;
    k.special[0] = KEY_KP_4; k.super[0] = KEY_KP_5;
    k.block[0] = KEY_KP_0;
    return k;
}

// ---------------------------------------------------------------------------
// InputBuffer
// ---------------------------------------------------------------------------
void InputBuffer::Clear()
{
    head_ = 0;
    count_ = 0;
}

void InputBuffer::Record(int dir, float now)
{
    if (count_ > 0 && Current() == dir) return;

    const int idx = (head_ + count_) % kSize;
    hist_[idx] = Entry{dir, now};
    if (count_ < kSize) ++count_;
    else                head_ = (head_ + 1) % kSize;
}

const InputBuffer::Entry &InputBuffer::At(int fromNewest) const
{
    return hist_[(head_ + count_ - 1 - fromNewest + kSize * 2) % kSize];
}

// Bàn phím khó bấm chéo chính xác, nên "2" chấp nhận mọi hướng có xuống
// (1/2/3) - giống độ dễ dãi mà các game đối kháng hiện đại vẫn áp dụng.
bool InputBuffer::Matches(char token, int dir)
{
    switch (token) {
        case '2': return dir == 1 || dir == 2 || dir == 3;
        case '8': return dir == 7 || dir == 8 || dir == 9;
        default:  return dir == token - '0';
    }
}

bool InputBuffer::Motion(const char *seq, float now, float window) const
{
    const int len = (int)std::strlen(seq);
    if (len == 0 || count_ == 0) return false;

    // Hướng cuối của lệnh phải là hướng đang giữ. Ngoại lệ: người chơi bàn phím
    // hay nhả phím hướng một nhịp rồi mới bấm đòn, nên cho phép bỏ qua MỘT hướng
    // lạc mới xuất hiện trong 0.15 giây.
    int i = 0;
    if (!Matches(seq[len - 1], At(0).dir)) {
        if (count_ > 1 && now - At(0).time < 0.15f && Matches(seq[len - 1], At(1).dir)) i = 1;
        else return false;
    }

    int s = len - 1;
    for (; i < count_ && s >= 0; ++i) {
        const Entry &e = At(i);
        // Dùng thời điểm hướng này được NHẢ ra: ngồi giữ xuống thật lâu rồi mới
        // quay 36 vẫn tính là 236, giống cách game đối kháng đọc lệnh.
        const float endTime = (i == 0) ? now : At(i - 1).time;
        if (now - endTime > window) return false;
        if (Matches(seq[s], e.dir)) --s;
    }
    return s < 0;
}

// ---------------------------------------------------------------------------
// Người chơi thật
// ---------------------------------------------------------------------------
void HumanController::Update(const Fighter &self, const Fighter &opponent, float dt)
{
    (void)self; (void)opponent; (void)dt;
    state_.Clear();

    auto down = [](const int (&keys)[KeyBindings::kSlots]) {
        for (int k : keys) if (k != KEY_NULL && IsKeyDown(k)) return true;
        return false;
    };
    auto pressed = [](const int (&keys)[KeyBindings::kSlots]) {
        for (int k : keys) if (k != KEY_NULL && IsKeyPressed(k)) return true;
        return false;
    };

    state_.left  = down(keys_.left);
    state_.right = down(keys_.right);
    state_.up    = down(keys_.up);
    state_.down  = down(keys_.down);
    state_.block = down(keys_.block);

    state_.lightPressed   = pressed(keys_.light);
    state_.mediumPressed  = pressed(keys_.medium);
    state_.heavyPressed   = pressed(keys_.heavy);
    state_.specialPressed = pressed(keys_.special);
    state_.superPressed   = pressed(keys_.super);
}

// ---------------------------------------------------------------------------
// Hình nộm luyện tập
// ---------------------------------------------------------------------------
const char *DummyController::ModeName(Mode m)
{
    switch (m) {
        case Mode::Stand:  return "Đứng yên";
        case Mode::Crouch: return "Ngồi";
        case Mode::Block:  return "Tự đỡ đòn";
        case Mode::Jump:   return "Nhảy liên tục";
        default:           return "";
    }
}

void DummyController::Update(const Fighter &self, const Fighter &opponent, float dt)
{
    state_.Clear();
    timer_ += dt;

    const bool oppRight = opponent.Position().x > self.Position().x;
    switch (mode_) {
        case Mode::Crouch:
            state_.down = true;
            break;
        case Mode::Block: {
            // Giữ lùi; ngồi đỡ nếu đòn sắp tới là đòn thấp.
            state_.left  = oppRight;
            state_.right = !oppRight;
            state_.down  = opponent.CurrentAttackIsLow();
            break;
        }
        case Mode::Jump:
            if (self.OnGround() && timer_ > 0.5f) { state_.up = true; timer_ = 0.0f; }
            break;
        default:
            break;
    }
}

} // namespace fighter
