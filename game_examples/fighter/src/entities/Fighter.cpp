#include "entities/Fighter.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

namespace {

// Khối va chạm thống nhất cho mọi nhân vật (sprite mỗi pack một tỉ lệ nên
// bám theo pixel ảnh sẽ không công bằng).
constexpr float kBodyWidth    = 70.0f;
constexpr float kBodyHeight   = 168.0f;
constexpr float kCrouchHeight = 108.0f;
constexpr float kAirHeight    = 130.0f;

constexpr float kCrouchSquash = 0.72f;   // tư thế ngồi = idle nén còn 72%
constexpr float kButtonBuffer = 0.10f;   // bấm sớm tối đa 0.1s vẫn được nhận
constexpr float kDashWindow   = 0.22f;   // nhấn đúp trong khoảng này = lướt

float Approach(float v, float target, float rate, float dt)
{
    const float k = 1.0f - std::exp(-rate * dt);
    return v + (target - v) * k;
}

} // namespace

bool IsNormal(MoveId id)  { return (int)id <= (int)MoveId::AirH; }
bool IsSpecial(MoveId id) { return id >= MoveId::SpecialA && id <= MoveId::SpecialD; }

// ============================================================================
// Khởi tạo
// ============================================================================
Fighter::Fighter(const CharacterDef &def, bool facingRight)
    : def_(def)
{
    maxHealth_   = def.maxHealth;
    health_      = maxHealth_;
    facingRight_ = facingRight;
    pos_ = Vector2{0.0f, kGroundY};
    PlayLoop(AnimId::Idle);
}

void Fighter::Reset(float x, bool facingRight)
{
    pos_ = Vector2{x, kGroundY};
    vel_ = Vector2{0.0f, 0.0f};
    facingRight_ = facingRight;
    onGround_ = true;
    health_ = maxHealth_;
    state_ = FighterState::Idle;
    stateTimer_ = moveTimer_ = 0.0f;
    hitstun_ = blockstun_ = invuln_ = 0.0f;
    comboCount_ = comboDamage_ = 0;
    comboTimer_ = 0.0f;
    flashTimer_ = counterFlash_ = superGlow_ = 0.0f;
    squash_ = 1.0f;
    tilt_ = 0.0f;
    frozen_ = false;
    throwVictim_ = nullptr;
    attack_ = ActiveAttack{};
    buffer_.Clear();
    for (float &t : pressedAt_) t = -10.0f;
    dashRequested_ = 0;
    trails_.clear();
    animator_.SetSpeed(1.0f);
    PlayLoop(AnimId::Idle);
    // Thanh Super giữ nguyên qua các hiệp, giống Street Fighter.
}

void Fighter::RefillForTraining()
{
    if (health_ <= 0) health_ = 1;   // không bao giờ chết trong luyện tập
    const bool resting = !InHitstun() && state_ != FighterState::Knockdown &&
                         state_ != FighterState::Wakeup && state_ != FighterState::Block;
    if (resting) health_ = maxHealth_;
    meter_ = kMaxMeterValue;
}

// ============================================================================
// Đọc đầu vào
// ============================================================================
int Fighter::RelativeDir() const
{
    const bool fwd  = facingRight_ ? in_.right : in_.left;
    const bool back = facingRight_ ? in_.left  : in_.right;
    const int h = (fwd && !back) ? 1 : (back && !fwd) ? -1 : 0;
    const int v = in_.up ? 1 : in_.down ? -1 : 0;
    // numpad: 1 2 3 / 4 5 6 / 7 8 9
    return 5 + h + v * 3;
}

void Fighter::ReadInput(const InputState &in, float dt)
{
    (void)dt;
    in_ = in;
    const int d = RelativeDir();

    // Nhấn đúp tiến/lùi = lướt (dash).
    if (d != lastDir_) {
        if ((d == 6 || d == 4) && lastDir_ == 5) {
            if (dashTapDir_ == d && clock_ - dashTapTime_ < kDashWindow) dashRequested_ = d;
            dashTapDir_  = d;
            dashTapTime_ = clock_;
        }
        lastDir_ = d;
    }
    buffer_.Record(d, clock_);

    if (in.lightPressed)   pressedAt_[BtnL] = clock_;
    if (in.mediumPressed)  pressedAt_[BtnM] = clock_;
    if (in.heavyPressed)   pressedAt_[BtnH] = clock_;
    if (in.specialPressed) pressedAt_[BtnS] = clock_;
    if (in.superPressed)   pressedAt_[BtnU] = clock_;

    // Ghi riêng thời điểm bấm L+M: nút có thể bị tiêu cho đòn vật của chính
    // mình, nhưng vẫn phải tính là "đã bấm phá vật".
    if ((in.lightPressed || in.mediumPressed) &&
        clock_ - pressedAt_[BtnL] < 0.1f && clock_ - pressedAt_[BtnM] < 0.1f) {
        throwPressAt_ = clock_;
    }
}

void Fighter::BufferInput(const InputState &in)
{
    if (frozen_) return;
    ReadInput(in, 0.0f);
}

bool Fighter::ButtonBuffered(Button b) const { return clock_ - pressedAt_[b] <= kButtonBuffer; }
void Fighter::ConsumeButton(Button b)        { pressedAt_[b] = -10.0f; }

Strength Fighter::BufferedStrength() const
{
    if (ButtonBuffered(BtnH)) return Strength::Heavy;
    if (ButtonBuffered(BtnM)) return Strength::Medium;
    return Strength::Light;
}

bool Fighter::PressedThrowRecently() const
{
    // Phá vật: bấm L+M trong khoảng 0.25s quanh lúc bị tóm.
    return clock_ - throwPressAt_ < 0.25f;
}

MoveDef Fighter::BuildMove(MoveId id, Strength s) const
{
    if (id == MoveId::Super) return SuperMove();
    if (IsSpecial(id))       return Special(id, s);
    return Normal(id);
}

