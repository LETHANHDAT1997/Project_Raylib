#include "characters/Characters.hpp"
#include "entities/Arena.hpp"
#include <cmath>

namespace fighter {

namespace {
Color WithAlpha(Color c, float a) { c.a = (unsigned char)(255.0f * std::fmax(0.0f, std::fmin(1.0f, a))); return c; }
}

// ===========================================================================
// Khí kiếm lưỡi liềm
// ===========================================================================
WaveProjectile::WaveProjectile(Vector2 pos, float dir, int owner, int damage, float speed,
                               Color color, float scale)
    : Projectile(pos, dir, owner, damage), scale_(scale)
{
    vel_ = Vector2{dir * speed, 0.0f};
    radius_ = 34.0f * scale;
    life_ = 2.2f;
    color_ = color;
    knockback_ = 300.0f;
    hitstun_ = 0.40f;
    knockdown_ = scale > 1.2f;
    weight_ = scale > 1.2f ? SfxWeight::Heavy : SfxWeight::Medium;
}

void WaveProjectile::Update(Arena &arena, float dt)
{
    Projectile::Update(arena, dt);
    if (alive_ && GetRandomValue(0, 1) == 0) {
        arena.Fx().Trail(Vector2{pos_.x - dir_ * 20.0f, pos_.y + (float)GetRandomValue(-30, 30) * scale_},
                         color_, 5.0f * scale_);
    }
}

void WaveProjectile::Draw() const
{
    const float r = 58.0f * scale_;
    const float pulse = 1.0f + 0.06f * std::sin(age_ * 40.0f);
    const Vector2 c{pos_.x - dir_ * r * 0.55f, pos_.y};
    const float mid = (dir_ > 0.0f) ? 0.0f : 180.0f;
    const float half = 58.0f;

    BeginBlendMode(BLEND_ADDITIVE);
    DrawRing(c, r * 0.55f, r * 1.12f * pulse, mid - half, mid + half, 32, WithAlpha(color_, 0.28f));
    DrawRing(c, r * 0.78f, r * pulse,         mid - half, mid + half, 32, WithAlpha(color_, 0.75f));
    DrawRing(c, r * 0.9f,  r * 0.98f,          mid - half * 0.8f, mid + half * 0.8f, 32,
             Color{255, 255, 255, 230});
    EndBlendMode();
}

// ===========================================================================
// Phi tiêu
// ===========================================================================
ShurikenProjectile::ShurikenProjectile(Vector2 pos, Vector2 vel, int owner, int damage, Color color)
    : Projectile(pos, vel.x > 0 ? 1.0f : -1.0f, owner, damage)
{
    vel_ = vel;
    radius_ = 16.0f;
    life_ = 1.4f;
    color_ = color;
    knockback_ = 150.0f;
    hitstun_ = 0.26f;
    blockstun_ = 0.14f;
    weight_ = SfxWeight::Light;
}

void ShurikenProjectile::Update(Arena &arena, float dt)
{
    Projectile::Update(arena, dt);
    spin_ += dt * 26.0f;
    if (pos_.y >= kGroundY - 6.0f) {   // cắm xuống đất
        arena.Fx().Burst(pos_, 6, color_, ParticleKind::Spark, 0.5f);
        alive_ = false;
    }
}

void ShurikenProjectile::Draw() const
{
    const float r = 20.0f;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV(pos_, r * 1.2f, WithAlpha(color_, 0.25f));
    EndBlendMode();
    for (int i = 0; i < 4; ++i) {
        const float a = spin_ + i * PI * 0.5f;
        const Vector2 tip{pos_.x + std::cos(a) * r, pos_.y + std::sin(a) * r};
        const Vector2 l{pos_.x + std::cos(a + 0.9f) * r * 0.3f, pos_.y + std::sin(a + 0.9f) * r * 0.3f};
        const Vector2 rr{pos_.x + std::cos(a - 0.9f) * r * 0.3f, pos_.y + std::sin(a - 0.9f) * r * 0.3f};
        DrawTriangle(tip, rr, l, Color{210, 216, 230, 255});
        DrawTriangle(tip, l, rr, Color{210, 216, 230, 255});
    }
    DrawCircleV(pos_, 4.0f, Color{60, 60, 70, 255});
}

// ===========================================================================
// Cầu hắc ám
// ===========================================================================
OrbProjectile::OrbProjectile(Vector2 pos, float dir, int owner, int damage, float speed, Color color)
    : Projectile(pos, dir, owner, damage)
{
    vel_ = Vector2{dir * speed, 0.0f};
    radius_ = 26.0f;
    life_ = 3.0f;
    color_ = color;
    knockback_ = 320.0f;
    hitstun_ = 0.38f;
}

void OrbProjectile::Update(Arena &arena, float dt)
{
    Projectile::Update(arena, dt);
    spin_ += dt * 9.0f;
    if (alive_) arena.Fx().Trail(Vector2{pos_.x - dir_ * 16.0f, pos_.y}, color_, 7.0f);
}

void OrbProjectile::Draw() const
{
    const float pulse = 1.0f + std::sin(spin_ * 2.4f) * 0.12f;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV(pos_, radius_ * 1.7f * pulse, WithAlpha(color_, 0.22f));
    DrawCircleV(pos_, radius_ * pulse, WithAlpha(color_, 0.6f));
    for (int i = 0; i < 3; ++i) {
        const float a = spin_ + i * 2.09f;
        DrawRing(pos_, radius_ * 0.9f, radius_ * 1.05f, a * RAD2DEG, a * RAD2DEG + 70.0f, 12,
                 Color{255, 220, 255, 170});
    }
    EndBlendMode();
    DrawCircleV(pos_, radius_ * 0.45f * pulse, Color{30, 10, 40, 255});
    DrawCircleLines((int)pos_.x, (int)pos_.y, radius_ * 0.45f * pulse, Color{255, 230, 255, 220});
}

// ===========================================================================
// Sóng chấn động
// ===========================================================================
ShockwaveProjectile::ShockwaveProjectile(Vector2 pos, float dir, int owner, int damage,
                                         float speed, float life, float size)
    : Projectile(pos, dir, owner, damage), size_(size)
{
    vel_ = Vector2{dir * speed, 0.0f};
    life_ = life;
    color_ = Color{255, 214, 140, 255};
    knockback_ = 380.0f;
    hitstun_ = 0.45f;
    knockdown_ = size > 1.2f;
    weight_ = SfxWeight::Heavy;
}

Rectangle ShockwaveProjectile::Box() const
{
    // Sóng bám mặt đất: nhảy lên là né được.
    const float h = 90.0f * size_;
    return Rectangle{pos_.x - 34.0f * size_, kGroundY - h, 68.0f * size_, h};
}

void ShockwaveProjectile::Update(Arena &arena, float dt)
{
    Projectile::Update(arena, dt);
    phase_ += dt;
    if (alive_) arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 2, -dir_);
}

