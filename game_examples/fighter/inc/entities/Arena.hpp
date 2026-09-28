#pragma once
#include "raylib.h"
#include "core/Input.hpp"
#include "entities/Events.hpp"
#include "entities/Fighter.hpp"
#include "entities/Particles.hpp"
#include "entities/Projectile.hpp"
#include <memory>
#include <vector>

namespace fighter {

// ============================================================================
// Arena - "thế giới" của một trận: hai võ sĩ, đạn, hiệu ứng, camera, tường.
//
// Không quản lý luật trận (hiệp, đồng hồ) - đó là việc của BattleScene. Mọi
// điều đáng chú ý (trúng đòn, đỡ, vật, K.O.) được đẩy vào hàng đợi sự kiện
// để scene phát âm thanh và hiện chữ.
// ============================================================================
class Arena {
public:
    void SetFighters(std::unique_ptr<Fighter> p1, std::unique_ptr<Fighter> p2);
    void ResetPositions();

    void Update(const InputState &in1, const InputState &in2, float dt);
    void DrawWorld(bool debugBoxes) const;   // gọi bên trong BeginMode2D(Camera())

    Fighter &P1() { return *p1_; }
    Fighter &P2() { return *p2_; }
    const Fighter &P1() const { return *p1_; }
    const Fighter &P2() const { return *p2_; }
    bool Ready() const { return p1_ && p2_; }

    Fighter &Opponent(const Fighter &who);
    int      IdOf(const Fighter &who) const { return (&who == p1_.get()) ? 0 : 1; }
    Fighter *ById(int id) { return id == 0 ? p1_.get() : p2_.get(); }

    // --- đạn --------------------------------------------------------------
    void SpawnProjectile(std::unique_ptr<Projectile> p);
    int  CountProjectiles(int ownerId) const;
    bool ProjectileThreatens(const Fighter &who) const;
    void CountProjectileHit(Fighter &owner, int damage);

    // --- hiệu ứng ---------------------------------------------------------
    ParticleSystem &Fx() { return fx_; }
    void HitSpark(Vector2 at, bool blocked, SfxWeight w, Color accent);
    void HitStop(float seconds);
    void Shake(float strength);
    void StartSuperFreeze(Fighter &who);

    Vector2 ShakeOffset()   const { return shakeOffset_; }
    bool    Frozen()        const { return hitStop_ > 0.0f || superFreeze_ > 0.0f; }
    float   SuperFreeze()   const { return superFreeze_; }
    int     SuperOwner()    const { return superOwner_; }

    // --- camera -----------------------------------------------------------
    float    CameraX() const { return camX_; }                 // mép trái khung nhìn (toạ độ thế giới)
    float    ViewWidth() const { return kViewWidth / zoom_; }   // bề ngang thế giới đang thấy
    Camera2D Camera() const;
    void     SetZoom(float z) { zoomTarget_ = z; }

    // --- sự kiện ----------------------------------------------------------
    void Emit(const GameEvent &e) { events_.push_back(e); }
    EventQueue &Events() { return events_; }

    // Khoá điều khiển khi đang chiếu chữ "FIGHT!" hoặc "K.O.".
    void SetInputEnabled(bool on) { inputEnabled_ = on; }

private:
    void ResolveBodies();
    void ResolveAttack(Fighter &attacker, Fighter &defender);
    void ResolveProjectiles();
    void FaceEachOther();
    void ConstrainFighters();
    void UpdateCamera(float dt, bool snap);

    std::unique_ptr<Fighter> p1_, p2_;
    std::vector<std::unique_ptr<Projectile>> projectiles_;
    ParticleSystem fx_;
    EventQueue events_;

    float   hitStop_ = 0.0f;
    float   superFreeze_ = 0.0f;
    int     superOwner_ = -1;
    float   shake_   = 0.0f;
    Vector2 shakeOffset_{};
    float   camX_ = 0.0f;
    float   zoom_ = 1.0f;
    float   zoomTarget_ = 1.0f;
    bool    inputEnabled_ = true;
};

} // namespace fighter
