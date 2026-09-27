#include "characters/Characters.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

// ===========================================================================
// MACK - lãng khách
// ===========================================================================
const MoveDef &Samurai::Move(MoveSlot slot) const
{
    static const MoveDef kLight = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.07f; m.active = 0.07f; m.recovery = 0.16f;
        m.box = Rectangle{40.0f, -158.0f, 92.0f, 56.0f};
        m.damage = 46; m.knockback = 190.0f; m.hitstun = 0.24f; m.blockstun = 0.14f;
        m.meterGain = 7.0f; m.label = "Chém ngang";
        return m;
    }();

    static const MoveDef kHeavy = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.15f; m.active = 0.09f; m.recovery = 0.28f;
        m.box = Rectangle{42.0f, -172.0f, 116.0f, 84.0f};
        m.damage = 92; m.knockback = 350.0f; m.hitstun = 0.34f; m.blockstun = 0.2f;
        m.meterGain = 10.0f; m.lunge = 190.0f; m.label = "Trảm hạ";
        return m;
    }();

    // Chiêu chống nhảy: người vọt lên cùng lưỡi kiếm, hất tung đối thủ.
    static const MoveDef kSpecial = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.09f; m.active = 0.18f; m.recovery = 0.36f;
        m.box = Rectangle{18.0f, -250.0f, 96.0f, 170.0f};
        m.damage = 112; m.knockback = 300.0f; m.hitstun = 0.42f; m.blockstun = 0.24f;
        m.meterGain = 13.0f; m.launcher = true; m.cooldown = 0.85f;
        m.label = "Thăng Long Trảm";
        return m;
    }();

    // Chiêu cuối: lao tới, ba nhát liên hoàn kèm một đạo khí kiếm.
    static const MoveDef kSuper = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.12f; m.active = 0.34f; m.recovery = 0.34f;
        m.box = Rectangle{36.0f, -175.0f, 128.0f, 96.0f};
        m.damage = 78; m.knockback = 380.0f; m.hitstun = 0.32f; m.blockstun = 0.24f;
        m.meterGain = 0.0f; m.meterCost = kMaxMeter; m.lunge = 430.0f;
        m.hits = 3; m.hitSpacing = 0.09f; m.cooldown = 1.0f;
        m.label = "Tam Liên Trảm";
        return m;
    }();

    switch (slot) {
        case MoveSlot::Heavy:   return kHeavy;
        case MoveSlot::Special: return kSpecial;
        case MoveSlot::Super:   return kSuper;
        default:                return kLight;
    }
}

void Samurai::OnSpecialActivate(Arena &arena)
{
    // Vọt lên khỏi mặt đất - đó là điều khiến đòn này chặn được người nhảy vào.
    ApplyKnockback(FacingSign() * 120.0f, -700.0f);
    slashGlow_ = 0.32f;
    arena.Fx().Burst(Center(), 18, def_.accent, ParticleKind::Energy, 0.9f);
    arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 10, 0.0f);
}

void Samurai::OnSuperActivate(Arena &arena)
{
    slashGlow_ = 0.5f;
    arena.Shake(8.0f);
    arena.Fx().Ring(Center(), def_.accent, 30.0f);

    const Vector2 muzzle{pos_.x + FacingSign() * 60.0f, pos_.y - 95.0f};
    arena.SpawnProjectile(std::unique_ptr<Projectile>(
        new SlashProjectile(muzzle, FacingSign(), arena.IdOf(*this), 70, def_.accent)));
}

void Samurai::OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point)
{
    if (slot == MoveSlot::Special || slot == MoveSlot::Super) {
        arena.Fx().Burst(point, 14, def_.accent, ParticleKind::Shard, 1.1f);
    }
}

void Samurai::UpdateCharacter(Arena &arena, float dt)
{
    (void)arena;
    slashGlow_ = std::max(0.0f, slashGlow_ - dt);
}

void Samurai::DrawFront(Vector2 screenPos, float scale) const
{
    (void)screenPos; (void)scale;
    if (slashGlow_ <= 0.0f) return;

    const float k = slashGlow_ / 0.5f;
    const Vector2 c = Center();
    Color glow = def_.accent;
    glow.a = (unsigned char)(170 * k);
    DrawCircleLines((int)(c.x + FacingSign() * 40.0f), (int)c.y, 60.0f + 40.0f * (1.0f - k), glow);
    glow.a = (unsigned char)(90 * k);
    DrawCircleLines((int)(c.x + FacingSign() * 40.0f), (int)c.y, 44.0f + 60.0f * (1.0f - k), glow);
}