bool Fighter::TryStartFromInput(Arena &arena, bool cancelling)
{
    const bool anyAtk = ButtonBuffered(BtnL) || ButtonBuffered(BtnM) || ButtonBuffered(BtnH);
    const bool air = !onGround_;
    auto consumeAll = [this]() {
        ConsumeButton(BtnL); ConsumeButton(BtnM); ConsumeButton(BtnH);
        ConsumeButton(BtnS); ConsumeButton(BtnU);
    };

    // --- 1. Siêu chiêu: nút Super hoặc 236236 + đòn --------------------------
    const bool superInput = ButtonBuffered(BtnU) ||
                            (anyAtk && buffer_.Motion("236236", clock_, 0.9f));
    if (superInput && meter_ >= kMeterPerStock) {
        MoveDef m = SuperMove();
        const bool poseOk   = air ? m.airOk : m.groundOk;
        const bool cancelOk = !cancelling || (contact_ && move_.superCancel);
        if (poseOk && cancelOk) {
            consumeAll();
            StartMove(arena, m);
            return true;
        }
    }

    // --- 2. Chiêu đặc biệt: lệnh quay tay hoặc nút Special + hướng -----------
    MoveId sp = MoveId::Count;
    Strength st = Strength::Medium;
    if (anyAtk) {
        st = BufferedStrength();
        if      (buffer_.Motion("623", clock_, 0.45f)) sp = MoveId::SpecialC;
        else if (buffer_.Motion("214", clock_, 0.45f)) sp = MoveId::SpecialB;
        else if (buffer_.Motion("236", clock_, 0.45f)) sp = MoveId::SpecialA;
        else if (buffer_.Motion("252", clock_, 0.40f)) sp = MoveId::SpecialD;
    }
    if (sp == MoveId::Count && ButtonBuffered(BtnS)) {
        const int d = RelativeDir();
        st = Strength::Medium;
        if (d == 6 || d == 9)                 sp = MoveId::SpecialB;
        else if (d == 1 || d == 2 || d == 3)  sp = MoveId::SpecialC;
        else if (d == 4 || d == 7)            sp = MoveId::SpecialD;
        else                                  sp = MoveId::SpecialA;
    }
    if (sp != MoveId::Count) {
        MoveDef m = Special(sp, st);
        const bool poseOk   = air ? m.airOk : m.groundOk;
        const bool cancelOk = !cancelling ||
                              (contact_ && move_.specialCancel && IsNormal(move_.id));
        if (poseOk && cancelOk && CanUseSpecial(arena, sp)) {
            consumeAll();
            StartMove(arena, m);
            return true;
        }
        if (ButtonBuffered(BtnS) && !anyAtk) ConsumeButton(BtnS);
    }

    // --- 3. Vật: L + M cùng lúc ---------------------------------------------
    if (!cancelling && onGround_ && ButtonBuffered(BtnL) && ButtonBuffered(BtnM) &&
        std::fabs(pressedAt_[BtnL] - pressedAt_[BtnM]) < 0.08f) {
        consumeAll();
        StartMove(arena, Normal(MoveId::Throw));
        return true;
    }

    // --- 4. Đòn thường theo tư thế --------------------------------------------
    if (anyAtk) {
        const Strength s = BufferedStrength();
        const int d = RelativeDir();
        const bool crouch = (d == 1 || d == 2 || d == 3);

        MoveId id;
        if (air)          id = (MoveId)((int)MoveId::AirL    + (int)s);
        else if (crouch)  id = (MoveId)((int)MoveId::CrouchL + (int)s);
        else              id = (MoveId)((int)MoveId::StandL  + (int)s);

        if (cancelling) {
            // Nối đòn (chain): chỉ lên lực mạnh hơn, hoặc nhẹ nối nhẹ.
            const bool stronger = (int)s > (int)move_.strength;
            const bool lightLight = s == Strength::Light && move_.strength == Strength::Light;
            if (!(contact_ && move_.chainable && IsNormal(move_.id) && !air &&
                  (stronger || lightLight))) {
                return false;
            }
        }

        if (s == Strength::Heavy)       ConsumeButton(BtnH);
        else if (s == Strength::Medium) ConsumeButton(BtnM);
        else                            ConsumeButton(BtnL);
        StartMove(arena, Normal(id));
        return true;
    }
    return false;
}

// ============================================================================
// Đòn thường mặc định - dựng từ NormalProfile của nhân vật
// ============================================================================
MoveDef Fighter::Normal(MoveId id) const
{
    const NormalProfile &p = def_.normals;
    MoveDef m;
    m.id = id;

    switch (id) {
        case MoveId::StandL:
            m.strength = Strength::Light; m.anim = AnimId::Attack1; m.frameTo = p.lightTo;
            m.startup = 0.05f; m.active = 0.05f; m.recovery = 0.10f;
            m.box = {30, -152, 88, 46}; m.damage = 32; m.knockback = 150;
            m.hitstun = 0.24f; m.blockstun = 0.14f; m.meterGain = 4;
            m.chainable = m.specialCancel = m.superCancel = true;
            m.fx = SlashFx::Jab; m.sfx = SfxWeight::Light; m.label = "Đứng nhẹ";
            break;
        case MoveId::StandM:
            m.strength = Strength::Medium; m.anim = AnimId::Attack1;
            m.startup = 0.09f; m.active = 0.07f; m.recovery = 0.17f;
            m.box = {32, -164, 108, 66}; m.damage = 62; m.knockback = 220;
            m.hitstun = 0.32f; m.blockstun = 0.18f; m.meterGain = 6;
            m.chainable = m.specialCancel = m.superCancel = true;
            m.fx = SlashFx::Horizontal; m.sfx = SfxWeight::Medium; m.label = "Đứng vừa";
            break;
        case MoveId::StandH:
            m.strength = Strength::Heavy; m.anim = p.heavyAnim;
            m.startup = 0.15f; m.active = 0.09f; m.recovery = 0.28f;
            m.box = {30, -196, 128, 116}; m.damage = 96; m.knockback = 330;
            m.hitstun = 0.40f; m.blockstun = 0.22f; m.meterGain = 9;
            m.specialCancel = m.superCancel = true;
            m.fx = SlashFx::Overhead; m.fxRadius = 130; m.sfx = SfxWeight::Heavy;
            m.voice = true; m.label = "Đứng mạnh";
            break;
        case MoveId::CrouchL:
            m.strength = Strength::Light; m.anim = AnimId::Attack1; m.frameTo = p.lightTo;
            m.pose = Pose::Crouch;
            m.startup = 0.05f; m.active = 0.05f; m.recovery = 0.10f;
            m.box = {26, -64, 86, 40}; m.damage = 26; m.knockback = 140;
            m.hitstun = 0.22f; m.blockstun = 0.13f; m.meterGain = 4;
            m.height = HitHeight::Low;
            m.chainable = m.specialCancel = m.superCancel = true;
            m.fx = SlashFx::Low; m.fxRadius = 80; m.sfx = SfxWeight::Light; m.label = "Ngồi nhẹ";
            break;
        case MoveId::CrouchM:
            m.strength = Strength::Medium; m.anim = AnimId::Attack1; m.pose = Pose::Crouch;
            m.startup = 0.08f; m.active = 0.07f; m.recovery = 0.18f;
            m.box = {28, -92, 112, 62}; m.damage = 52; m.knockback = 200;
            m.hitstun = 0.30f; m.blockstun = 0.17f; m.meterGain = 6;
            m.chainable = m.specialCancel = m.superCancel = true;
            m.fx = SlashFx::Low; m.fxRadius = 105; m.sfx = SfxWeight::Medium; m.label = "Ngồi vừa";
            break;
        case MoveId::CrouchH:
            // Quét chân: đòn thấp, làm ngã, nhưng hụt thì hở rất lâu.
            m.strength = Strength::Heavy; m.anim = p.heavyAnim; m.pose = Pose::Crouch;
            m.startup = 0.13f; m.active = 0.10f; m.recovery = 0.40f;
            m.box = {22, -58, 146, 56}; m.damage = 86; m.knockback = 260;
            m.hitstun = 0.5f; m.blockstun = 0.2f; m.meterGain = 9;
            m.height = HitHeight::Low; m.knockdown = true;
            m.fx = SlashFx::Sweep; m.fxRadius = 150; m.sfx = SfxWeight::Heavy;
            m.voice = true; m.label = "Quét chân";
            break;
        case MoveId::AirL:
        case MoveId::AirM:
        case MoveId::AirH: {
            const int s = (int)id - (int)MoveId::AirL;
            m.strength = (Strength)s;
            m.anim = (s == 2) ? p.heavyAnim : AnimId::Attack1;
            if (s == 0) m.frameTo = p.lightTo;
            m.pose = Pose::Air; m.airOk = true; m.groundOk = false;
            m.keepMomentum = true; m.landCancel = true;
            m.startup = 0.05f + 0.03f * s; m.active = 0.12f + 0.02f * s; m.recovery = 0.10f;
            m.box = {18, -128.0f - 8 * s, 84.0f + 18 * s, 76.0f + 16 * s};
            m.damage = 36 + 24 * s; m.knockback = 160.0f + 60 * s;
            m.hitstun = 0.30f + 0.06f * s; m.blockstun = 0.16f + 0.03f * s;
            m.meterGain = 4.0f + 2 * s;
            m.height = HitHeight::Overhead;   // đòn nhảy phải ĐỨNG đỡ
            m.fx = SlashFx::AirDiag; m.fxRadius = 90.0f + 18 * s;
            m.sfx = (SfxWeight)s; m.voice = (s == 2);
            m.label = "Đòn trên không";
            break;
        }
        case MoveId::Throw:
            m.strength = Strength::Medium; m.anim = AnimId::Attack1; m.frameTo = p.lightTo;
            m.startup = 0.05f; m.active = 0.06f; m.recovery = 0.32f;
            m.box = {18, -160, 64, 130}; m.damage = p.throwDamage;
            m.knockback = 380; m.hitstun = 0.6f; m.knockdown = true;
            m.height = HitHeight::Throw; m.techable = true; m.throwHold = 0.30f;
            m.meterGain = 10; m.sfx = SfxWeight::Heavy; m.voice = true; m.label = "Vật";
            return m;   // đòn vật không nhân hệ số tầm với
        default:
            break;
    }

    // Áp thông số riêng của nhân vật: tầm với, sức mạnh, tốc độ.
    m.box.x     *= p.reach;
    m.box.width *= p.reach;
    m.damage     = (int)(m.damage * p.power);
    m.startup   *= p.speed;
    m.recovery  *= p.speed;
    m.fxRadius  *= std::sqrt(p.reach);
    return m;
}

