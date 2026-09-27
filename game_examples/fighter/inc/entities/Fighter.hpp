#pragma once
#include "raylib.h"
#include "core/Animation.hpp"
#include "core/Config.hpp"
#include "core/Input.hpp"
#include "entities/Move.hpp"

namespace fighter {

class Arena;
struct CharacterDef;

enum class FighterState {
    Idle, Walk, Crouch, Jump, Fall, Block,
    Attack, Hurt, Knockdown, Victory, Defeat
};

// ============================================================================
// Fighter - lớp cơ sở trừu tượng cho mọi nhân vật.
//
// Lớp này nắm tất cả những gì MỌI nhân vật đều giống nhau: vật lý, máy trạng
// thái, hitbox, đỡ đòn, combo, vẽ sprite. Cái RIÊNG của từng nhân vật nằm ở
// các hàm ảo bên dưới - lớp con chỉ cần khai báo frame data 4 đòn và cài đặt
// hiệu ứng chiêu của mình.
// ============================================================================
class Fighter {
public:
    Fighter(const CharacterDef &def, bool facingRight);
    virtual ~Fighter() = default;

    Fighter(const Fighter &) = delete;
    Fighter &operator=(const Fighter &) = delete;

    // --- vòng đời ---------------------------------------------------------
    void Reset(float x, bool facingRight);
    void Update(Arena &arena, const InputState &in, Fighter &opponent, float dt);
    void DrawShadow() const;
    void Draw(bool debugBoxes) const;

    // --- trạng thái trận đấu ---------------------------------------------
    void ReceiveHit(Arena &arena, Fighter &attacker, const MoveDef &move, bool blocked);
    void SetOutcome(bool won);
    void FreezeForRoundEnd();

    // --- truy vấn ---------------------------------------------------------
    const CharacterDef &Def()      const { return def_; }
    Vector2  Position()            const { return pos_; }
    Vector2  Velocity()            const { return vel_; }
    bool     FacingRight()         const { return facingRight_; }
    bool     OnGround()            const { return onGround_; }
    int      Health()              const { return health_; }
    int      MaxHealth()           const { return maxHealth_; }
    float    HealthRatio()         const { return (float)health_ / (float)maxHealth_; }
    float    Meter()               const { return meter_; }
    float    MeterRatio()          const { return meter_ / kMaxMeter; }
    int      ComboCount()          const { return comboCount_; }
    float    ComboTimer()          const { return comboTimer_; }
    bool     IsDefeated()          const { return health_ <= 0; }
    bool     IsBlocking()          const { return state_ == FighterState::Block; }
    bool     IsAttacking()         const { return state_ == FighterState::Attack; }
    bool     IsBusy()              const;   // đang ra đòn / dính đòn / ngã
    FighterState State()           const { return state_; }
    float    SpecialCooldown()     const { return specialCd_; }
    Vector2  Center()              const;   // tâm thân người, để bắn hiệu ứng
    Rectangle Hurtbox()            const;
    const ActiveAttack &Attack()   const { return attack_; }

    void AddMeter(float amount);
    void ConsumeMeter(float amount);
    void ApplyKnockback(float vx, float vy);

    // --- Arena gọi khi xử lý va chạm --------------------------------------
    // Đòn hiện tại còn được phép trúng nữa không (chống 1 cú vung ăn nhiều lần).
    bool CanAttackHit() const;
    // Báo cho người tấn công biết đòn đã chạm, để trừ số hit và cộng thanh Super.
    void NotifyAttackLanded(Arena &arena, Fighter &target, bool blocked);
    // Quay mặt về phía một toạ độ x.
    void FaceTowards(float x);
    // Đặt lại toạ độ ngang (Arena dùng khi tách hai người đang chồng lên nhau).
    void Reposition(float x) { pos_.x = x; }

    // Hệ số sát thương nhận vào - màn Settings dùng để chỉnh độ khó.
    void SetIncomingDamageScale(float s) { incomingScale_ = s; }
    void SetMeterGainScale(float s)      { meterScale_ = s; }

protected:
    // --- phần mỗi nhân vật tự định nghĩa ---------------------------------

    // Frame data của 4 ô chiêu. Bắt buộc.
    virtual const MoveDef &Move(MoveSlot slot) const = 0;

    // Gọi đúng lúc hitbox của Special/Super bật lên - nơi bắn đạn, dịch chuyển,
    // tạo hiệu ứng riêng. Mặc định không làm gì (đòn thuần cận chiến).
    virtual void OnSpecialActivate(Arena &arena) { (void)arena; }
    virtual void OnSuperActivate(Arena &arena)   { (void)arena; }

    // Gọi mỗi khi đòn của nhân vật này trúng đối thủ.
    virtual void OnHitConfirm(Arena &arena, MoveSlot slot, Vector2 point)
    { (void)arena; (void)slot; (void)point; }

    // Cập nhật riêng mỗi frame (hào quang, bộ đếm nội tại...).
    virtual void UpdateCharacter(Arena &arena, float dt) { (void)arena; (void)dt; }

    // Vẽ thêm phía sau / phía trước sprite (hào quang, vệt kiếm...).
    virtual void DrawBehind(Vector2 screenPos, float scale) const { (void)screenPos; (void)scale; }
    virtual void DrawFront(Vector2 screenPos, float scale)  const { (void)screenPos; (void)scale; }

    // Tiện ích cho lớp con
    float FacingSign() const { return facingRight_ ? 1.0f : -1.0f; }
    void  StartMove(MoveSlot slot);
    bool  CanAct() const;

    // --- dữ liệu lớp con được đọc ---------------------------------------
    const CharacterDef &def_;
    Vector2 pos_{};
    Vector2 vel_{};
    bool    facingRight_ = true;
    bool    onGround_    = true;
    float   stateTimer_  = 0.0f;
    Animator animator_;

private:
    void UpdateControl(Arena &arena, const InputState &in, Fighter &opponent, float dt);
    void UpdatePhysics(float dt);
    void UpdateAttack(Arena &arena, float dt);
    void UpdateAnimation(float dt);
    void SetState(FighterState s);
    void PlayAnim(AnimId id, bool restart);
    Rectangle WorldBox(const Rectangle &local) const;

    FighterState state_ = FighterState::Idle;

    int   health_    = kMaxHealth;
    int   maxHealth_ = kMaxHealth;
    float meter_     = 0.0f;

    // Pha của đòn đang ra
    MoveSlot activeSlot_ = MoveSlot::Light;
    float    moveTimer_  = 0.0f;
    int      hitsLeft_   = 0;
    float    nextHitAt_  = 0.0f;
    bool     consumedHit_= false;
    bool     specialFired_ = false;
    ActiveAttack attack_{};

    float hitstun_   = 0.0f;
    float blockstun_ = 0.0f;
    float specialCd_ = 0.0f;
    float superCd_   = 0.0f;
    float armorLeft_ = 0.0f;

    int   comboCount_ = 0;
    float comboTimer_ = 0.0f;

    float flashTimer_ = 0.0f;
    float dustTimer_  = 0.0f;
    float incomingScale_ = 1.0f;
    float meterScale_    = 1.0f;
    bool  frozen_        = false;
};

} // namespace fighter
