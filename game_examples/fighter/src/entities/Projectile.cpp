#include "entities/Projectile.hpp"
#include "entities/Arena.hpp"
#include "entities/Fighter.hpp"
#include "core/Config.hpp"

namespace fighter {

Projectile::Projectile(Vector2 pos, float dir, int ownerId, int damage)
    : pos_(pos), dir_(dir), damage_(damage), ownerId_(ownerId)
{
}

void Projectile::Update(Arena &arena, float dt)
{
    (void)arena;
    age_ += dt;
    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;

    if (age_ >= life_) alive_ = false;
    if (pos_.x < -80.0f || pos_.x > kCanvasWidth + 80.0f) alive_ = false;
}

Rectangle Projectile::Box() const
{
    return Rectangle{pos_.x - radius_, pos_.y - radius_, radius_ * 2.0f, radius_ * 2.0f};
}

void Projectile::OnHit(Arena &arena, Fighter &target)
{
    MoveDef hit{};
    hit.damage    = damage_;
    hit.knockback = knockback_;
    hit.hitstun   = hitstun_;
    hit.blockstun = 0.18f;

    Fighter *owner = arena.ById(ownerId_);
    const bool blocked = target.IsBlocking();
    if (owner) target.ReceiveHit(arena, *owner, hit, blocked);

    arena.Fx().Burst(pos_, 22, color_, ParticleKind::Energy, 1.2f);
    arena.Fx().Ring(pos_, color_, 20.0f);
    arena.Shake(6.0f);
    alive_ = false;
}

} // namespace fighter