// ============================================================================
// Vòng cập nhật chính
// ============================================================================
void Fighter::Update(Arena &arena, const InputState &in, Fighter &opponent, float dt)
{
    opponent_ = &opponent;
    clock_ += dt;
    stateTimer_ += dt;

    UpdateTrails(dt);
    flashTimer_   = std::max(0.0f, flashTimer_   - dt);
    counterFlash_ = std::max(0.0f, counterFlash_ - dt);
    superGlow_    = std::max(0.0f, superGlow_    - dt);

    if (frozen_) {
        // Hết hiệp: không nhận lệnh nữa, nhưng người đang bay vẫn phải rơi
        // xuống đất (ăn đòn kết liễu giữa không trung).
        if (!onGround_) UpdatePhysics(arena, dt);
        UpdateAnimation(dt);
        return;
    }

    ReadInput(in, dt);

    hitstun_   = std::max(0.0f, hitstun_   - dt);
    blockstun_ = std::max(0.0f, blockstun_ - dt);
    invuln_    = std::max(0.0f, invuln_    - dt);

    switch (state_) {
        case FighterState::Hurt:
            if (hitstun_ <= 0.0f) SetState(in_.down ? FighterState::Crouch : FighterState::Idle);
            break;

        case FighterState::Block:
            if (blockstun_ <= 0.0f) {
                const bool stillGuarding = (HoldingBackFrom(opponent) || in_.block) &&
                    (opponent.InStartupOrActive() || arena.ProjectileThreatens(*this));
                if (stillGuarding) crouchBlock_ = in_.down;
                else               SetState(FighterState::Idle);
            }
            break;

        case FighterState::Knockdown:
            if (!IsDefeated() && stateTimer_ > 0.55f) {
                SetState(FighterState::Wakeup);
                PlayAnim(AnimId::Death, PlayMode{0, -1, false, true}, true);
                animator_.SetSpeed(2.4f);
                invuln_ = 0.6f;
            }
            break;

        case FighterState::Wakeup:
            if (animator_.Finished() || stateTimer_ > 0.8f) {
                animator_.SetSpeed(1.0f);
                SetState(FighterState::Idle);
            }
            break;

        case FighterState::Intro:
            if (animator_.Finished()) SetState(FighterState::Idle);
            break;

        case FighterState::Dash:
            if (stateTimer_ > 0.20f) SetState(FighterState::Idle);
            break;

        case FighterState::BackDash:
            if (stateTimer_ > 0.30f && onGround_) SetState(FighterState::Idle);
            break;

        case FighterState::Attack:
            if (throwVictim_) UpdateThrowHold(arena, dt);
            else              UpdateAttack(arena, dt);
            break;

        case FighterState::Idle:
        case FighterState::WalkF:
        case FighterState::WalkB:
        case FighterState::Crouch:
        case FighterState::Jump:
            UpdateNeutral(arena, opponent);
            break;

        default:
            break;
    }

    UpdatePhysics(arena, dt);
    UpdateCharacter(arena, dt);
    UpdateAnimation(dt);
}

void Fighter::UpdateNeutral(Arena &arena, Fighter &opponent)
{
    if (TryStartFromInput(arena, false)) return;
    if (!onGround_) return;   // nhảy kiểu Street Fighter: quỹ đạo cố định

    const int d = RelativeDir();

    // --- Lướt ---------------------------------------------------------------
    if (dashRequested_ == 6) {
        dashRequested_ = 0;
        SetState(FighterState::Dash);
        vel_.x = FacingSign() * def_.walkSpeed * 2.6f;
        arena.Emit({EventType::Dash, arena.IdOf(*this), pos_});
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 10, -FacingSign());
        return;
    }
    if (dashRequested_ == 4) {
        dashRequested_ = 0;
        SetState(FighterState::BackDash);
        vel_ = Vector2{-FacingSign() * def_.walkSpeed * 2.1f, -300.0f};
        onGround_ = false;
        invuln_ = 0.14f;   // lướt lùi có vài frame bất tử, như SF
        arena.Emit({EventType::Dash, arena.IdOf(*this), pos_});
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 8, FacingSign());
        return;
    }

    // --- Nhảy ---------------------------------------------------------------
    if (d >= 7) {
        const float hdir = (d == 9) ? 1.0f : (d == 7) ? -1.0f : 0.0f;
        vel_ = Vector2{hdir * FacingSign() * def_.jumpForward, -def_.jumpSpeed};
        onGround_ = false;
        SetState(FighterState::Jump);
        arena.Emit({EventType::Jump, arena.IdOf(*this), pos_});
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 8, 0.0f);
        return;
    }

    // --- Thủ: giữ lùi khi đối thủ đang vung đòn / có đạn bay tới -------------
    const bool back = (d == 1 || d == 4) || in_.block;
    const bool threatened = (opponent.InStartupOrActive() &&
                             std::fabs(opponent.Position().x - pos_.x) < 300.0f) ||
                            arena.ProjectileThreatens(*this);
    if (back && threatened) {
        SetState(FighterState::Block);
        crouchBlock_ = (d == 1) || in_.down;
        vel_.x = 0.0f;
        return;
    }

    // --- Ngồi ---------------------------------------------------------------
    if (d == 1 || d == 2 || d == 3) {
        SetState(FighterState::Crouch);
        vel_.x = 0.0f;
        return;
    }

    // --- Đi -----------------------------------------------------------------
    if (d == 6) {
        SetState(FighterState::WalkF);
        vel_.x = FacingSign() * def_.walkSpeed;
    } else if (d == 4) {
        SetState(FighterState::WalkB);
        vel_.x = -FacingSign() * def_.backSpeed;
    } else {
        SetState(FighterState::Idle);
        vel_.x = 0.0f;
    }
}