void ShockwaveProjectile::Draw() const
{
    const float fade = std::fmin(1.0f, (life_ - age_) / 0.2f);
    for (int i = 0; i < 5; ++i) {
        const float off = i * 18.0f * -dir_ * size_;
        const float k   = (1.0f - i * 0.18f);
        const float h   = (80.0f + std::sin(phase_ * 26.0f + i) * 12.0f) * size_ * k;
        const Vector2 tip{pos_.x + off, kGroundY - h};
        const Vector2 l{pos_.x + off - 20.0f * size_ * k, kGroundY};
        const Vector2 r{pos_.x + off + 20.0f * size_ * k, kGroundY};
        DrawTriangle(tip, l, r, WithAlpha(Color{120, 96, 70, 255}, fade * k));
        DrawTriangle(Vector2{tip.x, tip.y + 10}, Vector2{l.x + 8, l.y}, Vector2{r.x - 8, r.y},
                     WithAlpha(Color{176, 146, 110, 255}, fade * k));
    }
    BeginBlendMode(BLEND_ADDITIVE);
    DrawEllipse((int)pos_.x, (int)kGroundY, 60.0f * size_, 12.0f * size_, WithAlpha(color_, 0.45f * fade));
    EndBlendMode();
}

// ===========================================================================
// Cột hắc ám
// ===========================================================================
PillarHazard::PillarHazard(float x, int owner, int damage, Color color, float delay)
    : Projectile(Vector2{x, kGroundY}, 1.0f, owner, damage), delay_(delay)
{
    life_ = delay + 0.42f;
    color_ = color;
    launcher_ = true;
    knockback_ = 120.0f;
    hitstun_ = 0.5f;
    weight_ = SfxWeight::Heavy;
}

bool PillarHazard::Harmful() const
{
    return alive_ && age_ >= delay_ && age_ < delay_ + 0.3f;
}

Rectangle PillarHazard::Box() const
{
    return Rectangle{pos_.x - 42.0f, kGroundY - 270.0f, 84.0f, 270.0f};
}

void PillarHazard::Update(Arena &arena, float dt)
{
    age_ += dt;
    hitCooldown_ -= dt;
    if (!erupted_ && age_ >= delay_) {
        erupted_ = true;
        arena.Shake(6.0f);
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 20, 0.0f);
        arena.Fx().Burst(Vector2{pos_.x, kGroundY - 120.0f}, 20, color_, ParticleKind::Energy, 1.2f);
    }
    if (age_ >= life_) alive_ = false;
}

