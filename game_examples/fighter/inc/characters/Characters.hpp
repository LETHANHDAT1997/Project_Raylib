#pragma once
#include "entities/Fighter.hpp"
#include "entities/Projectile.hpp"
#include <vector>

namespace fighter {

// ============================================================================
// Bốn nhân vật hiện có. Mỗi lớp chỉ khai báo frame data của 4 ô chiêu và cài
// đặt phần "đặc sản" của mình - phần còn lại thừa kế nguyên từ Fighter.
// ============================================================================

// --- Mack: lãng khách cân bằng, chiêu là cú chém bay vọt lên chống nhảy ------
class Samurai final : public Fighter {
public:
    Samurai(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}

protected:
    const MoveDef &Move(MoveSlot slot) const override;
    void OnSpecialActivate(Arena &arena) override;
    void OnSuperActivate(Arena &arena) override;
    void OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point) override;
    void DrawFront(Vector2 screenPos, float scale) const override;
    void UpdateCharacter(Arena &arena, float dt) override;

private:
    float slashGlow_ = 0.0f;
};

// --- Kenji: áp sát tốc độ cao, chiêu là cú lướt xuyên qua đối thủ ------------
class Kenji final : public Fighter {
public:
    Kenji(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}

protected:
    const MoveDef &Move(MoveSlot slot) const override;
    void OnSpecialActivate(Arena &arena) override;
    void OnSuperActivate(Arena &arena) override;
    void OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point) override;
    void UpdateCharacter(Arena &arena, float dt) override;
    void DrawBehind(Vector2 screenPos, float scale) const override;

private:
    struct Afterimage { Vector2 pos; float life; bool facingRight; int frame; };
    std::vector<Afterimage> trail_;
    float dashGlow_ = 0.0f;
};

// --- Knight: chậm, trâu, đòn nặng có armor, chiêu là sóng chấn động mặt đất --
class Knight final : public Fighter {
public:
    Knight(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}

protected:
    const MoveDef &Move(MoveSlot slot) const override;
    void OnSpecialActivate(Arena &arena) override;
    void OnSuperActivate(Arena &arena) override;
    void OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point) override;
};

// --- Wizard: giữ khoảng cách, chiêu là cầu hắc ám, chiêu cuối là ba quả đuổi -
class Wizard final : public Fighter {
public:
    Wizard(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}

protected:
    const MoveDef &Move(MoveSlot slot) const override;
    void OnSpecialActivate(Arena &arena) override;
    void OnSuperActivate(Arena &arena) override;
    void OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point) override;
    void UpdateCharacter(Arena &arena, float dt) override;
    void DrawBehind(Vector2 screenPos, float scale) const override;

private:
    float orbPhase_ = 0.0f;
    float superQueue_ = 0.0f;
    int   superShots_ = 0;
};

// ============================================================================
// Các loại đạn
// ============================================================================

// Cầu năng lượng bay ngang, có đuôi và lõi sáng.
class OrbProjectile final : public Projectile {
public:
    OrbProjectile(Vector2 pos, float dir, int ownerId, int damage, Color color, float speed);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
    void OnHit(Arena &arena, Fighter &target) override;

private:
    Color core_;
    float spin_ = 0.0f;
};

// Sóng chấn động chạy sát mặt đất, không bị chặn bởi độ cao.
class ShockwaveProjectile final : public Projectile {
public:
    ShockwaveProjectile(Vector2 pos, float dir, int ownerId, int damage);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;
    Rectangle Box() const override;

private:
    float phase_ = 0.0f;
};

// Khí kiếm mỏng, bay nhanh, xoay theo hướng.
class SlashProjectile final : public Projectile {
public:
    SlashProjectile(Vector2 pos, float dir, int ownerId, int damage, Color color);
    void Update(Arena &arena, float dt) override;
    void Draw() const override;

private:
    float spin_ = 0.0f;
};

} // namespace fighter
