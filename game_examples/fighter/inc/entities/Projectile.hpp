#pragma once
#include "raylib.h"
#include <memory>

namespace fighter {

class Arena;
class Fighter;

// ============================================================================
// Projectile - lớp cơ sở cho mọi thứ bay ra khỏi tay nhân vật.
//
// Wizard bắn cầu năng lượng, Knight tạo sóng đất chạy dọc mặt sàn, Samurai
// phóng khí kiếm... ba thứ đó khác nhau ở quỹ đạo và cách vẽ, nên mỗi loại là
// một lớp con; Arena chỉ cần biết Update/Draw/Box.
// ============================================================================
class Projectile {
public:
    Projectile(Vector2 pos, float dir, int ownerId, int damage);
    virtual ~Projectile() = default;

    virtual void Update(Arena &arena, float dt);
    virtual void Draw() const = 0;

    // Gọi khi chạm đối thủ. Mặc định: gây sát thương rồi tự huỷ.
    virtual void OnHit(Arena &arena, Fighter &target);

    virtual Rectangle Box() const;

    bool  Alive()    const { return alive_; }
    int   OwnerId()  const { return ownerId_; }
    int   Damage()   const { return damage_; }
    void  Kill()           { alive_ = false; }
    Vector2 Position() const { return pos_; }

protected:
    Vector2 pos_{};
    Vector2 vel_{};
    float dir_       = 1.0f;
    float radius_    = 22.0f;
    float life_      = 3.0f;
    float age_       = 0.0f;
    int   damage_    = 80;
    int   ownerId_   = 0;
    bool  alive_     = true;
    float knockback_ = 320.0f;
    float hitstun_   = 0.34f;
    Color color_     = SKYBLUE;
};

} // namespace fighter
