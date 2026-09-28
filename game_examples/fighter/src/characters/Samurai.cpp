// ============================================================================
// MACK - lãng khách. Bộ chiêu kiểu "shoto" quen thuộc của Street Fighter:
//   A  ↓↘→ + đòn   Kiếm Khí         - khí kiếm bay (đạn)
//   B  ↓↙← + đòn   Toàn Phong Trảm  - xoay người chém lướt tới, dùng được trên không
//   C  →↓↘ + đòn   Thăng Long Trảm  - chém vọt lên, bất tử lúc khởi động (chống nhảy)
//   D  ↓ ↓ + đòn   Phản Kiếm        - thế thủ: bị đánh trúng thì phản một nhát
//   Super          Tam Liên Trảm    - lao tới năm nhát + khí kiếm lớn
// ============================================================================
#include "characters/Characters.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include "MoveHelpers.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

MoveDef Samurai::Special(MoveId id, Strength s) const
{
    switch (id) {
        case MoveId::SpecialA: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack1, "Kiếm Khí");
            m.startup = 0.14f; m.active = 0.06f; m.recovery = 0.30f;
            m.box = {0, 0, 0, 0};   // sát thương nằm ở quả đạn
            return m;
        }
        case MoveId::SpecialB: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack1, "Toàn Phong Trảm");
            m.startup = 0.10f;
            m.active = ByStrength(s, 0.36f, 0.48f, 0.60f);
            m.recovery = 0.22f;
            m.setActiveVel = true;
            m.activeVel = {ByStrength(s, 300, 360, 420), -150};
            m.noGravity = true;
            m.spin = true;
            m.hits = (int)ByStrength(s, 2, 3, 4);
            m.hitSpacing = 0.11f;
            m.box = {-72, -170, 144, 96};   // xoay tròn: trúng cả hai phía
            m.damage = 36; m.knockback = 170; m.hitstun = 0.30f; m.blockstun = 0.16f;
            m.finisherKnockdown = true;
            m.fx = SlashFx::Spin; m.fxRadius = 100;
            m.airOk = true;
            return m;
        }
        case MoveId::SpecialC: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack2, "Thăng Long Trảm");
            m.startup = 0.05f; m.active = 0.22f;
            m.recovery = ByStrength(s, 0.30f, 0.36f, 0.42f);
            m.invulnFrom = 0.0f;
            m.invulnTo   = ByStrength(s, 0.0f, 0.12f, 0.20f);
            m.setActiveVel = true;
            m.activeVel = {ByStrength(s, 70, 110, 150), ByStrength(s, -720, -860, -980)};
            m.box = {6, -236, 92, 210};
            m.damage = (int)ByStrength(s, 88, 104, 120);
            m.hits = (s == Strength::Heavy) ? 2 : 1;
            m.hitSpacing = 0.1f;
            m.knockback = 260; m.hitstun = 0.5f; m.blockstun = 0.22f;
            m.launcher = true; m.launchVy = -820; m.knockdown = true;
            m.fx = SlashFx::Rising; m.fxRadius = 125;
            m.sfx = SfxWeight::Heavy;
            return m;
        }
        case MoveId::SpecialD: {
            // Thế phản: đứng thủ, không có hitbox. Đòn thật nằm ở CounterStrike().
            MoveDef m = SpecialBase(id, s, AnimId::Attack2, "Phản Kiếm");
            m.frameFrom = 0; m.frameTo = 0;
            m.startup = 0.04f;
            m.active = ByStrength(s, 0.30f, 0.40f, 0.50f);
            m.recovery = 0.30f;
            m.box = {0, 0, 0, 0};
            m.superCancel = false;
            m.voice = false;
            return m;
        }
        default:
            return Normal(MoveId::StandM);
    }
}

MoveDef Samurai::CounterStrike() const
{
    MoveDef m = SpecialBase(MoveId::SpecialD, Strength::Heavy, AnimId::Attack2, "Phản Kiếm");
    m.startup = 0.03f; m.active = 0.10f; m.recovery = 0.30f;
    m.invulnFrom = 0.0f; m.invulnTo = 0.2f;
    m.box = {-30, -200, 190, 190};
    m.damage = 130; m.knockback = 420; m.hitstun = 0.6f;
    m.knockdown = true;
    m.fx = SlashFx::Overhead; m.fxRadius = 150;
    m.sfx = SfxWeight::Heavy;
    return m;
}

MoveDef Samurai::SuperMove() const
{
    MoveDef m = SuperBase(AnimId::Attack1, "Tam Liên Trảm");
    m.startup = 0.10f; m.active = 0.55f; m.recovery = 0.40f;
    m.setActiveVel = true;
    m.activeVel = {520, 0};
    m.hits = 5; m.hitSpacing = 0.1f;
    m.box = {10, -196, 160, 170};
    m.damage = 46; m.knockback = 160; m.hitstun = 0.45f; m.blockstun = 0.2f;
    m.finisherKnockdown = true;
    m.fx = SlashFx::Horizontal; m.fxRadius = 130;
    return m;
}

