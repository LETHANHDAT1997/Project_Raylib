#pragma once
// Tiện ích nội bộ cho các file nhân vật - không lộ ra ngoài thư mục src/.
#include "entities/Move.hpp"

namespace fighter {

// Chọn giá trị theo lực L / M / H của chiêu.
inline float ByStrength(Strength s, float l, float m, float h)
{
    return s == Strength::Light ? l : (s == Strength::Medium ? m : h);
}

// Khung chung của một chiêu đặc biệt: hủy được sang siêu chiêu, có tiếng hét.
inline MoveDef SpecialBase(MoveId id, Strength s, AnimId anim, const char *label)
{
    MoveDef m;
    m.id = id;
    m.strength = s;
    m.anim = anim;
    m.superCancel = true;
    m.voice = true;
    m.sfx = SfxWeight::Medium;
    m.meterGain = 10.0f;
    m.label = label;
    return m;
}

// Khung chung của siêu chiêu: tốn 1 thanh, bất tử lúc khởi động.
inline MoveDef SuperBase(AnimId anim, const char *label)
{
    MoveDef m;
    m.id = MoveId::Super;
    m.strength = Strength::Heavy;
    m.anim = anim;
    m.meterCost = 100.0f;
    m.meterGain = 0.0f;
    m.invulnFrom = 0.0f;
    m.invulnTo = 0.2f;
    m.sfx = SfxWeight::Heavy;
    m.voice = true;
    m.label = label;
    return m;
}

} // namespace fighter