// ============================================================================
// Đòn đánh
// ============================================================================
void Fighter::StartMove(Arena &arena, const MoveDef &m)
{
    move_ = m;
    moveTimer_   = 0.0f;
    hitsLeft_    = std::max(1, m.hits);
    nextHitAt_   = 0.0f;
    consumed_    = false;
    contact_     = false;
    activated_   = false;
    armorUsed_   = false;
    startedAirborne_ = !onGround_;
    spinTimer_   = 0.0f;
    spinFlip_    = false;
    throwVictim_ = nullptr;
    attack_ = ActiveAttack{};

    ConsumeMeter(m.meterCost);

    if (m.setStartVel) {
        vel_ = Vector2{m.startVel.x * FacingSign(), m.startVel.y};
        if (m.startVel.y < 0.0f) onGround_ = false;
    } else if (!m.keepMomentum && onGround_) {
        vel_.x = 0.0f;
    }

    state_ = FighterState::Attack;
    stateTimer_ = 0.0f;

    PlayMode mode;
    mode.from = m.frameFrom;
    mode.to   = m.frameTo;
    mode.loop = false;
    PlayAnim(m.anim, mode, true);

    // Khớp độ dài đoạn animation với frame data để hình và hitbox đi cùng nhau.
    const Animation &a = def_.Anim(m.anim);
    const float clipTime = animator_.ClipLength() / a.fps;
    if (m.Total() > 0.0f && clipTime > 0.0f) animator_.SetSpeed(clipTime / m.Total());

    const int id = arena.IdOf(*this);
    if (m.id == MoveId::Super) {
        superGlow_ = 1.2f;
        arena.Emit({EventType::SuperStart, id, Center(), SfxWeight::Heavy});
        arena.StartSuperFreeze(*this);
    } else if (IsSpecial(m.id)) {
        superGlow_ = 0.25f;
        arena.Emit({EventType::SpecialStart, id, Center(), m.sfx});
    }
    arena.Emit({EventType::MoveStart, id, Center(), m.sfx, m.voice ? 1 : 0});

    OnMoveStart(arena, move_);
}

void Fighter::EndMove()
{
    attack_.active = false;
    animator_.SetSpeed(1.0f);
    throwVictim_ = nullptr;
    if (!onGround_)       SetState(FighterState::Jump);
    else if (in_.down)    SetState(FighterState::Crouch);
    else                  SetState(FighterState::Idle);
}

void Fighter::UpdateAttack(Arena &arena, float dt)
{
    moveTimer_ += dt;
    const MoveDef &m = move_;
    const float activeStart = m.startup;
    const float activeEnd   = m.startup + m.active;

    // Vận tốc lúc bung đòn (lao tới, vọt lên...)
    if (!activated_ && moveTimer_ >= activeStart) {
        activated_ = true;
        if (m.setActiveVel) {
            vel_ = Vector2{m.activeVel.x * FacingSign(), m.activeVel.y};
            if (m.activeVel.y < 0.0f) onGround_ = false;
        }
        // Sprite đã có vệt chém vẽ sẵn thì đòn thường không vẽ chồng thêm.
        const int sf = def_.normals.slashFrom;
        const bool spriteSlash = sf >= 0 && (m.anim == AnimId::Attack1 || m.anim == AnimId::Attack2) &&
                                 m.frameFrom <= sf && (m.frameTo < 0 || m.frameTo >= sf);
        if (m.fx != SlashFx::None && !(spriteSlash && IsNormal(m.id)))
            SpawnTrail(m.fx, m.fxRadius, m.fxColor.a ? m.fxColor : Accent());
        OnMoveActive(arena, m);
    }

    // Xoay người (kiểu Tatsumaki): chỉ đổi hình, không đổi hướng logic.
    if (m.spin && moveTimer_ >= activeStart && moveTimer_ < activeEnd) {
        spinTimer_ += dt;
        if (spinTimer_ > 0.06f) { spinTimer_ = 0.0f; spinFlip_ = !spinFlip_; }
    } else {
        spinFlip_ = false;
    }

    const bool inActive = moveTimer_ >= activeStart && moveTimer_ < activeEnd;
    attack_.active = inActive && CanAttackHit() && moveTimer_ >= nextHitAt_ &&
                     m.box.width > 0.0f;
    attack_.def   = &move_;
    attack_.world = WorldBox(m.box);

    OnMoveUpdate(arena, m, moveTimer_, dt);

    // Thử hủy đòn khi đang trúng - nền tảng của combo.
    if (contact_ && moveTimer_ >= activeStart &&
        moveTimer_ < activeEnd + m.recovery * 0.8f &&
        (m.chainable || m.specialCancel || m.superCancel)) {
        if (TryStartFromInput(arena, true)) return;
    }

    if (moveTimer_ >= m.Total() && onGround_) EndMove();
    else if (moveTimer_ >= m.Total() && !onGround_ && !m.noGravity) {
        // Hết đòn trên không: rơi tự do, chạm đất mới hồi.
        attack_.active = false;
    }
}

bool Fighter::CanAttackHit() const
{
    return hitsLeft_ > 0 && !consumed_;
}

void Fighter::NotifyAttackLanded(Arena &arena, Fighter &target, bool blocked, int damage)
{
    --hitsLeft_;
    if (hitsLeft_ <= 0) consumed_ = true;
    else                nextHitAt_ = moveTimer_ + move_.hitSpacing;

    contact_ = true;
    attack_.active = false;

    AddMeter(blocked ? move_.meterGain * 0.5f : move_.meterGain);

    if (!blocked) {
        comboCount_  += 1;
        comboDamage_ += damage;
        comboTimer_   = 1.0f;
    }
    OnMoveHit(arena, move_, target, blocked);
}

bool Fighter::InHitstun() const
{
    return state_ == FighterState::Hurt || state_ == FighterState::AirHurt ||
           state_ == FighterState::Thrown ||
           (state_ == FighterState::Knockdown && stateTimer_ < 0.05f);
}

void Fighter::NotifyProjectileCombo(int damage)
{
    comboCount_  += 1;
    comboDamage_ += damage;
    comboTimer_   = 1.0f;
}

void Fighter::EndCombo()
{
    comboCount_  = 0;
    comboDamage_ = 0;
}

// ============================================================================
// Vật
// ============================================================================
bool Fighter::CanBeThrown() const
{
    if (!onGround_ || IsInvulnerable()) return false;
    switch (state_) {
        case FighterState::Hurt:
        case FighterState::AirHurt:
        case FighterState::Thrown:
        case FighterState::Knockdown:
        case FighterState::Wakeup:
            return false;
        case FighterState::Block:
            return blockstun_ <= 0.0f;
        default:
            return true;
    }
}

