#include "entities/Projectile.hpp"
#include "entities/Arena.hpp"
#include "entities/Fighter.hpp"
#include "core/Config.hpp"

namespace fighter {

Projectile::Projectile(Vector2 pos, float dir, int ownerId, int damage)
    : pos_(pos), dir_(dir), damage_(damage), ownerId_(ownerId)
{
    chip_ = damage / 6;
}

void Projectile::Update(Arena &arena, float dt)
{
    age_ += dt;
    hitCooldown_ -= dt;
    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;

    if (age_ >= life_) alive_ = false;
    if (pos_.x < arena.CameraX() - 160.0f || pos_.x > arena.CameraX() + arena.ViewWidth() + 160.0f)
        alive_ = false;
}

Rectangle Projectile::Box() const
{
    return Rectangle{pos_.x - radius_, pos_.y - radius_, radius_ * 2.0f, radius_ * 2.0f};
}

void Projectile::OnHit(Arena &arena, Fighter &target, bool blocked)
{
    if (hitCooldown_ > 0.0f) return;

    MoveDef hit{};
    hit.id        = MoveId::SpecialA;
    hit.damage    = damage_;
    hit.chip      = chip_;
    hit.knockback = knockback_;
    hit.hitstun   = hitstun_;
    hit.blockstun = blockstun_;
    hit.height    = height_;
    hit.knockdown = knockdown_;
    hit.launcher  = launcher_;
    hit.meterGain = 6.0f;
    if (fromSuper_) { hit.id = MoveId::Super; hit.meterCost = 1.0f; hit.meterGain = 0.0f; }

    Fighter *owner = arena.ById(ownerId_);
    if (!owner) { alive_ = false; return; }

    const int dmg = target.ReceiveHit(arena, *owner, hit, blocked, false);
    if (!blocked) {
        owner->AddMeter(hit.meterGain);
        arena.CountProjectileHit(*owner, dmg);
    }
    arena.Emit({blocked ? EventType::Block : EventType::Hit, ownerId_, pos_, weight_, dmg});
    arena.HitSpark(pos_, blocked, weight_, color_);

    --hitsLeft_;
    hitCooldown_ = 0.08f;
    if (hitsLeft_ <= 0) alive_ = false;
}

void Projectile::OnClash(Arena &arena)
{
    arena.HitSpark(pos_, false, SfxWeight::Medium, color_);
    --hitsLeft_;
    if (hitsLeft_ <= 0) alive_ = false;
}

} // namespace fighter
