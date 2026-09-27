#pragma once
#include "raylib.h"
#include "core/Input.hpp"
#include "entities/Fighter.hpp"
#include "entities/Particles.hpp"
#include "entities/Projectile.hpp"
#include <memory>
#include <vector>

namespace fighter {

// ============================================================================
// Arena - "thế giới" của một trận đấu: hai võ sĩ, đạn, hiệu ứng, rung màn hình.
//
// Arena KHÔNG quản lý luật trận (số round, đồng hồ, màn FIGHT!) - đó là việc
// của BattleScene. Tách ra như vậy để sau này thêm chế độ luyện tập hay 3 người
// chỉ cần viết scene mới, không đụng vào vật lý.
// ============================================================================
class Arena {
public:
    void SetFighters(std::unique_ptr<Fighter> p1, std::unique_ptr<Fighter> p2);
    void ResetPositions();

    void Update(const InputState &in1, const InputState &in2, float dt);
    void DrawWorld(bool debugBoxes) const;

    Fighter &P1() { return *p1_; }
    Fighter &P2() { return *p2_; }
    const Fighter &P1() const { return *p1_; }
    const Fighter &P2() const { return *p2_; }
    bool Ready() const { return p1_ && p2_; }

    Fighter &Opponent(const Fighter &who);
    int      IdOf(const Fighter &who) const { return (&who == p1_.get()) ? 0 : 1; }
    Fighter *ById(int id) { return id == 0 ? p1_.get() : p2_.get(); }

    void SpawnProjectile(std::unique_ptr<Projectile> p);
    ParticleSystem &Fx() { return fx_; }

    // Hiệu ứng "đánh đã tay": khựng hình vài chục ms + rung camera.
    void HitStop(float seconds);
    void Shake(float strength);

    Vector2 ShakeOffset() const { return shakeOffset_; }
    bool    Frozen() const { return hitStop_ > 0.0f; }

    // Khoá điều khiển khi đang chiếu chữ "FIGHT!" hoặc "K.O.".
    void SetInputEnabled(bool on) { inputEnabled_ = on; }

private:
    void ResolveBodies();
    void ResolveAttack(Fighter &attacker, Fighter &defender);
    void ResolveProjectiles();
    void FaceEachOther();

    std::unique_ptr<Fighter> p1_, p2_;
    std::vector<std::unique_ptr<Projectile>> projectiles_;
    ParticleSystem fx_;

    float   hitStop_ = 0.0f;
    float   shake_   = 0.0f;
    Vector2 shakeOffset_{};
    bool    inputEnabled_ = true;
};

} // namespace fighter