// ===========================================================================
// KENJI - kiếm ảnh
// ===========================================================================
const MoveDef &Kenji::Move(MoveSlot slot) const
{
    static const MoveDef kLight = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.05f; m.active = 0.06f; m.recovery = 0.12f;
        m.box = Rectangle{34.0f, -152.0f, 80.0f, 52.0f};
        m.damage = 38; m.knockback = 150.0f; m.hitstun = 0.20f; m.blockstun = 0.12f;
        m.meterGain = 8.0f; m.label = "Liên thích";
        return m;
    }();

    static const MoveDef kHeavy = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.12f; m.active = 0.08f; m.recovery = 0.24f;
        m.box = Rectangle{36.0f, -178.0f, 104.0f, 92.0f};
        m.damage = 82; m.knockback = 320.0f; m.hitstun = 0.32f; m.blockstun = 0.18f;
        m.meterGain = 11.0f; m.lunge = 250.0f; m.label = "Bạt đao";
        return m;
    }();

    // Lướt xuyên: hitbox rộng, người bay thẳng qua đối thủ.
    static const MoveDef kSpecial = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.06f; m.active = 0.16f; m.recovery = 0.26f;
        m.box = Rectangle{-20.0f, -168.0f, 150.0f, 80.0f};
        m.damage = 86; m.knockback = 270.0f; m.hitstun = 0.34f; m.blockstun = 0.2f;
        m.meterGain = 14.0f; m.lunge = 820.0f; m.cooldown = 0.75f;
        m.label = "Ảnh Bộ";
        return m;
    }();

    static const MoveDef kSuper = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.10f; m.active = 0.42f; m.recovery = 0.30f;
        m.box = Rectangle{28.0f, -176.0f, 124.0f, 92.0f};
        m.damage = 52; m.knockback = 300.0f; m.hitstun = 0.26f; m.blockstun = 0.2f;
        m.meterGain = 0.0f; m.meterCost = kMaxMeter; m.lunge = 340.0f;
        m.hits = 5; m.hitSpacing = 0.075f; m.cooldown = 1.0f;
        m.label = "Loạn Ảnh Kiếm";
        return m;
    }();

    switch (slot) {
        case MoveSlot::Heavy:   return kHeavy;
        case MoveSlot::Special: return kSpecial;
        case MoveSlot::Super:   return kSuper;
        default:                return kLight;
    }
}

void Kenji::OnSpecialActivate(Arena &arena)
{
    dashGlow_ = 0.34f;
    arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 14, -FacingSign());
    arena.Fx().Burst(Center(), 12, def_.accent, ParticleKind::Energy, 0.8f);
}

void Kenji::OnSuperActivate(Arena &arena)
{
    dashGlow_ = 0.6f;
    arena.Shake(7.0f);
    arena.Fx().Ring(Center(), def_.accent, 28.0f);
}

void Kenji::OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point)
{
    (void)slot;
    arena.Fx().Burst(point, 10, def_.accent, ParticleKind::Spark, 0.9f);
}

void Kenji::UpdateCharacter(Arena &arena, float dt)
{
    (void)arena;
    dashGlow_ = std::max(0.0f, dashGlow_ - dt);

    // Bóng mờ để thấy rõ vệt lướt.
    if (dashGlow_ > 0.0f) {
        Afterimage a;
        a.pos = pos_;
        a.life = 0.24f;
        a.facingRight = facingRight_;
        a.frame = animator_.Frame();
        trail_.push_back(a);
    }

    for (auto &a : trail_) a.life -= dt;
    trail_.erase(std::remove_if(trail_.begin(), trail_.end(),
                                [](const Afterimage &a) { return a.life <= 0.0f; }),
                 trail_.end());
}

void Kenji::DrawBehind(Vector2 screenPos, float scale) const
{
    (void)screenPos;
    const Animation *anim = animator_.Current();
    if (!anim || anim->texture.id == 0) return;

    const float side = (float)anim->texture.height;
    for (const auto &a : trail_) {
        const float k = std::clamp(a.life / 0.24f, 0.0f, 1.0f);
        Rectangle src = anim->FrameRect(a.frame);
        if (!a.facingRight) src.width = -src.width;

        const float anchorX = a.facingRight ? def_.anchor.x : (side - def_.anchor.x);
        Rectangle dest{
            a.pos.x - anchorX * scale,
            a.pos.y - def_.anchor.y * scale,
            side * scale, side * scale
        };
        Color tint = def_.accent;
        tint.a = (unsigned char)(120 * k);
        DrawTexturePro(anim->texture, src, dest, Vector2{0.0f, 0.0f}, 0.0f, tint);
    }
}

