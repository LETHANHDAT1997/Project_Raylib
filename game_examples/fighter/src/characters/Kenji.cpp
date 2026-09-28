// ============================================================================
// KENJI - ninja áp sát. Nhanh, nhiều cách tiếp cận, máu mỏng:
//   A  ↓↘→ + đòn   Phi Tiêu     - phi tiêu rất nhanh (H: ném ba chiếc toả ra)
//   B  ↓↙← + đòn   Ảnh Bộ       - lướt xuyên qua người đối thủ, xuyên cả đạn
//   C  →↓↘ + đòn   Ưng Trảo     - bật cao rồi bổ nhào chéo (đòn trên, phải đứng đỡ)
//   D  ↓ ↓ + đòn   Thế Thân     - biến mất trong khói, hiện ra sau lưng (H: rút lui)
//   Super          Loạn Ảnh Kiếm - dịch chuyển quanh đối thủ chém bảy nhát
// ============================================================================
#include "characters/Characters.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include "MoveHelpers.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

MoveDef Kenji::Special(MoveId id, Strength s) const
{
    switch (id) {
        case MoveId::SpecialA: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack1, "Phi Tiêu");
            m.frameTo = 1;   // động tác vung tay ném, trước frame có vệt chém
            m.startup = 0.08f; m.active = 0.04f; m.recovery = 0.22f;
            m.box = {0, 0, 0, 0};
            m.sfx = SfxWeight::Light;
            m.airOk = true;
            return m;
        }
        case MoveId::SpecialB: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack1, "Ảnh Bộ");
            m.startup = 0.06f; m.active = 0.22f; m.recovery = 0.24f;
            m.setActiveVel = true;
            m.activeVel = {ByStrength(s, 900, 1100, 1300), 0};
            m.passThrough = true;
            m.projInvuln = true;
            m.box = {-40, -172, 150, 96};
            m.damage = (int)ByStrength(s, 74, 82, 90);
            // Đẩy rất nhẹ: người lướt phải xuyên sang được phía bên kia.
            m.knockback = 60; m.hitstun = 0.42f; m.blockstun = 0.18f;
            m.fx = SlashFx::Thrust; m.fxRadius = 180;
            return m;
        }
        case MoveId::SpecialC: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack2, "Ưng Trảo");
            m.startup = 0.16f; m.active = 0.5f; m.recovery = 0.16f;
            m.setStartVel = true;
            m.startVel = {60, -1000};
            m.setActiveVel = true;
            m.activeVel = {ByStrength(s, 560, 680, 800), 760};
            m.landCancel = true;
            m.box = {4, -126, 104, 118};
            m.height = HitHeight::Overhead;
            m.damage = (int)ByStrength(s, 76, 84, 94);
            m.knockback = 240; m.hitstun = 0.40f; m.blockstun = 0.2f;
            m.fx = SlashFx::AirDiag; m.fxRadius = 120;
            m.airOk = true;
            return m;
        }
        case MoveId::SpecialD: {
            MoveDef m = SpecialBase(id, s, AnimId::Idle, "Thế Thân");
            m.frameFrom = 0; m.frameTo = 0;
            m.startup = 0.14f; m.active = 0.03f; m.recovery = 0.20f;
            m.invulnFrom = 0.0f; m.invulnTo = 0.22f;
            m.box = {0, 0, 0, 0};
            m.voice = false;
            return m;
        }
        default:
            return Normal(MoveId::StandM);
    }
}

MoveDef Kenji::SuperMove() const
{
    MoveDef m = SuperBase(AnimId::Attack2, "Loạn Ảnh Kiếm");
    m.startup = 0.12f; m.active = 0.84f; m.recovery = 0.36f;
    m.invulnTo = 0.96f;               // bất tử suốt pha chém
    m.hits = 7; m.hitSpacing = 0.12f;
    m.box = {-90, -210, 250, 210};
    m.damage = 34; m.knockback = 60; m.hitstun = 0.5f; m.blockstun = 0.16f;
    m.finisherKnockdown = true;
    m.noGravity = true;
    m.passThrough = true;
    return m;
}

bool Kenji::CanUseSpecial(Arena &arena, MoveId id) const
{
    if (id == MoveId::SpecialA) return arena.CountProjectiles(arena.IdOf(*this)) == 0;
    return true;
}

void Kenji::PushAfterimage()
{
    Afterimage a;
    a.pos = pos_;
    a.life = 0.26f;
    a.facingRight = facingRight_;
    a.frame = animator_.Frame();
    a.anim = animator_.Current();
    trail_.push_back(a);
}

