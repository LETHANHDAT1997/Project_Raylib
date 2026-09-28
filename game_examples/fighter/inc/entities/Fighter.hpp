#pragma once
#include "raylib.h"
#include "core/Animation.hpp"
#include "core/Config.hpp"
#include "core/Input.hpp"
#include "entities/Move.hpp"
#include <vector>

namespace fighter {

class Arena;
struct CharacterDef;

enum class FighterState {
    Idle, WalkF, WalkB, Crouch, Jump, Dash, BackDash,
    Attack, Block, Hurt, AirHurt, Knockdown, Wakeup, Thrown,
    Intro, Victory, Defeat
};

// Một vệt chém đang mờ dần - dựng từ SlashFx của đòn.
struct SlashTrail {
    Vector2 offset{};       // so với gốc chân, hướng mặt phải
    float   radius = 100.0f;
    float   thick  = 26.0f;
    float   a0 = 0.0f, a1 = 0.0f;   // độ, 0 = phải, 90 = xuống
    float   life = 0.0f, maxLife = 0.2f;
    Color   color = WHITE;
    bool    facingRight = true;
};

// ============================================================================
// Fighter - lớp cơ sở trừu tượng cho mọi nhân vật.
//
// Lớp này nắm những gì MỌI nhân vật đều giống nhau: đọc lệnh quay tay, máy
// trạng thái, đỡ theo độ cao đòn, combo/cancel, tung hứng, ngã-đứng dậy, vật,
// vẽ sprite. Cái RIÊNG của từng nhân vật nằm ở các hàm ảo bên dưới.
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
    // Trong lúc khựng hình / tung siêu chiêu vẫn phải ghi nhận phím bấm, nếu
    // không người chơi bấm nối combo đúng nhịp khựng sẽ bị "nuốt" mất phím.
    void BufferInput(const InputState &in);
    void DrawShadow() const;
    void Draw(bool debugBoxes) const;
    void DrawTrails() const;              // vệt chém, vẽ ở chế độ cộng sáng

    void PlayIntro();
    void SetOutcome(bool won);
    void FreezeForRoundEnd();

    // --- nhận đòn / đỡ đòn (Arena gọi) -----------------------------------
    bool WantsToBlock(const Fighter &attacker) const;
    bool WantsToBlockFrom(float sourceX) const;   // đạn: đỡ theo vị trí quả đạn
    void WhiffCurrentHit();                        // đòn bị "nuốt" (phản đòn)
    bool CanBlockHeight(HitHeight h) const;
    // Trả về lượng máu thực mất (sau hệ số combo, phòng thủ...).
    int  ReceiveHit(Arena &arena, Fighter &attacker, const MoveDef &move,
                    bool blocked, bool counter);
    void NotifyAttackLanded(Arena &arena, Fighter &target, bool blocked, int damage);
    bool CanAttackHit() const;
    bool InHitstun() const;              // đang dính đòn (để đếm combo)
    void EndCombo();                     // đối thủ đã thoát combo
    void NotifyProjectileCombo(int damage);

    // --- vật ----------------------------------------------------------------
    bool CanBeThrown() const;
    bool PressedThrowRecently() const;     // để phá đòn vật
    void BeginThrow(Arena &arena, Fighter &victim);
    void BeginThrown(Fighter &thrower);
    void ReleaseFromThrow(Arena &arena, Fighter &thrower, const MoveDef &move, bool toss_behind);
    void BreakThrow(float pushDir);

    // --- phản đòn (lớp con quyết định) ------------------------------------
    // Trả về true nếu nhân vật "nuốt" đòn này (thế phản đòn).
    virtual bool OnIncomingHit(Arena &arena, Fighter &attacker, const MoveDef &move)
    { (void)arena; (void)attacker; (void)move; return false; }

