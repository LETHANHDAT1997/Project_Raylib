#pragma once
#include "raylib.h"
#include "core/Animation.hpp"

namespace fighter {

// Bốn "ô chiêu" mà mọi nhân vật đều có. Nội dung từng ô do lớp con định nghĩa,
// nên cùng một phím bấm sẽ ra đòn khác nhau tuỳ nhân vật.
enum class MoveSlot { Light = 0, Heavy, Special, Super, Count };

// ============================================================================
// MoveDef - frame data của một đòn đánh, theo kiểu game đối kháng cổ điển:
// startup (vung tay) -> active (hitbox bật) -> recovery (thu đòn).
// Chỉ trong giai đoạn active mới gây sát thương; recovery dài = dễ bị phản đòn.
// ============================================================================
struct MoveDef {
    AnimId anim       = AnimId::Attack1;
    float  startup    = 0.10f;
    float  active     = 0.08f;
    float  recovery   = 0.18f;

    // Hitbox tính theo gốc toạ độ là điểm giữa hai bàn chân, hướng mặt sang phải.
    // x tăng = ra xa trước mặt, y âm = lên cao.
    Rectangle box     = {40.0f, -150.0f, 90.0f, 60.0f};

    int    damage     = 60;
    float  knockback  = 260.0f;
    float  hitstun    = 0.28f;
    float  blockstun  = 0.16f;
    float  meterGain  = 7.0f;     // thanh Super cộng cho người đánh khi trúng
    float  meterCost  = 0.0f;     // thanh Super tiêu hao khi tung đòn
    float  lunge      = 0.0f;     // tự lao tới trước bao nhiêu px/s
    float  cooldown   = 0.0f;     // thời gian chờ trước khi dùng lại
    bool   launcher   = false;    // hất tung đối thủ lên
    bool   armor      = false;    // chịu 1 đòn không bị khựng (đòn nặng của Knight)
    int    hits       = 1;        // số lần trúng của một lần tung đòn
    float  hitSpacing = 0.05f;    // khoảng cách giữa các lần trúng khi hits > 1
    const char *label = "";
};

// Hitbox đang bật của một nhân vật ở frame hiện tại, đã đổi ra toạ độ màn hình.
struct ActiveAttack {
    bool      active = false;
    Rectangle world{};
    MoveSlot  slot = MoveSlot::Light;
    const MoveDef *def = nullptr;
};

} // namespace fighter
