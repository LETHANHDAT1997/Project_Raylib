#include "entities/Particles.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

namespace {
constexpr int kPoolSize = 900;
float Rand(float a, float b)
{
    return a + (b - a) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}
bool Additive(ParticleKind k)
{
    return k != ParticleKind::Dust && k != ParticleKind::Smoke;
}
} // namespace

void ParticleSystem::Clear() { pool_.clear(); }

Particle *ParticleSystem::Spawn()
{
    for (auto &p : pool_) if (p.life <= 0.0f) return &p;
    if ((int)pool_.size() >= kPoolSize) return nullptr;
    pool_.emplace_back();
    return &pool_.back();
}

void ParticleSystem::Update(float dt)
{
    for (auto &p : pool_) {
        if (p.life <= 0.0f) continue;
        p.life -= dt;
        if (p.kind == ParticleKind::Bolt) continue;   // vel là điểm cuối, không phải vận tốc
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.angle += p.spin * dt;
        if (p.gravity) p.vel.y += 1500.0f * dt;
        p.vel.x *= std::exp(-2.0f * dt);
        if (p.kind == ParticleKind::Ring)  p.size += 300.0f * dt;
        if (p.kind == ParticleKind::Smoke) { p.size += 30.0f * dt; p.vel.y -= 40.0f * dt; }
    }
}

void ParticleSystem::DrawOne(const Particle &p) const
{
    const float t = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
    Color c = p.color;
    c.a = (unsigned char)(c.a * t);

    switch (p.kind) {
        case ParticleKind::Ring:
            DrawRing(p.pos, p.size * 0.86f, p.size, 0, 360, 40, c);
            break;

        case ParticleKind::Spark: {
            const Vector2 tail{p.pos.x - p.vel.x * 0.025f, p.pos.y - p.vel.y * 0.025f};
            DrawLineEx(tail, p.pos, std::max(1.5f, p.size * t), c);
            break;
        }

        case ParticleKind::Shard: {
            Rectangle r{p.pos.x, p.pos.y, p.size * t * 2.4f, p.size * t * 0.7f};
            DrawRectanglePro(r, Vector2{r.width * 0.5f, r.height * 0.5f}, p.angle * RAD2DEG, c);
            break;
        }

        case ParticleKind::Energy:
            DrawCircleV(p.pos, p.size * t * 1.8f, Color{c.r, c.g, c.b, (unsigned char)(c.a * 0.35f)});
            DrawCircleV(p.pos, p.size * t, c);
            break;

        case ParticleKind::Flash: {
            // Ngôi sao 4-8 cánh co lại rất nhanh: tia sáng va chạm kiểu arcade.
            const float grow = (t > 0.7f) ? (1.0f - t) / 0.3f : 1.0f;
            const float r = p.size * (0.4f + 0.6f * grow) * (0.5f + 0.5f * t);
            const int rays = 8;
            for (int i = 0; i < rays; ++i) {
                const float a = p.angle + i * (2.0f * PI / rays);
                const float len = r * ((i % 2) ? 0.55f : 1.0f);
                const Vector2 tip{p.pos.x + std::cos(a) * len, p.pos.y + std::sin(a) * len};
                const Vector2 l{p.pos.x + std::cos(a + 0.22f) * r * 0.16f, p.pos.y + std::sin(a + 0.22f) * r * 0.16f};
                const Vector2 rr{p.pos.x + std::cos(a - 0.22f) * r * 0.16f, p.pos.y + std::sin(a - 0.22f) * r * 0.16f};
                DrawTriangle(tip, l, rr, c);
                DrawTriangle(tip, rr, l, c);
            }
            DrawCircleV(p.pos, r * 0.28f, Color{255, 255, 255, (unsigned char)(255 * t)});
            break;
        }

        case ParticleKind::Smoke:
            DrawCircleV(p.pos, p.size, c);
            break;

        case ParticleKind::Bolt: {
            // vel dùng làm điểm cuối, angle làm hạt giống ngẫu nhiên
            const Vector2 a = p.pos, b = p.vel;
            const int segs = 7;
            Vector2 prev = a;
            unsigned int seed = (unsigned int)(p.angle * 1000.0f);
            for (int i = 1; i <= segs; ++i) {
                const float k = (float)i / segs;
                seed = seed * 1103515245u + 12345u;
                const float jitter = (i == segs) ? 0.0f : ((float)((seed >> 16) % 100) / 100.0f - 0.5f) * 46.0f;
                const Vector2 cur{a.x + (b.x - a.x) * k + jitter, a.y + (b.y - a.y) * k};
                DrawLineEx(prev, cur, 9.0f * t, Color{c.r, c.g, c.b, (unsigned char)(c.a * 0.5f)});
                DrawLineEx(prev, cur, 3.0f * t, Color{255, 255, 255, c.a});
                prev = cur;
            }
            break;
        }

        case ParticleKind::Dust:
        default:
            DrawCircleV(p.pos, p.size * t, c);
            break;
    }
}

