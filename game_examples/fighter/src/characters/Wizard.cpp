// ============================================================================
// MALTHUS - hắc pháp sư, lối đánh giữ khoảng cách (zoner):
//   A  ↓↘→ + đòn   Hắc Cầu     - cầu năng lượng (L chậm, H nhanh)
//   B  ↓↙← + đòn   Hắc Trụ     - cột đen trồi lên ở xa (L gần, H xa), hất tung
//   C  →↓↘ + đòn   Hắc Bạo     - bộc phá quanh người, bất tử lúc khởi động
//   D  ↓ ↓ + đòn   Dịch Chuyển - L: sau lưng đối thủ, M: trước mặt, H: về cuối sân
//   Super          Thiên Phạt  - gọi năm tia sét giáng dọc sàn đấu
// ============================================================================
#include "characters/Characters.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include "MoveHelpers.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

MoveDef Wizard::Special(MoveId id, Strength s) const
{
    switch (id) {
        case MoveId::SpecialA: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack1, "Hắc Cầu");
            m.startup = 0.16f; m.active = 0.06f; m.recovery = 0.32f;
            m.box = {0, 0, 0, 0};
            return m;
        }
        case MoveId::SpecialB: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack2, "Hắc Trụ");
            m.startup = 0.18f; m.active = 0.05f; m.recovery = 0.42f;
            m.box = {0, 0, 0, 0};
            return m;
        }
        case MoveId::SpecialC: {
            MoveDef m = SpecialBase(id, s, AnimId::Attack2, "Hắc Bạo");
            m.startup = 0.06f; m.active = 0.16f; m.recovery = 0.42f;
            m.invulnFrom = 0.0f; m.invulnTo = ByStrength(s, 0.06f, 0.14f, 0.2f);
            m.box = {-104, -240, 208, 240};
            m.damage = (int)ByStrength(s, 90, 102, 114);
            m.knockback = 280; m.hitstun = 0.5f; m.blockstun = 0.22f;
            m.launcher = true; m.launchVy = -780; m.knockdown = true;
            m.sfx = SfxWeight::Heavy;
            return m;
        }
        case MoveId::SpecialD: {
            MoveDef m = SpecialBase(id, s, AnimId::Idle, "Dịch Chuyển");
            m.frameFrom = 0; m.frameTo = 0;
            m.startup = 0.16f; m.active = 0.03f; m.recovery = 0.24f;
            m.invulnFrom = 0.0f; m.invulnTo = 0.24f;
            m.box = {0, 0, 0, 0};
            m.voice = false;
            return m;
        }
        default:
            return Normal(MoveId::StandM);
    }
}

MoveDef Wizard::SuperMove() const
{
    MoveDef m = SuperBase(AnimId::Attack2, "Thiên Phạt");
    m.startup = 0.20f; m.active = 0.10f; m.recovery = 0.70f;
    m.box = {0, 0, 0, 0};
    return m;
}

bool Wizard::CanUseSpecial(Arena &arena, MoveId id) const
{
    const int mine = arena.CountProjectiles(arena.IdOf(*this));
    if (id == MoveId::SpecialA) return mine == 0;
    if (id == MoveId::SpecialB) return mine <= 1;
    return true;
}

void Wizard::OnMoveStart(Arena &arena, const MoveDef &m)
{
    if (m.id == MoveId::SpecialD) arena.Fx().Smoke(Center(), 12, Color{80, 40, 120, 190});
}

