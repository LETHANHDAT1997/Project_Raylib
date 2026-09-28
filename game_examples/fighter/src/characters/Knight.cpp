// ============================================================================
// GARETH - hiệp sĩ thép, lối đánh "đô vật": chậm, trâu, gần là chết:
//   A  ↓↘→ + đòn   Khiên Xung      - húc thẳng, có giáp (ăn 1 đòn vẫn đi tiếp)
//   B  ↓↙← + đòn   Trảm Địa        - nhảy bổ xuống (đòn trên) + chấn động khi đáp
//   C  →↓↘ + đòn   Nộ Kích         - chém vọt có giáp, hất tung
//   D  ↓ ↓ + đòn   Địa Ngục Quăng  - vật lệnh: không đỡ, không phá được
//   Super          Địa Chấn        - nện đất, sóng chấn động quét ngang sàn
// ============================================================================
#include "characters/Characters.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include "MoveHelpers.hpp"
#include <cmath>

namespace fighter {

MoveDef Knight::Special(MoveId id, Strength s) const
{
    switch (id) {
        case MoveId::SpecialA: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack1, "Khiên Xung");
            m.startup = 0.12f;
            m.active = ByStrength(s, 0.26f, 0.32f, 0.38f);
            m.recovery = 0.30f;
            m.setActiveVel = true;
            m.activeVel = {ByStrength(s, 470, 570, 670), 0};
            m.armor = true;
            m.box = {10, -176, 92, 156};
            m.damage = (int)ByStrength(s, 84, 92, 100);
            m.knockback = 420; m.hitstun = 0.42f; m.blockstun = 0.22f;
            m.knockdown = (s == Strength::Heavy);
            m.fx = SlashFx::Thrust; m.fxRadius = 150;
            m.sfx = SfxWeight::Heavy;
            return m;
        }
        case MoveId::SpecialB: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack2, "Trảm Địa");
            m.startup = 0.26f; m.active = 0.5f; m.recovery = 0.2f;
            m.setStartVel = true;
            m.startVel = {ByStrength(s, 150, 240, 330), -700};
            m.landCancel = true;
            m.box = {4, -160, 128, 160};
            m.height = HitHeight::Overhead;
            m.damage = (int)ByStrength(s, 96, 104, 112);
            m.knockback = 300; m.hitstun = 0.45f; m.blockstun = 0.24f;
            m.knockdown = true;
            m.fx = SlashFx::Overhead; m.fxRadius = 150;
            m.sfx = SfxWeight::Heavy;
            return m;
        }
        case MoveId::SpecialC: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack2, "Nộ Kích");
            m.startup = 0.10f; m.active = 0.18f; m.recovery = 0.42f;
            m.armor = true;
            m.setActiveVel = true;
            m.activeVel = {60, ByStrength(s, -520, -620, -720)};
            m.box = {-4, -256, 116, 220};
            m.damage = (int)ByStrength(s, 100, 110, 122);
            m.knockback = 240; m.hitstun = 0.5f; m.blockstun = 0.24f;
            m.launcher = true; m.launchVy = -760; m.knockdown = true;
            m.fx = SlashFx::Rising; m.fxRadius = 140;
            m.sfx = SfxWeight::Heavy;
            return m;
        }
        case MoveId::SpecialD: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack1, "Địa Ngục Quăng");
            m.frameTo = 2;
            m.startup = 0.07f; m.active = 0.07f; m.recovery = 0.48f;
            m.box = {8, -176, ByStrength(s, 84, 100, 116), 156};
            m.height = HitHeight::Throw;
            m.techable = false;          // vật lệnh: không phá được
            m.slam = true;
            m.throwHold = 0.52f;
            m.damage = (int)ByStrength(s, 170, 185, 200);
            m.knockback = 160;
            m.knockdown = true;
            m.superCancel = false;
            m.sfx = SfxWeight::Heavy;
            return m;
        }
        default:
            return Normal(MoveId::StandM);
    }
}

MoveDef Knight::SuperMove() const
{
    MoveDef m = SuperBase(AnimId::Attack2, "Địa Chấn");
    m.startup = 0.22f; m.active = 0.12f; m.recovery = 0.50f;
    m.armor = true;
    m.box = {10, -210, 150, 210};
    m.damage = 110; m.knockback = 420; m.hitstun = 0.6f; m.blockstun = 0.3f;
    m.knockdown = true;
    m.fx = SlashFx::Overhead; m.fxRadius = 170;
    return m;
}

void Knight::OnMoveActive(Arena &arena, const MoveDef &m)
{
    if (m.id == MoveId::SpecialA) {
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 14, -FacingSign());
        arena.Shake(3.0f);
    }
    if (m.id == MoveId::Super) {
        arena.Shake(20.0f);
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 34, 0.0f);
        arena.Fx().Ring(Vector2{pos_.x + FacingSign() * 60.0f, kGroundY - 10.0f}, def_.accent, 50.0f);
        const Vector2 origin{pos_.x + FacingSign() * 80.0f, kGroundY};
        auto *wave = new ShockwaveProjectile(origin, FacingSign(), arena.IdOf(*this), 130, 900.0f, 1.6f, 1.6f);
        wave->MarkSuper();
        arena.SpawnProjectile(std::unique_ptr<Projectile>(wave));
    }
}

void Knight::OnMoveUpdate(Arena &arena, const MoveDef &m, float t, float dt)
{
    (void)dt;
    // Khiên Xung: bụi cuộn sau lưng khi lao.
    if (m.id == MoveId::SpecialA && t > m.startup && t < m.startup + m.active && GetRandomValue(0, 2) == 0) {
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 2, -FacingSign());
    }
}

void Knight::OnMoveLand(Arena &arena, const MoveDef &m)
{
    // Trảm Địa: đáp xuống tạo hai sóng chấn động ngắn hai bên.
    if (m.id != MoveId::SpecialB) return;
    const int id = arena.IdOf(*this);
    arena.Shake(10.0f);
    arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 26, 0.0f);
    for (float dir : {-1.0f, 1.0f}) {
        arena.SpawnProjectile(std::unique_ptr<Projectile>(
            new ShockwaveProjectile(Vector2{pos_.x + dir * 40.0f, kGroundY}, dir, id, 45, 620.0f, 0.32f, 0.8f)));
    }
}

} // namespace fighter
