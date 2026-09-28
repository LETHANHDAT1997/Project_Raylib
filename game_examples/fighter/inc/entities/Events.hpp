#pragma once
#include "raylib.h"
#include "entities/Move.hpp"
#include <vector>

namespace fighter {

// ============================================================================
// Sự kiện trong trận. Arena và Fighter chỉ việc đẩy sự kiện vào hàng đợi;
// BattleScene đọc ra để phát âm thanh, hiện chữ "COUNTER", rung tay cầm...
// Nhờ vậy phần mô phỏng không phụ thuộc vào âm thanh hay giao diện.
// ============================================================================
enum class EventType {
    MoveStart,      // bắt đầu vung đòn (tiếng gió)
    SpecialStart,   // bắt đầu chiêu đặc biệt (tiếng hét)
    SuperStart,     // siêu chiêu - màn hình tối lại
    Hit,
    Block,
    CounterHit,
    Throw,
    ThrowTech,
    Knockdown,
    Land,
    Jump,
    Dash,
    Projectile,
    Clash,          // hai quả đạn triệt tiêu nhau
    Counter,        // thế phản đòn kích hoạt
    Teleport,
    Thunder,
    KO,
};

struct GameEvent {
    EventType type;
    int       who = 0;          // 0 = P1, 1 = P2 (người gây ra)
    Vector2   pos{};
    SfxWeight weight = SfxWeight::Light;
    int       value  = 0;       // sát thương...
};

using EventQueue = std::vector<GameEvent>;

} // namespace fighter