// ===========================================================================
// GARETH - hiệp sĩ thép
// ===========================================================================
const MoveDef &Knight::Move(MoveSlot slot) const
{
    static const MoveDef kLight = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.10f; m.active = 0.08f; m.recovery = 0.20f;
        m.box = Rectangle{42.0f, -162.0f, 100.0f, 62.0f};
        m.damage = 54; m.knockback = 210.0f; m.hitstun = 0.26f; m.blockstun = 0.16f;
        m.meterGain = 6.0f; m.label = "Vụt kiếm";
        return m;
    }();

    // Đòn nặng có armor: ăn một đòn vẫn vung tiếp - đổi máu lấy sát thương.
    static const MoveDef kHeavy = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.24f; m.active = 0.11f; m.recovery = 0.38f;
        m.box = Rectangle{40.0f, -186.0f, 148.0f, 108.0f};
        m.damage = 124; m.knockback = 430.0f; m.hitstun = 0.40f; m.blockstun = 0.26f;
        m.meterGain = 12.0f; m.armor = true; m.lunge = 130.0f;
        m.label = "Cường trảm";
        return m;
    }();

    static const MoveDef kSpecial = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.12f; m.active = 0.20f; m.recovery = 0.30f;
        m.box = Rectangle{20.0f, -166.0f, 116.0f, 82.0f};
        m.damage = 96; m.knockback = 420.0f; m.hitstun = 0.36f; m.blockstun = 0.22f;
        m.meterGain = 13.0f; m.lunge = 470.0f; m.armor = true; m.cooldown = 0.9f;
        m.label = "Khiên Xung";
        return m;
    }();

    // Chiêu cuối: nện đất, sóng chấn động chạy dọc mặt sàn.
    static const MoveDef kSuper = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.22f; m.active = 0.12f; m.recovery = 0.42f;
        m.box = Rectangle{30.0f, -180.0f, 130.0f, 110.0f};
        m.damage = 130; m.knockback = 420.0f; m.hitstun = 0.42f; m.blockstun = 0.28f;
        m.meterGain = 0.0f; m.meterCost = kMaxMeter; m.cooldown = 1.1f;
        m.label = "Địa Chấn";
        return m;
    }();

    switch (slot) {
        case MoveSlot::Heavy:   return kHeavy;
        case MoveSlot::Special: return kSpecial;
        case MoveSlot::Super:   return kSuper;
        default:                return kLight;
    }
}

void Knight::OnSpecialActivate(Arena &arena)
{
    arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 12, -FacingSign());
    arena.Shake(4.0f);
}

void Knight::OnSuperActivate(Arena &arena)
{
    arena.Shake(14.0f);
    arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 26, 0.0f);
    arena.Fx().Ring(Vector2{pos_.x, kGroundY - 10.0f}, def_.accent, 34.0f);

    const Vector2 origin{pos_.x + FacingSign() * 50.0f, kGroundY};
    arena.SpawnProjectile(std::unique_ptr<Projectile>(
        new ShockwaveProjectile(origin, FacingSign(), arena.IdOf(*this), 95)));
}

void Knight::OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point)
{
    if (slot == MoveSlot::Heavy || slot == MoveSlot::Super) {
        arena.Fx().Burst(point, 18, Color{255, 236, 190, 255}, ParticleKind::Shard, 1.3f);
        arena.Shake(5.0f);
    }
}

