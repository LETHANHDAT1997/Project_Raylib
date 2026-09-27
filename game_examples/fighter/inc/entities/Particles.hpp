#pragma once
#include "raylib.h"
#include <vector>

namespace fighter {

enum class ParticleKind { Spark, Dust, Energy, Shard, Ring };

struct Particle {
    Vector2 pos{}, vel{};
    float life = 0.0f, maxLife = 1.0f;
    float size = 4.0f, spin = 0.0f, angle = 0.0f;
    Color color = WHITE;
    ParticleKind kind = ParticleKind::Spark;
    bool gravity = true;
};

// ============================================================================
// ParticleSystem - hiệu ứng va chạm, bụi chân, năng lượng. Tách riêng khỏi
// Fighter để mọi thực thể (đòn đánh, đạn, HUD) đều bắn được hiệu ứng.
// ============================================================================
class ParticleSystem {
public:
    void Clear();
    void Update(float dt);
    void Draw() const;

    void Burst(Vector2 at, int count, Color color, ParticleKind kind, float power = 1.0f);
    void Dust(Vector2 at, int count, float dir);
    void Ring(Vector2 at, Color color, float radius);
    void Trail(Vector2 at, Color color, float size);

private:
    Particle *Spawn();
    std::vector<Particle> pool_;
};

} // namespace fighter