void Fighter::BeginThrow(Arena &arena, Fighter &victim)
{
    throwVictim_ = &victim;
    throwTimer_  = move_.throwHold;
    // Giữ lùi lúc tóm = quăng ra sau lưng (đổi bên).
    throwBehind_ = HoldingBackFrom(victim);
    attack_.active = false;
    consumed_ = true;
    // Nhấc đối thủ: chỉ phát phần vung tay, bỏ các frame có vệt chém vẽ sẵn.
    const int sf = def_.normals.slashFrom;
    PlayAnim(def_.normals.heavyAnim, PlayMode{0, sf > 0 ? sf - 1 : -1, false, false}, true);
    animator_.SetSpeed((float)animator_.ClipLength() / def_.Anim(def_.normals.heavyAnim).fps /
                       (move_.throwHold + 0.1f));
    victim.BeginThrown(*this);
    arena.Emit({EventType::Throw, arena.IdOf(*this), victim.Center(), SfxWeight::Medium, 0});
}

void Fighter::BeginThrown(Fighter &thrower)
{
    (void)thrower;
    SetState(FighterState::Thrown);
    attack_.active = false;
    vel_ = Vector2{0.0f, 0.0f};
    PlayAnim(AnimId::TakeHit, PlayMode{0, -1, false, false}, true);
}

void Fighter::UpdateThrowHold(Arena &arena, float dt)
{
    throwTimer_ -= dt;
    Fighter &v = *throwVictim_;
    const float k = 1.0f - std::max(0.0f, throwTimer_) / std::max(0.01f, move_.throwHold);

    // Nạn nhân bị nhấc lên trước mặt; đòn vật lệnh (slam) nhấc cao hơn hẳn.
    const float lift = move_.slam ? std::sin(k * PI) * 150.0f : 30.0f + 20.0f * k;
    const float side = throwBehind_ ? (1.0f - 2.0f * k) : 1.0f;
    v.SetPosition(Vector2{pos_.x + FacingSign() * 62.0f * side, kGroundY - lift});

    if (throwTimer_ <= 0.0f) {
        v.ReleaseFromThrow(arena, *this, move_, throwBehind_);
        throwVictim_ = nullptr;
        moveTimer_ = move_.startup + move_.active;   // tiếp tục phần thu đòn
        if (throwBehind_) facingRight_ = !facingRight_;
    }
}

void Fighter::ReleaseFromThrow(Arena &arena, Fighter &thrower, const MoveDef &move, bool behind)
{
    const float dir = thrower.FacingSign() * (behind ? -1.0f : 1.0f);
    const int dmg = std::max(1, (int)(move.damage * def_.defenseScale * incomingScale_));
    health_ = std::max(0, health_ - dmg);
    flashTimer_ = 0.16f;

    if (move.slam) {
        // Nện thẳng xuống đất
        pos_.y = kGroundY - 4.0f;
        vel_ = Vector2{dir * 120.0f, -220.0f};
        arena.Shake(18.0f);
        arena.HitStop(0.12f);
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 30, 0.0f);
        arena.Fx().Ring(Vector2{pos_.x, kGroundY - 10.0f}, Color{255, 230, 180, 220}, 40.0f);
    } else {
        vel_ = Vector2{dir * move.knockback, -520.0f};
        arena.Shake(9.0f);
        arena.HitStop(0.06f);
    }
    onGround_ = false;
    pendingKnockdown_ = true;
    airHitsLeft_ = 0;
    SetState(FighterState::AirHurt);
    thrower.comboCount_ += 1;
    thrower.comboDamage_ += dmg;
    thrower.AddMeter(move.meterGain);
    arena.Emit({EventType::Hit, arena.IdOf(thrower), Center(), SfxWeight::Heavy, dmg});
}

void Fighter::BreakThrow(float pushDir)
{
    throwVictim_ = nullptr;
    attack_.active = false;
    SetState(FighterState::Hurt);
    hitstun_ = 0.25f;
    vel_ = Vector2{pushDir * 380.0f, 0.0f};
    PlayAnim(AnimId::TakeHit, PlayMode{0, -1, false, false}, true);
}

// ============================================================================
// Đỡ / nhận đòn
// ============================================================================
bool Fighter::HoldingBackFrom(const Fighter &other) const
{
    return (other.Position().x > pos_.x) ? in_.left : in_.right;
}

bool Fighter::WantsToBlock(const Fighter &attacker) const
{
    return WantsToBlockFrom(attacker.Position().x);
}

bool Fighter::WantsToBlockFrom(float sourceX) const
{
    if (!onGround_) return false;          // không đỡ trên không
    const bool holdingBack = (sourceX > pos_.x) ? in_.left : in_.right;
    switch (state_) {
        case FighterState::Idle:
        case FighterState::WalkB:
        case FighterState::WalkF:
        case FighterState::Crouch:
        case FighterState::Block:
            return holdingBack || in_.block;
        default:
            return false;
    }
}

void Fighter::WhiffCurrentHit()
{
    consumed_ = true;
    attack_.active = false;
}

bool Fighter::CanBlockHeight(HitHeight h) const
{
    const bool crouching = in_.down;
    switch (h) {
        case HitHeight::Low:      return crouching;
        case HitHeight::Overhead: return !crouching;
        case HitHeight::Throw:    return false;
        default:                  return true;
    }
}

int Fighter::ReceiveHit(Arena &arena, Fighter &attacker, const MoveDef &move,
                        bool blocked, bool counter)
{
    (void)arena;   // hiệu ứng/âm thanh do Arena phát qua hàng đợi sự kiện
    const float away = (pos_.x < attacker.Position().x) ? -1.0f : 1.0f;

    if (blocked) {
        // Đỡ được: mất chút máu nếu là chiêu đặc biệt (chip), bị đẩy lùi.
        // Chiêu đặc biệt và siêu chiêu mặc định trừ 1/6 sát thương dù bị đỡ.
        int chipBase = move.chip;
        if (chipBase == 0 && (IsSpecial(move.id) || move.id == MoveId::Super)) chipBase = move.damage / 6;
        const int chip = (int)(chipBase * def_.defenseScale);
        health_ = std::max(0, health_ - chip);
        blockstun_ = move.blockstun;
        crouchBlock_ = in_.down;
        SetState(FighterState::Block);
        vel_.x = away * move.knockback * 0.7f;
        AddMeter(3.0f);
        if (health_ <= 0) {
            // Chết vì chip - vẫn phải ngã xuống.
            SetState(FighterState::AirHurt);
            vel_ = Vector2{away * 200.0f, -400.0f};
            onGround_ = false;
            pendingKnockdown_ = true;
        }
        return chip;
    }

    // Armor: chịu đòn trong lúc vung đòn nặng, vẫn mất máu nhưng không khựng.
    const bool armored = state_ == FighterState::Attack && move_.armor && !armorUsed_ &&
                         moveTimer_ < move_.startup + move_.active &&
                         move.height != HitHeight::Throw && move.meterCost <= 0.0f;

    // Hệ số combo: đòn thứ 3 trở đi giảm dần, siêu chiêu tối thiểu 50%.
    const int n = attacker.ComboCount();
    float scale = (n < 2) ? 1.0f : std::max(0.3f, 1.0f - 0.1f * (float)(n));
    if (move.meterCost > 0.0f) scale = std::max(scale, 0.5f);
    if (counter) scale *= 1.2f;

    int dmg = (int)(move.damage * scale * def_.defenseScale * incomingScale_);
    dmg = std::max(1, dmg);
    health_ = std::max(0, health_ - dmg);
    AddMeter(dmg * 0.04f);
    flashTimer_ = 0.14f;
    if (counter) counterFlash_ = 0.3f;

    if (armored && health_ > 0) {
        armorUsed_ = true;
        return dmg;
    }

    // Bị trúng khi đang vung đòn thì mất đòn đó.
    attack_.active = false;
    if (throwVictim_) {
        throwVictim_->BreakThrow(FacingSign());
        throwVictim_ = nullptr;
    }
    animator_.SetSpeed(1.0f);

    hitstun_ = move.hitstun * (counter ? 1.35f : 1.0f);
    const bool wasAirHurt = state_ == FighterState::AirHurt;

    if (!onGround_ || move.launcher || move.knockdown || health_ <= 0) {
        if (wasAirHurt) --airHitsLeft_;
        else            airHitsLeft_ = 0;
        if (move.launcher) airHitsLeft_ = std::max(airHitsLeft_, 3);

        SetState(FighterState::AirHurt);
        onGround_ = false;
        const float vy = move.launcher ? move.launchVy
                       : (wasAirHurt ? std::min(vel_.y, -360.0f) : -440.0f);
        vel_ = Vector2{away * move.knockback * 0.6f, vy};
        // Mọi đòn trúng trên không đều khiến người nhận ngã khi tiếp đất.
        pendingKnockdown_ = true;
        PlayAnim(AnimId::TakeHit, PlayMode{0, -1, false, false}, true);
    } else {
        crouchHurt_ = in_.down && state_ != FighterState::Attack;
        SetState(FighterState::Hurt);
        vel_.x = away * move.knockback;
        PlayAnim(AnimId::TakeHit, PlayMode{0, -1, false, false}, true);
    }
    return dmg;
}