// ===========================================================================
// MALTHUS - hắc pháp sư
// ===========================================================================
const MoveDef &Wizard::Move(MoveSlot slot) const
{
    static const MoveDef kLight = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.09f; m.active = 0.08f; m.recovery = 0.20f;
        m.box = Rectangle{44.0f, -158.0f, 116.0f, 58.0f};
        m.damage = 42; m.knockback = 180.0f; m.hitstun = 0.24f; m.blockstun = 0.14f;
        m.meterGain = 8.0f; m.label = "Vụt trượng";
        return m;
    }();

    static const MoveDef kHeavy = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.17f; m.active = 0.10f; m.recovery = 0.30f;
        m.box = Rectangle{44.0f, -180.0f, 142.0f, 96.0f};
        m.damage = 88; m.knockback = 360.0f; m.hitstun = 0.34f; m.blockstun = 0.2f;
        m.meterGain = 11.0f; m.label = "Quét trượng";
        return m;
    }();

    // Chiêu bắn đạn: hitbox cận chiến để rỗng, sát thương nằm ở quả cầu.
    static const MoveDef kSpecial = [] {
        MoveDef m;
        m.anim = AnimId::Attack1;
        m.startup = 0.16f; m.active = 0.06f; m.recovery = 0.30f;
        m.box = Rectangle{0.0f, 0.0f, 0.0f, 0.0f};
        m.damage = 0; m.meterGain = 0.0f; m.cooldown = 0.65f;
        m.label = "Hắc Cầu";
        return m;
    }();

    static const MoveDef kSuper = [] {
        MoveDef m;
        m.anim = AnimId::Attack2;
        m.startup = 0.20f; m.active = 0.08f; m.recovery = 0.40f;
        m.box = Rectangle{0.0f, 0.0f, 0.0f, 0.0f};
        m.damage = 0; m.meterGain = 0.0f; m.meterCost = kMaxMeter; m.cooldown = 1.2f;
        m.label = "Tam Hắc Cầu";
        return m;
    }();

    switch (slot) {
        case MoveSlot::Heavy:   return kHeavy;
        case MoveSlot::Special: return kSpecial;
        case MoveSlot::Super:   return kSuper;
        default:                return kLight;
    }
}

void Wizard::OnSpecialActivate(Arena &arena)
{
    const Vector2 muzzle{pos_.x + FacingSign() * 64.0f, pos_.y - 112.0f};
    arena.SpawnProjectile(std::unique_ptr<Projectile>(
        new OrbProjectile(muzzle, FacingSign(), arena.IdOf(*this), 78, def_.accent, 620.0f)));
    arena.Fx().Burst(muzzle, 14, def_.accent, ParticleKind::Energy, 0.8f);
    AddMeter(9.0f);
}

void Wizard::OnSuperActivate(Arena &arena)
{
    // Ba quả bắn giãn cách, quả sau nhanh hơn quả trước.
    superShots_ = 3;
    superQueue_ = 0.0f;
    arena.Shake(6.0f);
    arena.Fx().Ring(Center(), def_.accent, 30.0f);
}

void Wizard::OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point)
{
    (void)slot;
    arena.Fx().Burst(point, 10, def_.accent, ParticleKind::Energy, 0.9f);
}

void Wizard::UpdateCharacter(Arena &arena, float dt)
{
    orbPhase_ += dt;

    if (superShots_ > 0) {
        superQueue_ -= dt;
        if (superQueue_ <= 0.0f) {
            const int index = 3 - superShots_;
            const Vector2 muzzle{pos_.x + FacingSign() * 64.0f,
                                 pos_.y - 92.0f - index * 26.0f};
            arena.SpawnProjectile(std::unique_ptr<Projectile>(
                new OrbProjectile(muzzle, FacingSign(), arena.IdOf(*this), 68,
                                  def_.accent, 560.0f + index * 130.0f)));
            arena.Fx().Burst(muzzle, 12, def_.accent, ParticleKind::Energy, 0.9f);
            --superShots_;
            superQueue_ = 0.14f;
        }
    }
}

void Wizard::DrawBehind(Vector2 screenPos, float scale) const
{
    (void)screenPos; (void)scale;
    // Hào quang hắc ám luôn vờn quanh - dấu hiệu nhận dạng của nhân vật này.
    const Vector2 c = Center();
    for (int i = 0; i < 3; ++i) {
        const float ang = orbPhase_ * (1.4f + i * 0.5f) + i * 2.1f;
        const float r   = 54.0f + i * 10.0f;
        const Vector2 p{c.x + std::cos(ang) * r, c.y + std::sin(ang) * r * 0.55f};
        Color col = def_.accent;
        col.a = 90;
        DrawCircleV(p, 7.0f - i, col);
        col.a = 40;
        DrawCircleV(p, 13.0f - i, col);
    }
}

// ===========================================================================
// Đạn
// ===========================================================================
OrbProjectile::OrbProjectile(Vector2 pos, float dir, int ownerId, int damage,
                             Color color, float speed)
    : Projectile(pos, dir, ownerId, damage), core_(color)
{
    vel_       = Vector2{dir * speed, 0.0f};
    radius_    = 26.0f;
    life_      = 2.6f;
    color_     = color;
    knockback_ = 330.0f;
    hitstun_   = 0.34f;
}