bool Samurai::CanUseSpecial(Arena &arena, MoveId id) const
{
    // Luật Street Fighter: mỗi lúc chỉ một quả khí kiếm trên màn hình.
    if (id == MoveId::SpecialA) return arena.CountProjectiles(arena.IdOf(*this)) == 0;
    return true;
}

bool Samurai::OnIncomingHit(Arena &arena, Fighter &attacker, const MoveDef &move)
{
    const bool inStance = State() == FighterState::Attack && move_.id == MoveId::SpecialD &&
                          move_.frameTo == 0 &&
                          moveTimer_ >= move_.startup && moveTimer_ < move_.startup + move_.active;
    if (!inStance || move.height == HitHeight::Throw) return false;

    // Bắt được đòn: dịch sát đối thủ và chém trả.
    counterGlow_ = 0.5f;
    FaceTowards(attacker.Position().x);
    arena.Emit({EventType::Counter, arena.IdOf(*this), Center(), SfxWeight::Heavy});
    arena.HitStop(0.14f);
    arena.Shake(6.0f);
    arena.Fx().Spark(Center(), Color{180, 220, 255, 255}, 1.6f);
    arena.Fx().Ring(Center(), def_.accent, 40.0f);
    StartMove(arena, CounterStrike());
    return true;
}

void Samurai::OnMoveActive(Arena &arena, const MoveDef &m)
{
    const int id = arena.IdOf(*this);
    if (m.id == MoveId::SpecialA) {
        const float speed = ByStrength(m.strength, 430, 580, 740);
        const Vector2 muzzle{pos_.x + FacingSign() * 70.0f, pos_.y - 104.0f};
        arena.SpawnProjectile(std::unique_ptr<Projectile>(
            new WaveProjectile(muzzle, FacingSign(), id, 72, speed, def_.accent, 1.0f)));
        arena.Fx().Burst(muzzle, 14, def_.accent, ParticleKind::Energy, 0.8f);
    }
    if (m.id == MoveId::SpecialC) {
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 12, 0.0f);
        arena.Fx().Burst(Center(), 16, Color{255, 150, 60, 255}, ParticleKind::Energy, 1.0f);
    }
    if (m.id == MoveId::Super) {
        waveFired_ = false;
        trailTimer_ = 0.0f;
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 16, -FacingSign());
    }
    if (m.id == MoveId::SpecialD && m.frameTo == 0) counterGlow_ = m.active;
}

void Samurai::OnMoveUpdate(Arena &arena, const MoveDef &m, float t, float dt)
{
    // Thăng Long Trảm: vệt lửa theo người khi vọt lên.
    if (m.id == MoveId::SpecialC && t > m.startup && t < m.startup + m.active) {
        arena.Fx().Trail(Vector2{pos_.x + FacingSign() * 40.0f, pos_.y - 120.0f},
                         Color{255, 140, 50, 255}, 9.0f);
    }
    // Toàn Phong Trảm: gió xoáy quanh người.
    if (m.id == MoveId::SpecialB && t > m.startup && t < m.startup + m.active) {
        trailTimer_ -= dt;
        if (trailTimer_ <= 0.0f) {
            trailTimer_ = 0.1f;
            SpawnTrail(SlashFx::Spin, 95.0f, def_.accent);
        }
    }
    // Siêu chiêu: mỗi nhát một vệt, cuối cùng phóng khí kiếm lớn.
    if (m.id == MoveId::Super && t > m.startup) {
        trailTimer_ -= dt;
        if (t < m.startup + m.active && trailTimer_ <= 0.0f) {
            trailTimer_ = m.hitSpacing;
            static const SlashFx order[] = {SlashFx::Horizontal, SlashFx::Overhead, SlashFx::Rising};
            SpawnTrail(order[GetRandomValue(0, 2)], 140.0f, def_.accent);
        }
        if (!waveFired_ && t >= m.startup + m.active - 0.04f) {
            waveFired_ = true;
            const Vector2 muzzle{pos_.x + FacingSign() * 70.0f, pos_.y - 110.0f};
            auto *wave = new WaveProjectile(muzzle, FacingSign(), arena.IdOf(*this), 90, 820,
                                            Color{255, 220, 140, 255}, 1.6f);
            wave->MarkSuper();
            arena.SpawnProjectile(std::unique_ptr<Projectile>(wave));
            arena.Shake(8.0f);
        }
    }
}

void Samurai::UpdateCharacter(Arena &arena, float dt)
{
    (void)arena;
    counterGlow_ = std::max(0.0f, counterGlow_ - dt);
}

void Samurai::DrawFront() const
{
    // Thế phản đòn: vòng sáng xanh nhấp nháy quanh người.
    if (counterGlow_ <= 0.0f || State() != FighterState::Attack || move_.id != MoveId::SpecialD) return;
    const Vector2 c = Center();
    const float pulse = 0.6f + 0.4f * std::sin(clock_ * 40.0f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRing(c, 64.0f, 70.0f, 0, 360, 40, Color{150, 210, 255, (unsigned char)(160 * pulse)});
    DrawRing(c, 52.0f, 56.0f, 0, 360, 40, Color{255, 255, 255, (unsigned char)(90 * pulse)});
    EndBlendMode();
}

} // namespace fighter