void Kenji::OnMoveStart(Arena &arena, const MoveDef &m)
{
    if (m.id == MoveId::SpecialD) {
        arena.Fx().Smoke(Center(), 14, Color{70, 70, 90, 200});
    }
    if (m.id == MoveId::Super) {
        warpTimer_ = 0.0f;
        warpSide_ = 1;
    }
}

void Kenji::OnMoveActive(Arena &arena, const MoveDef &m)
{
    const int id = arena.IdOf(*this);

    if (m.id == MoveId::SpecialA) {
        const Vector2 muzzle{pos_.x + FacingSign() * 56.0f, pos_.y - (onGround_ ? 118.0f : 90.0f)};
        const float speed = ByStrength(m.strength, 720, 860, 980);
        const int count = (m.strength == Strength::Heavy) ? 3 : 1;
        for (int i = 0; i < count; ++i) {
            const float spread = (count == 1) ? 0.0f : (i - 1) * 0.16f;
            // Trên không thì ném chéo xuống.
            const float down = onGround_ ? 0.0f : 0.45f;
            const Vector2 v{FacingSign() * speed * std::cos(spread), speed * (spread + down)};
            arena.SpawnProjectile(std::unique_ptr<Projectile>(
                new ShurikenProjectile(muzzle, v, id, count == 1 ? 48 : 30, def_.accent)));
        }
        return;
    }

    if (m.id == MoveId::SpecialB) {
        PushAfterimage();
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 12, -FacingSign());
        return;
    }

    if (m.id == MoveId::SpecialD && opponent_) {
        // Hiện ra sau lưng đối thủ; lực H thì rút lui ra xa.
        const float ox = opponent_->Position().x;
        const float side = (ox > pos_.x) ? 1.0f : -1.0f;
        float target = (m.strength == Strength::Heavy) ? ox - side * 520.0f : ox + side * 90.0f;
        target = std::clamp(target, kStageLeft + 20.0f, kStageRight - 20.0f);
        const float lo = arena.CameraX() + 70.0f, hi = arena.CameraX() + arena.ViewWidth() - 70.0f;
        target = std::clamp(target, lo, hi);

        arena.Fx().Smoke(Center(), 10, Color{70, 70, 90, 200});
        pos_.x = target;
        FaceTowards(ox);
        arena.Fx().Smoke(Center(), 14, Color{70, 70, 90, 200});
        arena.Emit({EventType::Teleport, id, Center()});
    }
}

void Kenji::OnMoveUpdate(Arena &arena, const MoveDef &m, float t, float dt)
{
    const bool active = t >= m.startup && t < m.startup + m.active;

    if ((m.id == MoveId::SpecialB || m.id == MoveId::SpecialC) && active) {
        ghostTimer_ -= dt;
        if (ghostTimer_ <= 0.0f) { ghostTimer_ = 0.03f; PushAfterimage(); }
    }

    // Siêu chiêu: mỗi nhát lại "chớp" sang một bên đối thủ.
    if (m.id == MoveId::Super && active && opponent_) {
        warpTimer_ -= dt;
        if (warpTimer_ <= 0.0f) {
            warpTimer_ = m.hitSpacing;
            warpSide_ = -warpSide_;
            PushAfterimage();
            const Vector2 o = opponent_->Position();
            pos_.x = std::clamp(o.x + warpSide_ * 80.0f, kStageLeft, kStageRight);
            pos_.y = std::min(kGroundY, o.y);
            onGround_ = pos_.y >= kGroundY;
            vel_ = Vector2{0.0f, 0.0f};
            FaceTowards(o.x);
            SpawnTrail(warpSide_ > 0 ? SlashFx::Overhead : SlashFx::Rising, 130.0f, def_.accent);
            arena.Fx().Burst(opponent_->Center(), 6, def_.accent, ParticleKind::Shard, 0.8f);
        }
    }
}

void Kenji::UpdateCharacter(Arena &arena, float dt)
{
    (void)arena;
    for (auto &a : trail_) a.life -= dt;
    trail_.erase(std::remove_if(trail_.begin(), trail_.end(),
                                [](const Afterimage &a) { return a.life <= 0.0f; }),
                 trail_.end());
}

void Kenji::DrawBehind() const
{
    for (const auto &a : trail_) {
        if (!a.anim) continue;
        const float k = std::clamp(a.life / 0.26f, 0.0f, 1.0f);
        Color tint = def_.accent;
        tint.a = (unsigned char)(130 * k);
        DrawSpriteFrame(*a.anim, a.frame, a.pos, a.facingRight, tint);
    }
}

} // namespace fighter