void OrbProjectile::Update(Arena &arena, float dt)
{
    Projectile::Update(arena, dt);
    spin_ += dt * 9.0f;
    if (alive_) {
        arena.Fx().Trail(Vector2{pos_.x - dir_ * 14.0f, pos_.y}, color_, 6.0f);
    }
}

void OrbProjectile::Draw() const
{
    const float pulse = 1.0f + std::sin(spin_ * 2.4f) * 0.12f;
    Color halo = color_; halo.a = 70;
    DrawCircleV(pos_, radius_ * 1.5f * pulse, halo);
    halo.a = 130;
    DrawCircleV(pos_, radius_ * pulse, halo);
    DrawCircleV(pos_, radius_ * 0.55f * pulse, Color{255, 255, 255, 230});

    // Vành xoay cho quả cầu có cảm giác quay.
    for (int i = 0; i < 2; ++i) {
        const float a = spin_ + i * PI;
        DrawCircleLines((int)(pos_.x + std::cos(a) * 6.0f),
                        (int)(pos_.y + std::sin(a) * 6.0f),
                        radius_ * (0.8f + 0.1f * i), Color{255, 255, 255, 90});
    }
}

void OrbProjectile::OnHit(Arena &arena, Fighter &target)
{
    arena.Fx().Ring(pos_, core_, 26.0f);
    Projectile::OnHit(arena, target);
}

ShockwaveProjectile::ShockwaveProjectile(Vector2 pos, float dir, int ownerId, int damage)
    : Projectile(pos, dir, ownerId, damage)
{
    vel_       = Vector2{dir * 780.0f, 0.0f};
    radius_    = 34.0f;
    life_      = 1.6f;
    color_     = Color{255, 214, 140, 255};
    knockback_ = 420.0f;
    hitstun_   = 0.42f;
}

Rectangle ShockwaveProjectile::Box() const
{
    // Sóng bám mặt đất: cao, hẹp, không chạm được người đang bay trên cao.
    return Rectangle{pos_.x - 34.0f, kGroundY - 96.0f, 68.0f, 96.0f};
}

void ShockwaveProjectile::Update(Arena &arena, float dt)
{
    Projectile::Update(arena, dt);
    phase_ += dt;
    if (alive_) {
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 2, -dir_);
    }
}

void ShockwaveProjectile::Draw() const
{
    const float h = 92.0f + std::sin(phase_ * 22.0f) * 10.0f;
    for (int i = 0; i < 4; ++i) {
        const float off = i * 16.0f * -dir_;
        const float k   = 1.0f - i * 0.22f;
        Color c = color_;
        c.a = (unsigned char)(190 * k);
        Vector2 tip{pos_.x + off, kGroundY - h * k};
        Vector2 l  {pos_.x + off - 22.0f * k, kGroundY};
        Vector2 r  {pos_.x + off + 22.0f * k, kGroundY};
        DrawTriangle(tip, l, r, c);
    }
    DrawEllipse((int)pos_.x, (int)kGroundY, 40.0f, 10.0f, Color{255, 236, 190, 150});
}

SlashProjectile::SlashProjectile(Vector2 pos, float dir, int ownerId, int damage, Color color)
    : Projectile(pos, dir, ownerId, damage)
{
    vel_       = Vector2{dir * 900.0f, 0.0f};
    radius_    = 24.0f;
    life_      = 1.4f;
    color_     = color;
    knockback_ = 300.0f;
    hitstun_   = 0.30f;
}

void SlashProjectile::Update(Arena &arena, float dt)
{
    Projectile::Update(arena, dt);
    spin_ += dt * 18.0f;
    if (alive_) arena.Fx().Trail(pos_, color_, 5.0f);
}

void SlashProjectile::Draw() const
{
    Rectangle r{pos_.x, pos_.y, 74.0f, 16.0f};
    Color glow = color_; glow.a = 90;
    DrawRectanglePro(Rectangle{r.x, r.y, r.width * 1.3f, r.height * 2.2f},
                     Vector2{r.width * 0.65f, r.height * 1.1f},
                     std::sin(spin_) * 6.0f, glow);
    DrawRectanglePro(r, Vector2{r.width * 0.5f, r.height * 0.5f},
                     std::sin(spin_) * 6.0f, color_);
    DrawRectanglePro(Rectangle{r.x, r.y, r.width * 0.6f, r.height * 0.35f},
                     Vector2{r.width * 0.3f, r.height * 0.175f},
                     std::sin(spin_) * 6.0f, Color{255, 255, 255, 230});
}

} // namespace fighter