// ============================================================================
// Vật lý
// ============================================================================
void Fighter::UpdatePhysics(Arena &arena, float dt)
{
    if (state_ == FighterState::Thrown) return;

    const bool floating = state_ == FighterState::Attack && move_.noGravity &&
                          moveTimer_ < move_.Total();
    if (!onGround_ && !floating) vel_.y += kGravity * dt;
    if (floating) vel_.y = Approach(vel_.y, 0.0f, 10.0f, dt);

    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;

    if (pos_.y >= kGroundY) {
        pos_.y = kGroundY;
        if (!onGround_) {
            onGround_ = true;
            vel_.y = 0.0f;
            const int id = arena.IdOf(*this);

            switch (state_) {
                case FighterState::Jump:
                case FighterState::BackDash:
                    vel_.x = 0.0f;
                    SetState(FighterState::Idle);
                    arena.Emit({EventType::Land, id, pos_});
                    arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 6, 0.0f);
                    break;

                case FighterState::Attack:
                    if (startedAirborne_ || move_.landCancel || moveTimer_ >= move_.Total()) {
                        vel_.x = 0.0f;
                        OnMoveLand(arena, move_);
                        EndMove();
                        arena.Emit({EventType::Land, id, pos_});
                        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 6, 0.0f);
                    }
                    break;

                case FighterState::AirHurt:
                    if (pendingKnockdown_ || health_ <= 0) {
                        SetState(FighterState::Knockdown);
                        PlayAnim(AnimId::Death, PlayMode{0, -1, false, false}, true);
                        animator_.SetSpeed(health_ <= 0 ? 1.0f : 1.8f);
                        vel_.x *= 0.4f;
                        arena.Emit({EventType::Knockdown, id, pos_});
                        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 18, 0.0f);
                        arena.Shake(5.0f);
                    } else {
                        SetState(FighterState::Idle);
                    }
                    pendingKnockdown_ = false;
                    tilt_ = 0.0f;
                    break;

                default:
                    break;
            }
        }
    } else {
        onGround_ = false;
    }

    // Ma sát mặt đất cho mọi trạng thái không tự đi.
    if (onGround_) {
        switch (state_) {
            case FighterState::WalkF:
            case FighterState::WalkB:
                break;
            case FighterState::Dash:
                vel_.x = Approach(vel_.x, 0.0f, 6.0f, dt);
                break;
            case FighterState::Attack:
                if (!move_.setStartVel && !move_.setActiveVel) vel_.x = Approach(vel_.x, 0.0f, 14.0f, dt);
                else if (moveTimer_ > move_.startup + move_.active) vel_.x = Approach(vel_.x, 0.0f, 10.0f, dt);
                break;
            default:
                vel_.x = Approach(vel_.x, 0.0f, 11.0f, dt);
                break;
        }
    }
}

// ============================================================================
// Trạng thái & truy vấn
// ============================================================================
void Fighter::SetState(FighterState s)
{
    if (state_ == s) return;
    state_ = s;
    stateTimer_ = 0.0f;
    if (s != FighterState::Attack) {
        attack_.active = false;
        spinFlip_ = false;
        if (s != FighterState::Wakeup && s != FighterState::Knockdown) animator_.SetSpeed(1.0f);
    }
}

bool Fighter::IsBusy() const
{
    switch (state_) {
        case FighterState::Idle:
        case FighterState::WalkF:
        case FighterState::WalkB:
        case FighterState::Crouch:
            return frozen_;
        default:
            return true;
    }
}

bool Fighter::InStartupOrActive() const
{
    return state_ == FighterState::Attack && !throwVictim_ &&
           moveTimer_ < move_.startup + move_.active;
}

bool Fighter::IsInvulnerable() const
{
    if (invuln_ > 0.0f) return true;
    if (state_ == FighterState::Knockdown || state_ == FighterState::Wakeup) return true;
    if (state_ == FighterState::Thrown) return true;
    if (state_ == FighterState::AirHurt && airHitsLeft_ <= 0 && stateTimer_ > 0.02f) return true;
    if (state_ == FighterState::Attack &&
        moveTimer_ >= move_.invulnFrom && moveTimer_ < move_.invulnTo) return true;
    return frozen_;
}

bool Fighter::PassesThrough() const
{
    return state_ == FighterState::Attack && move_.passThrough &&
           moveTimer_ < move_.startup + move_.active;
}

bool Fighter::IsProjInvulnerable() const
{
    return IsInvulnerable() || (state_ == FighterState::Attack && move_.projInvuln &&
                                moveTimer_ < move_.startup + move_.active);
}

bool Fighter::IsCrouching() const
{
    switch (state_) {
        case FighterState::Crouch: return true;
        case FighterState::Block:  return crouchBlock_;
        case FighterState::Hurt:   return crouchHurt_;
        case FighterState::Attack: return move_.pose == Pose::Crouch;
        default:                   return false;
    }
}

bool Fighter::CurrentAttackIsLow() const
{
    return InStartupOrActive() && move_.height == HitHeight::Low;
}

void Fighter::PlayIntro()
{
    SetState(FighterState::Intro);
    PlayAnim(def_.normals.heavyAnim, PlayMode{0, -1, false, false}, true);
    animator_.SetSpeed(0.55f);
}