void ParticleSystem::Draw() const
{
    for (const auto &p : pool_) if (p.life > 0.0f && !Additive(p.kind)) DrawOne(p);
    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto &p : pool_) if (p.life > 0.0f && Additive(p.kind)) DrawOne(p);
    EndBlendMode();
}

void ParticleSystem::Burst(Vector2 at, int count, Color color, ParticleKind kind, float power)
{
    for (int i = 0; i < count; ++i) {
        Particle *p = Spawn();
        if (!p) return;
        const float ang = Rand(0.0f, 2.0f * PI);
        const float spd = Rand(140.0f, 560.0f) * power;
        p->pos     = at;
        p->vel     = Vector2{std::cos(ang) * spd, std::sin(ang) * spd - 80.0f};
        p->maxLife = Rand(0.2f, 0.45f);
        p->life    = p->maxLife;
        p->size    = Rand(2.0f, 5.0f) * power;
        p->color   = color;
        p->kind    = kind;
        p->spin    = Rand(-14.0f, 14.0f);
        p->angle   = ang;
        p->gravity = (kind != ParticleKind::Energy);
    }
}

void ParticleSystem::Spark(Vector2 at, Color color, float power)
{
    Particle *p = Spawn();
    if (!p) return;
    p->pos = at;
    p->vel = Vector2{0.0f, 0.0f};
    p->maxLife = p->life = 0.16f + 0.04f * power;
    p->size = 60.0f * power;
    p->color = color;
    p->kind = ParticleKind::Flash;
    p->angle = Rand(0.0f, PI);
    p->gravity = false;
}

void ParticleSystem::Dust(Vector2 at, int count, float dir)
{
    for (int i = 0; i < count; ++i) {
        Particle *p = Spawn();
        if (!p) return;
        p->pos     = Vector2{at.x + Rand(-14.0f, 14.0f), at.y + Rand(-4.0f, 2.0f)};
        p->vel     = Vector2{dir * Rand(20.0f, 140.0f) + Rand(-40.0f, 40.0f), Rand(-150.0f, -40.0f)};
        p->maxLife = Rand(0.25f, 0.55f);
        p->life    = p->maxLife;
        p->size    = Rand(3.0f, 8.0f);
        p->color   = Color{190, 176, 156, 170};
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
    p->maxLife = p->life = 0.30f;
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
    p->maxLife = p->life = 0.28f;
    p->size    = size;
    p->color   = color;
    p->kind    = ParticleKind::Energy;
    p->gravity = false;
}

void ParticleSystem::Smoke(Vector2 at, int count, Color color)
{
    for (int i = 0; i < count; ++i) {
        Particle *p = Spawn();
        if (!p) return;
        p->pos     = Vector2{at.x + Rand(-30.0f, 30.0f), at.y + Rand(-60.0f, 30.0f)};
        p->vel     = Vector2{Rand(-60.0f, 60.0f), Rand(-80.0f, 0.0f)};
        p->maxLife = p->life = Rand(0.35f, 0.7f);
        p->size    = Rand(14.0f, 30.0f);
        p->color   = color;
        p->kind    = ParticleKind::Smoke;
        p->gravity = false;
    }
}

void ParticleSystem::Bolt(Vector2 from, Vector2 to, Color color)
{
    Particle *p = Spawn();
    if (!p) return;
    p->pos = from;
    p->vel = to;           // điểm cuối
    p->maxLife = p->life = 0.22f;
    p->color = color;
    p->kind = ParticleKind::Bolt;
    p->angle = Rand(0.0f, 1000.0f);
    p->gravity = false;
}

} // namespace fighter