void PillarHazard::Draw() const
{
    if (age_ < delay_) {
        // Báo trước: vòng tròn đen lan trên mặt đất.
        const float k = age_ / delay_;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawEllipse((int)pos_.x, (int)kGroundY, 50.0f * k, 12.0f * k, WithAlpha(color_, 0.5f + 0.4f * std::sin(age_ * 50.0f)));
        EndBlendMode();
        return;
    }
    const float t = age_ - delay_;
    const float rise = std::fmin(1.0f, t / 0.08f);
    const float fade = std::fmin(1.0f, (life_ - age_) / 0.15f);
    const float h = 270.0f * rise;
    const float w = 70.0f * (1.0f + 0.1f * std::sin(t * 60.0f));

    DrawRectangleRec(Rectangle{pos_.x - w * 0.5f, kGroundY - h, w, h}, WithAlpha(Color{30, 12, 44, 255}, 0.9f * fade));
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleGradientV((int)(pos_.x - w * 0.5f), (int)(kGroundY - h), (int)w, (int)h,
                           WithAlpha(color_, 0.1f * fade), WithAlpha(color_, 0.8f * fade));
    DrawRectangle((int)(pos_.x - 6), (int)(kGroundY - h), 12, (int)h, WithAlpha(Color{255, 220, 255, 255}, 0.7f * fade));
    DrawEllipse((int)pos_.x, (int)(kGroundY - h), w * 0.6f, 14.0f, WithAlpha(color_, 0.8f * fade));
    EndBlendMode();
}

// ===========================================================================
// Tia sét
// ===========================================================================
LightningHazard::LightningHazard(float x, int owner, int damage, Color color, float delay)
    : Projectile(Vector2{x, kGroundY}, 1.0f, owner, damage), delay_(delay)
{
    life_ = delay + 0.55f;
    color_ = color;
    launcher_ = true;
    knockback_ = 100.0f;
    hitstun_ = 0.55f;
    chip_ = damage / 4;
    weight_ = SfxWeight::Heavy;
}

bool LightningHazard::Harmful() const
{
    return alive_ && age_ >= delay_ + 0.25f && age_ < delay_ + 0.45f;
}

Rectangle LightningHazard::Box() const
{
    return Rectangle{pos_.x - 48.0f, kGroundY - 640.0f, 96.0f, 640.0f};
}

void LightningHazard::Update(Arena &arena, float dt)
{
    age_ += dt;
    hitCooldown_ -= dt;
    if (!struck_ && age_ >= delay_ + 0.25f) {
        struck_ = true;
        arena.Shake(9.0f);
        arena.Emit({EventType::Thunder, ownerId_, Vector2{pos_.x, kGroundY - 100.0f}, SfxWeight::Heavy});
        for (int i = 0; i < 3; ++i) {
            arena.Fx().Bolt(Vector2{pos_.x + (float)GetRandomValue(-40, 40), kGroundY - 700.0f},
                            Vector2{pos_.x, kGroundY}, color_);
        }
        arena.Fx().Spark(Vector2{pos_.x, kGroundY - 20.0f}, Color{230, 200, 255, 255}, 1.8f);
        arena.Fx().Burst(Vector2{pos_.x, kGroundY - 10.0f}, 24, color_, ParticleKind::Spark, 1.2f);
    }
    if (age_ >= life_) alive_ = false;
}

void LightningHazard::Draw() const
{
    if (age_ < delay_) return;
    const float t = age_ - delay_;
    BeginBlendMode(BLEND_ADDITIVE);
    if (t < 0.25f) {
        // Báo trước: cột sáng mờ + vòng trên đất.
        const float k = t / 0.25f;
        DrawRectangleGradientV((int)pos_.x - 30, 0, 60, (int)kGroundY, WithAlpha(color_, 0.0f), WithAlpha(color_, 0.25f * k));
        DrawEllipse((int)pos_.x, (int)kGroundY, 50.0f, 12.0f, WithAlpha(color_, 0.6f * k));
    } else {
        const float k = 1.0f - std::fmin(1.0f, (t - 0.25f) / 0.3f);
        DrawRectangleGradientV((int)pos_.x - 50, (int)(kGroundY - 640), 100, 640,
                               WithAlpha(color_, 0.1f * k), WithAlpha(color_, 0.7f * k));
        DrawRectangle((int)pos_.x - 8, (int)(kGroundY - 640), 16, 640, WithAlpha(Color{255, 250, 255, 255}, k));
    }
    EndBlendMode();
}

} // namespace fighter
