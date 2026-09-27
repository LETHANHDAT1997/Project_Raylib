#include "entities/Particles.hpp"
#include "core/Config.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

namespace {
constexpr int kPoolSize = 512;
float Rand(float a, float b)
{
    return a + (b - a) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}
} // namespace

void ParticleSystem::Clear()
{
    pool_.clear();
}

Particle *ParticleSystem::Spawn()
{
    for (auto &p : pool_) {
        if (p.life <= 0.0f) return &p;
    }
    if ((int)pool_.size() >= kPoolSize) return nullptr;
    pool_.emplace_back();
    return &pool_.back();
}

void ParticleSystem::Update(float dt)
{
    for (auto &p : pool_) {
        if (p.life <= 0.0f) continue;
        p.life -= dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.angle += p.spin * dt;
        if (p.gravity) p.vel.y += 1500.0f * dt;
        p.vel.x *= 0.985f;
        if (p.kind == ParticleKind::Ring) p.size += 260.0f * dt;
    }
}

void ParticleSystem::Draw() const
{
    for (const auto &p : pool_) {
        if (p.life <= 0.0f) continue;
        const float t = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        Color c = p.color;
        c.a = (unsigned char)(c.a * t);

        switch (p.kind) {
            case ParticleKind::Ring:
                DrawCircleLines((int)p.pos.x, (int)p.pos.y, p.size, c);
                DrawCircleLines((int)p.pos.x, (int)p.pos.y, p.size * 0.82f,
                                Color{c.r, c.g, c.b, (unsigned char)(c.a / 2)});
                break;

            case ParticleKind::Spark: {
                const Vector2 tail{p.pos.x - p.vel.x * 0.02f, p.pos.y - p.vel.y * 0.02f};
                DrawLineEx(tail, p.pos, std::max(1.0f, p.size * t), c);
                break;
            }

            case ParticleKind::Shard: {
                Rectangle r{p.pos.x, p.pos.y, p.size * t * 2.2f, p.size * t * 0.7f};
                DrawRectanglePro(r, Vector2{r.width * 0.5f, r.height * 0.5f},
                                 p.angle * RAD2DEG, c);
                break;
            }

            case ParticleKind::Energy:
                DrawCircleV(p.pos, p.size * t * 1.6f,
                            Color{c.r, c.g, c.b, (unsigned char)(c.a * 0.4f)});
                DrawCircleV(p.pos, p.size * t, c);
                break;

            case ParticleKind::Dust:
            default:
                DrawCircleV(p.pos, p.size * t, c);
                break;
        }
    }
}

void ParticleSystem::Burst(Vector2 at, int count, Color color, ParticleKind kind, float power)
{
    for (int i = 0; i < count; ++i) {
        Particle *p = Spawn();
        if (!p) return;
        const float ang = Rand(0.0f, 2.0f * PI);
        const float spd = Rand(120.0f, 520.0f) * power;
        p->pos     = at;
        p->vel     = Vector2{std::cos(ang) * spd, std::sin(ang) * spd - 90.0f};
        p->maxLife = Rand(0.22f, 0.5f);
        p->life    = p->maxLife;
        p->size    = Rand(2.0f, 5.0f) * power;
        p->color   = color;
        p->kind    = kind;
        p->spin    = Rand(-14.0f, 14.0f);
        p->angle   = ang;
        p->gravity = (kind != ParticleKind::Energy);
    }
}

void ParticleSystem::Dust(Vector2 at, int count, float dir)
{
    for (int i = 0; i < count; ++i) {
        Particle *p = Spawn();
        if (!p) return;
        p->pos     = Vector2{at.x + Rand(-12.0f, 12.0f), at.y + Rand(-4.0f, 2.0f)};
        p->vel     = Vector2{dir * Rand(20.0f, 120.0f), Rand(-140.0f, -40.0f)};
        p->maxLife = Rand(0.25f, 0.5f);
        p->life    = p->maxLife;
        p->size    = Rand(3.0f, 7.0f);
        p->color   = Color{186, 172, 150, 170};
        p->kind    = ParticleKind::Dust;
        p->gravity = true;
    }
}

void ParticleSystem::Ring(Vector2 at, Color color, float radius)
{
    Particle *p = Spawn();
    if (!p) return;
    p->pos     = at;
    p->vel     = Vector2{0.0f, 0.0f};
    p->maxLife = 0.30f;
    p->life    = p->maxLife;
    p->size    = radius;
    p->color   = color;
    p->kind    = ParticleKind::Ring;
    p->gravity = false;
}

void ParticleSystem::Trail(Vector2 at, Color color, float size)
{
    Particle *p = Spawn();
    if (!p) return;
    p->pos     = at;
    p->vel     = Vector2{Rand(-30.0f, 30.0f), Rand(-50.0f, 10.0f)};
    p->maxLife = 0.28f;
    p->life    = p->maxLife;
    p->size    = size;
    p->color   = color;
    p->kind    = ParticleKind::Energy;
    p->gravity = false;
}

} // namespace fighter