    // --- truy vấn ---------------------------------------------------------
    const CharacterDef &Def()   const { return def_; }
    Vector2  Position()         const { return pos_; }
    Vector2  Velocity()         const { return vel_; }
    bool     FacingRight()      const { return facingRight_; }
    float    FacingSign()       const { return facingRight_ ? 1.0f : -1.0f; }
    bool     OnGround()         const { return onGround_; }
    int      Health()           const { return health_; }
    int      MaxHealth()        const { return maxHealth_; }
    float    HealthRatio()      const { return (float)health_ / (float)maxHealth_; }
    float    Meter()            const { return meter_; }
    int      MeterStocks()      const { return (int)(meter_ / kMeterPerStock); }
    int      ComboCount()       const { return comboCount_; }
    int      ComboDamage()      const { return comboDamage_; }
    float    ComboTimer()       const { return comboTimer_; }
    bool     IsDefeated()       const { return health_ <= 0; }
    bool     IsBlocking()       const { return state_ == FighterState::Block; }
    bool     IsAttacking()      const { return state_ == FighterState::Attack; }
    bool     InStartupOrActive() const;
    bool     IsBusy()           const;
    bool     IsInvulnerable()   const;
    bool     IsProjInvulnerable() const;
    bool     IsCrouching()      const;
    bool     IsAirborne()       const { return !onGround_; }
    bool     CurrentAttackIsLow() const;
    bool     HoldingBackFrom(const Fighter &other) const;
    FighterState State()        const { return state_; }
    const MoveDef &CurrentMove() const { return move_; }
    float    MoveTime()         const { return moveTimer_; }
    int      HitsLeft()         const { return hitsLeft_; }
    bool     PassesThrough()    const;
    Vector2  Center()           const;
    Rectangle Hurtbox()         const;
    const ActiveAttack &Attack() const { return attack_; }
    bool     SuperFlashing()    const { return superGlow_ > 0.0f; }

    // --- tiện ích Arena dùng --------------------------------------------
    void AddMeter(float amount);
    void ConsumeMeter(float amount);
    void ApplyKnockback(float vx, float vy);
    void FaceTowards(float x);
    void Reposition(float x) { pos_.x = x; }
    void SetPosition(Vector2 p) { pos_ = p; }
    void SetIncomingDamageScale(float s) { incomingScale_ = s; }
    void SetMeterGainScale(float s)      { meterScale_ = s; }
    void RefillForTraining();            // luyện tập: hồi đầy máu khi hết combo, không chết

    static constexpr float kMeterPerStock = 100.0f;
    static constexpr float kMaxMeterValue = 200.0f;   // 2 thanh Super

protected:
    // --- phần mỗi nhân vật tự định nghĩa ---------------------------------

    // Đòn thường. Mặc định dựng từ thông số chung trong CharacterDef; lớp con
    // ghi đè nếu muốn một đòn thường "đặc sản".
    virtual MoveDef Normal(MoveId id) const;
    // Bốn chiêu đặc biệt, mỗi chiêu có 3 lực (L/M/H) như Street Fighter.
    virtual MoveDef Special(MoveId id, Strength s) const = 0;
    virtual MoveDef SuperMove() const = 0;

    // Có được tung chiêu này ngay lúc này không (ví dụ: mỗi lúc chỉ 1 quả đạn).
    virtual bool CanUseSpecial(Arena &arena, MoveId id) const
    { (void)arena; (void)id; return true; }

    virtual void OnMoveStart(Arena &arena, const MoveDef &m)  { (void)arena; (void)m; }
    virtual void OnMoveActive(Arena &arena, const MoveDef &m) { (void)arena; (void)m; }
    virtual void OnMoveUpdate(Arena &arena, const MoveDef &m, float t, float dt)
    { (void)arena; (void)m; (void)t; (void)dt; }
    virtual void OnMoveHit(Arena &arena, const MoveDef &m, Fighter &target, bool blocked)
    { (void)arena; (void)m; (void)target; (void)blocked; }
    virtual void OnMoveLand(Arena &arena, const MoveDef &m) { (void)arena; (void)m; }
    virtual void UpdateCharacter(Arena &arena, float dt) { (void)arena; (void)dt; }
    virtual void DrawBehind() const {}
    virtual void DrawFront()  const {}