void Fighter::SetOutcome(bool won)
{
    frozen_ = true;
    attack_.active = false;
    throwVictim_ = nullptr;
    vel_.x = 0.0f;
    EndCombo();
    if (won) {
        state_ = FighterState::Victory;
        PlayAnim(def_.normals.heavyAnim, PlayMode{0, -1, false, false}, true);
        animator_.SetSpeed(0.5f);
    } else if (state_ != FighterState::AirHurt && state_ != FighterState::Knockdown) {
        // Đang bay thì để UpdatePhysics tự chuyển sang nằm khi chạm đất.
        state_ = FighterState::Defeat;
        PlayAnim(AnimId::Death, PlayMode{0, -1, false, false}, true);
    }
}

void Fighter::FreezeForRoundEnd()
{
    frozen_ = true;
    attack_.active = false;
    throwVictim_ = nullptr;
    vel_.x = 0.0f;
    EndCombo();
    if (state_ == FighterState::Attack || state_ == FighterState::Block) {
        state_ = FighterState::Idle;
        PlayLoop(AnimId::Idle);
    }
}

void Fighter::AddMeter(float amount)
{
    meter_ = std::clamp(meter_ + amount * def_.meterRate * meterScale_, 0.0f, kMaxMeterValue);
}

void Fighter::ConsumeMeter(float amount)
{
    meter_ = std::clamp(meter_ - amount, 0.0f, kMaxMeterValue);
}

void Fighter::ApplyKnockback(float vx, float vy)
{
    vel_.x += vx;
    vel_.y += vy;
    if (vy < 0.0f) onGround_ = false;
}

void Fighter::FaceTowards(float x)
{
    facingRight_ = x >= pos_.x;
}

Vector2 Fighter::Center() const
{
    const float h = IsCrouching() ? kCrouchHeight : kBodyHeight;
    return Vector2{pos_.x, pos_.y - h * 0.55f};
}

Rectangle Fighter::Hurtbox() const
{
    float h = kBodyHeight;
    if (IsCrouching()) h = kCrouchHeight;
    else if (!onGround_) h = kAirHeight;
    float top = pos_.y - h;
    if (!onGround_) top = pos_.y - h - 20.0f;   // co chân khi nhảy
    if (state_ == FighterState::Knockdown) { h = 40.0f; top = pos_.y - h; }
    return Rectangle{pos_.x - kBodyWidth * 0.5f, top, kBodyWidth, h};
}

Rectangle Fighter::WorldBox(const Rectangle &local) const
{
    if (facingRight_) {
        return Rectangle{pos_.x + local.x, pos_.y + local.y, local.width, local.height};
    }
    return Rectangle{pos_.x - local.x - local.width, pos_.y + local.y, local.width, local.height};
}

Color Fighter::Accent() const { return def_.accent; }

// ============================================================================
// Animation
// ============================================================================
void Fighter::PlayAnim(AnimId id, const PlayMode &mode, bool restart)
{
    animator_.Play(&def_.Anim(id), mode, restart);
}

void Fighter::PlayLoop(AnimId id)
{
    PlayMode m;
    m.loop = true;
    animator_.Play(&def_.Anim(id), m, false);
}

void Fighter::UpdateAnimation(float dt)
{
    switch (state_) {
        case FighterState::Idle:
        case FighterState::Crouch:
            PlayLoop(AnimId::Idle);
            animator_.SetSpeed(1.0f);
            break;
        case FighterState::WalkF:
            PlayLoop(AnimId::Run);
            animator_.SetSpeed(0.8f);
            break;
        case FighterState::WalkB: {
            // Lùi = chạy phát ngược, chậm lại: chân bước lùi khớp với hướng di
            // chuyển trong khi mặt vẫn nhìn đối thủ - đúng kiểu Street Fighter.
            PlayMode m; m.loop = true; m.reverse = true;
            animator_.Play(&def_.Anim(AnimId::Run), m, false);
            animator_.SetSpeed(0.62f);
            break;
        }
        case FighterState::Dash: {
            PlayLoop(AnimId::Run);
            animator_.SetSpeed(1.7f);
            break;
        }
        case FighterState::BackDash: {
            PlayMode m; m.loop = false;
            animator_.Play(&def_.Anim(vel_.y < 0 ? AnimId::Jump : AnimId::Fall), m, false);
            break;
        }
        case FighterState::Jump: {
            PlayMode m; m.loop = false;
            animator_.Play(&def_.Anim(vel_.y < 0.0f ? AnimId::Jump : AnimId::Fall), m, false);
            break;
        }
        case FighterState::Block:
            // Không có sprite đỡ đòn: dừng ở frame idle đầu + lá chắn vẽ thêm.
            PlayLoop(AnimId::Idle);
            animator_.SetSpeed(0.0f);
            break;
        case FighterState::AirHurt:
            tilt_ = std::clamp(vel_.y / 26.0f, -28.0f, 32.0f) * -FacingSign();
            break;
        case FighterState::Victory:
            if (animator_.Finished()) {
                PlayLoop(AnimId::Idle);
                animator_.SetSpeed(1.0f);
            }
            break;
        default:
            break;
    }
    if (state_ != FighterState::AirHurt) tilt_ = Approach(tilt_, 0.0f, 18.0f, dt);

    squash_ = Approach(squash_, IsCrouching() ? kCrouchSquash : 1.0f, 28.0f, dt);
    animator_.Update(dt);
}

// ============================================================================
// Vệt chém
// ============================================================================
void Fighter::SpawnTrail(SlashFx fx, float radius, Color color)
{
    SlashTrail t;
    t.radius = radius;
    t.color = color;
    t.facingRight = facingRight_;
    t.maxLife = t.life = 0.22f;

    switch (fx) {
        case SlashFx::Jab:        t.offset = {40, -120}; t.thick = 12; t.a0 = -25;  t.a1 = 20;  t.radius *= 0.6f; t.maxLife = t.life = 0.14f; break;
        case SlashFx::Horizontal: t.offset = {10, -112}; t.thick = 22; t.a0 = -70;  t.a1 = 35;  break;
        case SlashFx::Overhead:   t.offset = {0,  -100}; t.thick = 32; t.a0 = -120; t.a1 = 65;  t.maxLife = t.life = 0.28f; break;
        case SlashFx::Low:        t.offset = {10, -48};  t.thick = 16; t.a0 = -35;  t.a1 = 25;  break;
        case SlashFx::Sweep:      t.offset = {0,  -22};  t.thick = 20; t.a0 = -25;  t.a1 = 18;  t.maxLife = t.life = 0.26f; break;
        case SlashFx::Rising:     t.offset = {10, -70};  t.thick = 30; t.a0 = -110; t.a1 = 80;  t.maxLife = t.life = 0.34f; break;
        case SlashFx::AirDiag:    t.offset = {10, -95};  t.thick = 22; t.a0 = -50;  t.a1 = 80;  break;
        case SlashFx::Spin:       t.offset = {0,  -100}; t.thick = 22; t.a0 = 0;    t.a1 = 360; t.maxLife = t.life = 0.3f; break;
        case SlashFx::Thrust:     t.offset = {30, -112}; t.thick = 10; t.a0 = 0;    t.a1 = 0;   break;
        default: return;
    }
    trails_.push_back(t);
}

void Fighter::UpdateTrails(float dt)
{
    for (auto &t : trails_) t.life -= dt;
    trails_.erase(std::remove_if(trails_.begin(), trails_.end(),
                                 [](const SlashTrail &t) { return t.life <= 0.0f; }),
                  trails_.end());
}