void Wizard::OnMoveActive(Arena &arena, const MoveDef &m)
{
    const int id = arena.IdOf(*this);
    const float face = FacingSign();

    switch (m.id) {
        case MoveId::SpecialA: {
            const Vector2 muzzle{pos_.x + face * 70.0f, pos_.y - 118.0f};
            arena.SpawnProjectile(std::unique_ptr<Projectile>(
                new OrbProjectile(muzzle, face, id, 78, ByStrength(m.strength, 360, 520, 680), def_.accent)));
            arena.Fx().Burst(muzzle, 14, def_.accent, ParticleKind::Energy, 0.8f);
            break;
        }
        case MoveId::SpecialB: {
            const float x = std::clamp(pos_.x + face * ByStrength(m.strength, 230, 390, 550),
                                       kStageLeft, kStageRight);
            arena.SpawnProjectile(std::unique_ptr<Projectile>(
                new PillarHazard(x, id, 92, def_.accent, 0.32f)));
            break;
        }
        case MoveId::SpecialC:
            burstFx_ = 0.3f;
            arena.Fx().Ring(Center(), def_.accent, 50.0f);
            arena.Fx().Burst(Center(), 26, def_.accent, ParticleKind::Energy, 1.4f);
            arena.Shake(5.0f);
            break;
        case MoveId::SpecialD: {
            if (!opponent_) break;
            const float ox = opponent_->Position().x;
            const float side = (ox > pos_.x) ? 1.0f : -1.0f;
            float target;
            if (m.strength == Strength::Light)       target = ox + side * 100.0f;   // sau lưng
            else if (m.strength == Strength::Medium) target = ox - side * 140.0f;   // trước mặt
            else                                     target = ox - side * 1000.0f;  // về xa
            const float lo = std::max(kStageLeft + 20.0f, arena.CameraX() + 70.0f);
            const float hi = std::min(kStageRight - 20.0f, arena.CameraX() + arena.ViewWidth() - 70.0f);
            arena.Fx().Smoke(Center(), 10, Color{80, 40, 120, 190});
            pos_.x = std::clamp(target, lo, hi);
            FaceTowards(ox);
            arena.Fx().Smoke(Center(), 14, Color{80, 40, 120, 190});
            arena.Fx().Ring(Center(), def_.accent, 30.0f);
            arena.Emit({EventType::Teleport, id, Center()});
            break;
        }
        case MoveId::Super:
            // Năm tia sét lần lượt giáng xuống ĐÚNG chỗ đối thủ đang đứng/bay
            // (gọi dần trong UpdateCharacter), nên bị trúng tia đầu là dính cả chuỗi.
            strikesLeft_ = 5;
            strikeTimer_ = 0.0f;
            break;
        default:
            break;
    }
}

void Wizard::UpdateCharacter(Arena &arena, float dt)
{
    orbPhase_ += dt;
    burstFx_ = std::max(0.0f, burstFx_ - dt);

    if (strikesLeft_ > 0 && opponent_) {
        strikeTimer_ -= dt;
        if (strikeTimer_ <= 0.0f) {
            strikeTimer_ = 0.16f;
            const float jitter = (strikesLeft_ == 5) ? 0.0f : (float)GetRandomValue(-24, 24);
            const float x = std::clamp(opponent_->Position().x + jitter, kStageLeft, kStageRight);
            auto *bolt = new LightningHazard(x, arena.IdOf(*this), 58, def_.accent, 0.0f);
            bolt->MarkSuper();
            arena.SpawnProjectile(std::unique_ptr<Projectile>(bolt));
            --strikesLeft_;
        }
    }
}

void Wizard::DrawBehind() const
{
    // Ba đốm hắc ám bay vờn quanh người - dấu hiệu nhận dạng.
    const Vector2 c = Center();
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 3; ++i) {
        const float ang = orbPhase_ * (1.4f + i * 0.5f) + i * 2.1f;
        const float r   = 58.0f + i * 10.0f;
        const Vector2 p{c.x + std::cos(ang) * r, c.y + std::sin(ang) * r * 0.5f};
        Color col = def_.accent;
        col.a = 50; DrawCircleV(p, 14.0f - i, col);
        col.a = 120; DrawCircleV(p, 6.0f - i, col);
    }
    EndBlendMode();
}

void Wizard::DrawFront() const
{
    if (burstFx_ <= 0.0f) return;
    const float k = 1.0f - burstFx_ / 0.3f;
    const Vector2 c = Center();
    BeginBlendMode(BLEND_ADDITIVE);
    Color a = def_.accent; a.a = (unsigned char)(200 * (1.0f - k));
    DrawRing(c, 40.0f + 110.0f * k, 60.0f + 120.0f * k, 0, 360, 48, a);
    DrawCircleV(c, 70.0f * (1.0f - k), Color{255, 230, 255, (unsigned char)(120 * (1.0f - k))});
    EndBlendMode();
}

} // namespace fighter