    // --- tiện ích cho lớp con ---------------------------------------------
    void StartMove(Arena &arena, const MoveDef &m);
    void EndMove();
    void SpawnTrail(SlashFx fx, float radius, Color color);
    void DrawSpriteFrame(const Animation &a, int frame, Vector2 pos, bool faceRight,
                         Color tint, float squash = 1.0f) const;
    Color Accent() const;

    const CharacterDef &def_;
    Vector2 pos_{};
    Vector2 vel_{};
    bool    facingRight_ = true;
    bool    onGround_    = true;
    float   stateTimer_  = 0.0f;
    float   clock_       = 0.0f;
    Animator animator_;
    MoveDef  move_{};
    float    moveTimer_ = 0.0f;
    Fighter *opponent_  = nullptr;

private:
    enum Button { BtnL, BtnM, BtnH, BtnS, BtnU, BtnCount };

    void ReadInput(const InputState &in, float dt);
    bool TryStartFromInput(Arena &arena, bool cancelling);
    bool ButtonBuffered(Button b) const;
    void ConsumeButton(Button b);
    Strength BufferedStrength() const;
    MoveDef BuildMove(MoveId id, Strength s) const;

    void UpdateNeutral(Arena &arena, Fighter &opponent);
    void UpdateAttack(Arena &arena, float dt);
    void UpdateThrowHold(Arena &arena, float dt);
    void UpdatePhysics(Arena &arena, float dt);
    void UpdateAnimation(float dt);
    void UpdateTrails(float dt);
    void SetState(FighterState s);
    void PlayAnim(AnimId id, const PlayMode &mode, bool restart);
    void PlayLoop(AnimId id);
    Rectangle WorldBox(const Rectangle &local) const;
    int  RelativeDir() const;

    FighterState state_ = FighterState::Idle;

    int   health_    = kMaxHealth;
    int   maxHealth_ = kMaxHealth;
    float meter_     = 0.0f;

    // Đầu vào
    InputState  in_{};
    InputBuffer buffer_;
    float pressedAt_[BtnCount] = {-10, -10, -10, -10, -10};
    int   lastDir_ = 5;
    float throwPressAt_ = -10.0f;       // lần gần nhất bấm L+M (không bị tiêu)
    float dashTapTime_ = -10.0f;
    int   dashTapDir_  = 0;

    // Đòn đang ra
    int   hitsLeft_  = 0;
    float nextHitAt_ = 0.0f;
    bool  consumed_  = false;
    bool  contact_   = false;       // đã chạm (trúng hoặc bị đỡ)
    bool  activated_ = false;
    bool  armorUsed_ = false;
    bool  startedAirborne_ = false;
    ActiveAttack attack_{};

    // Vật
    Fighter *throwVictim_ = nullptr;
    float    throwTimer_  = 0.0f;
    bool     throwBehind_ = false;

    // Trạng thái chịu đòn
    float hitstun_   = 0.0f;
    float blockstun_ = 0.0f;
    bool  crouchBlock_ = false;
    bool  pendingKnockdown_ = false;
    int   airHitsLeft_ = 0;             // số đòn tung hứng còn được nhận
    bool  crouchHurt_  = false;
    float invuln_      = 0.0f;
    int   dashRequested_ = 0;           // 6 = lướt tới, 4 = lướt lùi

    int   comboCount_  = 0;
    int   comboDamage_ = 0;
    float comboTimer_  = 0.0f;

    // Hình ảnh
    float flashTimer_ = 0.0f;
    float counterFlash_ = 0.0f;
    float superGlow_  = 0.0f;
    float squash_     = 1.0f;
    float spinTimer_  = 0.0f;
    bool  spinFlip_   = false;
    float tilt_       = 0.0f;
    float dustTimer_  = 0.0f;
    std::vector<SlashTrail> trails_;

    float incomingScale_ = 1.0f;
    float meterScale_    = 1.0f;
    bool  frozen_        = false;
};

} // namespace fighter