void Fighter::DrawTrails() const
{
    for (const auto &t : trails_) {
        const float k    = 1.0f - t.life / t.maxLife;       // 0 -> 1
        const float grow = std::min(1.0f, k / 0.35f);       // vệt quét ra
        const float fade = (k < 0.35f) ? 1.0f : 1.0f - (k - 0.35f) / 0.65f;

        const float sx = t.facingRight ? 1.0f : -1.0f;
        const Vector2 c{pos_.x + t.offset.x * sx, pos_.y + t.offset.y * squash_};

        if (t.a0 == t.a1) {
            // Đâm thẳng: vài vệt ngang
            for (int i = 0; i < 3; ++i) {
                const float y = c.y + (i - 1) * 9.0f;
                const float len = t.radius * grow * (1.0f - i * 0.2f);
                Color col = t.color; col.a = (unsigned char)(200 * fade);
                DrawLineEx(Vector2{c.x, y}, Vector2{c.x + sx * len, y}, t.thick * (1.0f - 0.3f * i), col);
            }
            continue;
        }

        float a0 = t.a0, a1 = t.a0 + (t.a1 - t.a0) * grow;
        if (!t.facingRight) { const float b0 = 180.0f - a1, b1 = 180.0f - a0; a0 = b0; a1 = b1; }
        if (a1 < a0) std::swap(a0, a1);

        // Lưỡi liềm: dày ở giữa, thon nhọn hai đầu - giống vệt kiếm vẽ tay.
        auto crescent = [&](float rOuter, float thick, Color col) {
            const int segs = 28;
            for (int i = 0; i < segs; ++i) {
                const float u0 = (float)i / segs, u1 = (float)(i + 1) / segs;
                const float b0 = (a0 + (a1 - a0) * u0) * DEG2RAD, b1 = (a0 + (a1 - a0) * u1) * DEG2RAD;
                const float w0 = thick * std::sin(PI * u0), w1 = thick * std::sin(PI * u1);
                const Vector2 o0{c.x + std::cos(b0) * rOuter, c.y + std::sin(b0) * rOuter};
                const Vector2 o1{c.x + std::cos(b1) * rOuter, c.y + std::sin(b1) * rOuter};
                const Vector2 i0{c.x + std::cos(b0) * (rOuter - w0), c.y + std::sin(b0) * (rOuter - w0)};
                const Vector2 i1{c.x + std::cos(b1) * (rOuter - w1), c.y + std::sin(b1) * (rOuter - w1)};
                // Vẽ cả hai chiều quấn để không bị loại mặt sau.
                DrawTriangle(o0, i0, i1, col); DrawTriangle(o0, i1, i0, col);
                DrawTriangle(o0, i1, o1, col); DrawTriangle(o0, o1, i1, col);
            }
        };
        const float r = t.radius;
        Color glow = t.color; glow.a = (unsigned char)(70 * fade);
        Color body = t.color; body.a = (unsigned char)(190 * fade);
        Color core{255, 250, 235, (unsigned char)(200 * fade)};
        crescent(r + 8.0f, t.thick * 1.8f, glow);
        crescent(r, t.thick, body);
        crescent(r - 2.0f, t.thick * 0.32f, core);
    }
}

// ============================================================================
// Vẽ
// ============================================================================
void Fighter::DrawSpriteFrame(const Animation &a, int frame, Vector2 pos, bool faceRight,
                              Color tint, float squash) const
{
    if (a.texture.id == 0) return;
    const float side  = (float)a.texture.height;
    const float scale = def_.scale;
    const float sx    = scale * (squash < 0.99f ? 1.06f : 1.0f);
    const float sy    = scale * squash;

    Rectangle src = a.FrameRect(frame);
    if (!faceRight) src.width = -src.width;

    const float anchorX = faceRight ? def_.anchor.x : (side - def_.anchor.x);

    // Xoay quanh thân (không phải bàn chân) để cú tung hứng trông như lộn nhào.
    const float pivotUp = 90.0f;
    Rectangle dest{pos.x, pos.y - pivotUp, side * sx, side * sy};
    Vector2 origin{anchorX * sx, def_.anchor.y * sy - pivotUp};
    DrawTexturePro(a.texture, src, dest, origin, tilt_, tint);
}

void Fighter::DrawShadow() const
{
    const float height = std::max(0.0f, kGroundY - pos_.y);
    const float t      = std::clamp(1.0f - height / 420.0f, 0.25f, 1.0f);
    DrawEllipse((int)pos_.x, (int)(kGroundY + 6.0f),
                48.0f * t, 12.0f * t, Color{0, 0, 0, (unsigned char)(120 * t)});
}

void Fighter::Draw(bool debugBoxes) const
{
    const Animation *anim = animator_.Current();
    if (!anim) return;

    DrawBehind();

    const bool faceVisual = spinFlip_ ? !facingRight_ : facingRight_;

    Color tint = WHITE;
    if (flashTimer_ > 0.0f) {
        const float k = flashTimer_ / 0.14f;
        tint = Color{255, (unsigned char)(255 - 140 * k), (unsigned char)(255 - 140 * k), 255};
    }
    if (counterFlash_ > 0.0f && std::fmod(counterFlash_, 0.08f) < 0.04f) tint = Color{255, 230, 120, 255};
    if (state_ == FighterState::Block) tint = Color{196, 216, 255, 255};
    if (state_ == FighterState::Wakeup || invuln_ > 0.0f) {
        tint.a = (std::fmod(clock_, 0.1f) < 0.05f) ? 170 : 255;
    }

    DrawSpriteFrame(*anim, animator_.Frame(), pos_, faceVisual, tint, squash_);

    // Hào quang lúc tung chiêu: vẽ chồng một lớp cộng sáng màu nhân vật.
    if (superGlow_ > 0.0f) {
        Color g = Accent();
        g.a = (unsigned char)(std::min(1.0f, superGlow_) * (110 + 60 * std::sin(clock_ * 30.0f)));
        BeginBlendMode(BLEND_ADDITIVE);
        DrawSpriteFrame(*anim, animator_.Frame(), pos_, faceVisual, g, squash_);
        EndBlendMode();
    }

    // Lá chắn khi đỡ
    if (state_ == FighterState::Block) {
        const Vector2 c = Center();
        const float cx = c.x + FacingSign() * 38.0f;
        const float k = (blockstun_ > 0.0f) ? 1.0f : 0.55f;
        const float a0 = facingRight_ ? -65.0f : 115.0f;
        const float a1 = facingRight_ ?  65.0f : 245.0f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRing(Vector2{cx - FacingSign() * 30.0f, c.y}, 58.0f, 66.0f, a0, a1, 24,
                 Color{110, 170, 255, (unsigned char)(150 * k)});
        DrawRing(Vector2{cx - FacingSign() * 30.0f, c.y}, 50.0f, 58.0f, a0, a1, 24,
                 Color{200, 230, 255, (unsigned char)(80 * k)});
        EndBlendMode();
    }

    DrawFront();

    if (debugBoxes) {
        DrawRectangleLinesEx(Hurtbox(), 1.0f,
                             IsInvulnerable() ? Color{255, 255, 255, 160} : Color{80, 200, 255, 200});
        if (attack_.active) DrawRectangleLinesEx(attack_.world, 2.0f, Color{255, 70, 70, 230});
    }
}

} // namespace fighter
