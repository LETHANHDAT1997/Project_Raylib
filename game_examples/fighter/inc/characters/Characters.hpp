#pragma once
#include "entities/Fighter.hpp"
#include "entities/Projectile.hpp"
#include <vector>

namespace fighter {

// ============================================================================
// Bốn nhân vật. Mỗi lớp khai báo 4 chiêu đặc biệt (mỗi chiêu 3 lực L/M/H) và
// một siêu chiêu, cài đặt phần "đặc sản" bằng các hook của Fighter. Đòn thường,
// đỡ, vật, combo... thừa kế nguyên từ lớp cơ sở.
// ============================================================================

// --- MACK: lãng khách kiểu "shoto" - đạn, chém xoáy, chém vọt, phản đòn ------
class Samurai final : public Fighter {
public:
    Samurai(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}
    bool OnIncomingHit(Arena &arena, Fighter &attacker, const MoveDef &move) override;

protected:
    MoveDef Special(MoveId id, Strength s) const override;
    MoveDef SuperMove() const override;
    bool CanUseSpecial(Arena &arena, MoveId id) const override;
    void OnMoveActive(Arena &arena, const MoveDef &m) override;
    void OnMoveUpdate(Arena &arena, const MoveDef &m, float t, float dt) override;
    void UpdateCharacter(Arena &arena, float dt) override;
    void DrawFront() const override;

private:
    MoveDef CounterStrike() const;
    float counterGlow_ = 0.0f;
    float trailTimer_  = 0.0f;
    bool  waveFired_   = false;
};

// --- KENJI: ninja áp sát - phi tiêu, lướt xuyên, bổ nhào, dịch chuyển --------
class Kenji final : public Fighter {
public:
    Kenji(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}

protected:
    MoveDef Special(MoveId id, Strength s) const override;
    MoveDef SuperMove() const override;
    bool CanUseSpecial(Arena &arena, MoveId id) const override;
    void OnMoveStart(Arena &arena, const MoveDef &m) override;
    void OnMoveActive(Arena &arena, const MoveDef &m) override;
    void OnMoveUpdate(Arena &arena, const MoveDef &m, float t, float dt) override;
    void UpdateCharacter(Arena &arena, float dt) override;
    void DrawBehind() const override;

private:
    struct Afterimage { Vector2 pos; float life; bool facingRight; int frame; const Animation *anim; };
    void PushAfterimage();
    std::vector<Afterimage> trail_;
    float ghostTimer_ = 0.0f;
    float warpTimer_  = 0.0f;
    int   warpSide_   = 1;
};

// --- GARETH: đô vật bọc thép - húc khiên, bổ đất, chém vọt có giáp, vật lệnh -
class Knight final : public Fighter {
public:
    Knight(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}

protected:
    MoveDef Special(MoveId id, Strength s) const override;
    MoveDef SuperMove() const override;
    void OnMoveActive(Arena &arena, const MoveDef &m) override;
    void OnMoveUpdate(Arena &arena, const MoveDef &m, float t, float dt) override;
    void OnMoveLand(Arena &arena, const MoveDef &m) override;
};

// --- MALTHUS: pháp sư giữ khoảng cách - cầu hắc ám, cột đất, bộc phá, dịch
//     chuyển, siêu chiêu gọi sét ----------------------------------------------
class Wizard final : public Fighter {
public:
    Wizard(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}

protected:
    MoveDef Special(MoveId id, Strength s) const override;
    MoveDef SuperMove() const override;
    bool CanUseSpecial(Arena &arena, MoveId id) const override;
    void OnMoveStart(Arena &arena, const MoveDef &m) override;
    void OnMoveActive(Arena &arena, const MoveDef &m) override;
    void UpdateCharacter(Arena &arena, float dt) override;
    void DrawBehind() const override;
    void DrawFront() const override;

private:
    float orbPhase_ = 0.0f;
    float burstFx_  = 0.0f;
    int   strikesLeft_ = 0;       // siêu chiêu: số tia sét còn phải gọi
    float strikeTimer_ = 0.0f;
};

// ============================================================================
// Đạn & vùng sát thương
// ============================================================================

// Khí kiếm hình trăng lưỡi liềm (Mack).
class WaveProjectile final : public Projectile {
public:
    WaveProjectile(Vector2 pos, float dir, int owner, int damage, float speed, Color color, float scale);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
private:
    float scale_;
};

// Phi tiêu bốn cánh xoay tít (Kenji).
class ShurikenProjectile final : public Projectile {
public:
    ShurikenProjectile(Vector2 pos, Vector2 vel, int owner, int damage, Color color);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
private:
    float spin_ = 0.0f;
};

// Cầu năng lượng hắc ám (Malthus).
class OrbProjectile final : public Projectile {
public:
    OrbProjectile(Vector2 pos, float dir, int owner, int damage, float speed, Color color);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
private:
    float spin_ = 0.0f;
};

// Sóng chấn động chạy dọc mặt đất (Gareth).
class ShockwaveProjectile final : public Projectile {
public:
    ShockwaveProjectile(Vector2 pos, float dir, int owner, int damage, float speed, float life, float size);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
    Rectangle Box() const override;
    bool Clashable() const override { return false; }
private:
    float phase_ = 0.0f;
    float size_  = 1.0f;
};

// Cột hắc ám trồi lên từ mặt đất sau một nhịp báo trước (Malthus).
class PillarHazard final : public Projectile {
public:
    PillarHazard(float x, int owner, int damage, Color color, float delay);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
    Rectangle Box() const override;
    bool Harmful() const override;
    bool Clashable() const override { return false; }
private:
    float delay_;
    bool  erupted_ = false;
};

// Tia sét giáng từ trời (siêu chiêu của Malthus).
class LightningHazard final : public Projectile {
public:
    LightningHazard(float x, int owner, int damage, Color color, float delay);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
    Rectangle Box() const override;
    bool Harmful() const override;
    bool Clashable() const override { return false; }
private:
    float delay_;
    bool  struck_ = false;
};

} // namespace fighter
