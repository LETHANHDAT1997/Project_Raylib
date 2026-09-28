#pragma once
#include "raylib.h"
#include "entities/Move.hpp"

namespace fighter {

class Arena;
class Fighter;

// ============================================================================
// Projectile - lớp cơ sở cho mọi thứ tách khỏi người nhân vật: cầu năng lượng,
// phi tiêu, sóng đất, cột hắc ám, sét... Arena chỉ cần Update/Draw/Box.
//
// Luật kiểu Street Fighter:
//   - hai quả đạn của hai bên chạm nhau thì triệt tiêu (clash)
//   - đỡ được đạn, nhưng vẫn mất một chút máu (chip)
//   - mỗi nhân vật thường chỉ có 1 quả đạn trên màn hình
// ============================================================================
class Projectile {
public:
    Projectile(Vector2 pos, float dir, int ownerId, int damage);
    virtual ~Projectile() = default;

    virtual void Update(Arena &arena, float dt);
    virtual void Draw() const = 0;
    virtual Rectangle Box() const;

    // Đang gây sát thương được không (cột hắc ám cần thời gian "nhú" lên).
    virtual bool Harmful() const { return alive_; }
    virtual bool Clashable() const { return true; }
    virtual void OnHit(Arena &arena, Fighter &target, bool blocked);
    virtual void OnClash(Arena &arena);

    bool  Alive()   const { return alive_; }
    int   OwnerId() const { return ownerId_; }
    float Dir()     const { return dir_; }
    Vector2 Position() const { return pos_; }
    HitHeight Height() const { return height_; }
    // Đạn sinh ra từ siêu chiêu: được đánh trúng người đang bị tung hứng và
    // hưởng mức giảm sát thương combo nhẹ hơn.
    bool  FromSuper() const { return fromSuper_; }
    void  MarkSuper() { fromSuper_ = true; }
    void  Kill() { alive_ = false; }

protected:
    Vector2 pos_{};
    Vector2 vel_{};
    float dir_       = 1.0f;
    float radius_    = 22.0f;
    float life_      = 3.0f;
    float age_       = 0.0f;
    int   damage_    = 70;
    int   chip_      = 10;
    int   ownerId_   = 0;
    int   hitsLeft_  = 1;
    float hitCooldown_ = 0.0f;
    bool  alive_     = true;
    bool  knockdown_ = false;
    bool  launcher_  = false;
    bool  fromSuper_ = false;
    float knockback_ = 300.0f;
    float hitstun_   = 0.34f;
    float blockstun_ = 0.2f;
    HitHeight height_ = HitHeight::Mid;
    SfxWeight weight_ = SfxWeight::Medium;
    Color color_     = SKYBLUE;
};

} // namespace fighter
