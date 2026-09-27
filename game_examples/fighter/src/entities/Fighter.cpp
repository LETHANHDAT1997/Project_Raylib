#include "entities/Fighter.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

namespace {

// Kích thước hurtbox quy ra pixel màn hình. Sprite của mỗi pack có tỉ lệ khác
// nhau nên ta dùng một khối va chạm thống nhất cho công bằng, thay vì bám theo
// pixel của ảnh.
constexpr float kBodyWidth       = 72.0f;
constexpr float kBodyHeight      = 168.0f;
constexpr float kCrouchHeight    = 104.0f;
constexpr float kAirControl      = 0.55f;
constexpr float kKnockdownThresh = 170.0f;   // lực đẩy đủ mạnh thì ngã hẳn

} // namespace

Fighter::Fighter(const CharacterDef &def, bool facingRight)
    : def_(def)
{
    maxHealth_ = def.maxHealth;
    health_    = maxHealth_;
    facingRight_ = facingRight;
    pos_ = Vector2{0.0f, kGroundY};
    animator_.Play(&def_.Anim(AnimId::Idle), true);
}

void Fighter::Reset(float x, bool facingRight)
{
    pos_ = Vector2{x, kGroundY};
    vel_ = Vector2{0.0f, 0.0f};
    facingRight_ = facingRight;
    onGround_    = true;
    health_      = maxHealth_;
    meter_       = 0.0f;
    state_       = FighterState::Idle;
    stateTimer_  = 0.0f;
    moveTimer_   = 0.0f;
    hitstun_ = blockstun_ = specialCd_ = superCd_ = armorLeft_ = 0.0f;
    comboCount_  = 0;
    comboTimer_  = 0.0f;
    flashTimer_  = 0.0f;
    dustTimer_   = 0.0f;
    frozen_      = false;
    attack_ = ActiveAttack{};
    animator_.SetSpeed(1.0f);
    animator_.Play(&def_.Anim(AnimId::Idle), true);
}

// ---------------------------------------------------------------------------
// Vòng cập nhật chính
// ---------------------------------------------------------------------------
void Fighter::Update(Arena &arena, const InputState &in, Fighter &opponent, float dt)
{
    stateTimer_ += dt;

    if (frozen_) {          // hết hiệp: chỉ còn chạy animation ăn mừng / gục
        UpdateAnimation(dt);
        return;
    }

    hitstun_   = std::max(0.0f, hitstun_   - dt);
    blockstun_ = std::max(0.0f, blockstun_ - dt);
    specialCd_ = std::max(0.0f, specialCd_ - dt);
    superCd_   = std::max(0.0f, superCd_   - dt);
    armorLeft_ = std::max(0.0f, armorLeft_ - dt);
    flashTimer_= std::max(0.0f, flashTimer_- dt);

    if (comboTimer_ > 0.0f) {
        comboTimer_ -= dt;
        if (comboTimer_ <= 0.0f) comboCount_ = 0;
    }

    // Thoát khỏi trạng thái dính đòn khi hết hitstun và đã chạm đất.
    if (state_ == FighterState::Hurt && hitstun_ <= 0.0f && onGround_) {
        SetState(FighterState::Idle);
    }
    if (state_ == FighterState::Knockdown && animator_.Finished() && hitstun_ <= 0.0f) {
        SetState(FighterState::Idle);
    }

    if (state_ == FighterState::Attack) UpdateAttack(arena, dt);
    else                                attack_.active = false;

    UpdateControl(arena, in, opponent, dt);
    UpdatePhysics(dt);
    UpdateCharacter(arena, dt);
    UpdateAnimation(dt);
}

bool Fighter::CanAct() const
{
    if (frozen_) return false;
    switch (state_) {
        case FighterState::Attack:
        case FighterState::Hurt:
        case FighterState::Knockdown:
        case FighterState::Victory:
        case FighterState::Defeat:
            return false;
        default:
            return hitstun_ <= 0.0f && blockstun_ <= 0.0f;
    }
}

bool Fighter::IsBusy() const
{
    return !CanAct();
}

void Fighter::UpdateControl(Arena &arena, const InputState &in, Fighter &opponent, float dt)
{
    (void)opponent;
    (void)dt;

    if (!CanAct()) return;

    // --- Đòn đánh ---------------------------------------------------------
    if (in.superPressed && superCd_ <= 0.0f && meter_ >= Move(MoveSlot::Super).meterCost) {
        StartMove(MoveSlot::Super);
        return;
    }
    if (in.specialPressed && specialCd_ <= 0.0f) {
        StartMove(MoveSlot::Special);
        return;
    }
    if (in.heavyPressed) { StartMove(MoveSlot::Heavy); return; }
    if (in.lightPressed) { StartMove(MoveSlot::Light); return; }

    // --- Đỡ đòn -----------------------------------------------------------
    // Đỡ được khi đứng dưới đất và giữ phím đỡ (hoặc lùi về phía sau).
    const bool holdingBack = (facingRight_ && in.left) || (!facingRight_ && in.right);
    if (onGround_ && (in.block || (holdingBack && in.down))) {
        SetState(FighterState::Block);
        vel_.x = 0.0f;
        return;
    }

    // --- Nhảy -------------------------------------------------------------
    if (in.jumpPressed && onGround_) {
        vel_.y = -def_.jumpSpeed;
        onGround_ = false;
        SetState(FighterState::Jump);
        arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 8, 0.0f);
    }

    // --- Ngồi -------------------------------------------------------------
    if (onGround_ && in.down && !in.left && !in.right) {
        SetState(FighterState::Crouch);
        vel_.x = 0.0f;
        return;
    }

    // --- Di chuyển --------------------------------------------------------
    float dir = 0.0f;
    if (in.right) dir += 1.0f;
    if (in.left)  dir -= 1.0f;

    if (dir != 0.0f) {
        const bool forward = (dir > 0.0f) == facingRight_;
        float speed = forward ? def_.walkSpeed : def_.backSpeed;
        if (!onGround_) speed *= kAirControl;
        vel_.x = dir * speed;
        if (onGround_) SetState(FighterState::Walk);
    } else {
        vel_.x = 0.0f;
        if (onGround_ && state_ != FighterState::Crouch) SetState(FighterState::Idle);
    }

    // Bụi bốc lên khi chạy
    if (onGround_ && std::fabs(vel_.x) > 10.0f) {
        dustTimer_ -= dt;
        if (dustTimer_ <= 0.0f) {
            arena.Fx().Dust(Vector2{pos_.x, kGroundY}, 2, vel_.x > 0 ? -1.0f : 1.0f);
            dustTimer_ = 0.09f;
        }
    }
}

void Fighter::UpdatePhysics(float dt)
{
    if (!onGround_) vel_.y += kGravity * dt;

    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;

    if (pos_.y >= kGroundY) {
        if (!onGround_) {
            // Vừa tiếp đất
            if (state_ == FighterState::Jump || state_ == FighterState::Fall)
                SetState(FighterState::Idle);
            else if (state_ == FighterState::Hurt && hitstun_ > 0.0f)
                SetState(FighterState::Knockdown);
        }
        pos_.y = kGroundY;
        vel_.y = 0.0f;
        onGround_ = true;
        // Ma sát khi đang bị đẩy lùi dưới đất
        if (!CanAct()) vel_.x *= 0.82f;
    } else {
        onGround_ = false;
        if (state_ == FighterState::Jump && vel_.y > 0.0f) SetState(FighterState::Fall);
    }

    pos_.x = std::clamp(pos_.x, kStageLeft, kStageRight);
}

// ---------------------------------------------------------------------------
// Đòn đánh
// ---------------------------------------------------------------------------
void Fighter::StartMove(MoveSlot slot)
{
    const MoveDef &m = Move(slot);
    if (m.meterCost > meter_) return;

    ConsumeMeter(m.meterCost);

    activeSlot_   = slot;
    moveTimer_    = 0.0f;
    hitsLeft_     = std::max(1, m.hits);
    nextHitAt_    = 0.0f;
    consumedHit_  = false;
    specialFired_ = false;
    armorLeft_    = m.armor ? (m.startup + m.active) : 0.0f;

    if (m.lunge != 0.0f && onGround_) vel_.x = FacingSign() * m.lunge;

    SetState(FighterState::Attack);
    PlayAnim(m.anim, true);

    // Khớp thời lượng animation với frame data, để hình và hitbox đi cùng nhau.
    const Animation &a = def_.Anim(m.anim);
    const float total  = m.startup + m.active + m.recovery;
    if (total > 0.0f && a.Duration() > 0.0f) animator_.SetSpeed(a.Duration() / total);

    if (slot == MoveSlot::Special) specialCd_ = m.cooldown;
    if (slot == MoveSlot::Super)   superCd_   = m.cooldown;
}

void Fighter::UpdateAttack(Arena &arena, float dt)
{
    const MoveDef &m = Move(activeSlot_);
    const float total = m.startup + m.active + m.recovery;
    moveTimer_ += dt;

    // Lực lao tới chỉ có trong pha startup rồi tắt dần.
    if (m.lunge != 0.0f) {
        if (moveTimer_ < m.startup) vel_.x = FacingSign() * m.lunge;
        else                        vel_.x *= 0.88f;
    } else if (onGround_) {
        vel_.x *= 0.80f;
    }

    const bool inActiveWindow = moveTimer_ >= m.startup && moveTimer_ < m.startup + m.active;

    if (inActiveWindow && !specialFired_) {
        specialFired_ = true;
        if (activeSlot_ == MoveSlot::Special) OnSpecialActivate(arena);
        if (activeSlot_ == MoveSlot::Super)   OnSuperActivate(arena);
    }

    attack_.active = inActiveWindow && CanAttackHit() && moveTimer_ >= nextHitAt_;
    attack_.slot   = activeSlot_;
    attack_.def    = &m;
    attack_.world  = WorldBox(m.box);

    if (moveTimer_ >= total) {
        attack_.active = false;
        animator_.SetSpeed(1.0f);
        SetState(onGround_ ? FighterState::Idle : FighterState::Fall);
    }
}

bool Fighter::CanAttackHit() const
{
    return hitsLeft_ > 0 && !consumedHit_;
}

void Fighter::NotifyAttackLanded(Arena &arena, Fighter &target, bool blocked)
{
    const MoveDef &m = Move(activeSlot_);

    --hitsLeft_;
    if (hitsLeft_ <= 0) consumedHit_ = true;
    else                nextHitAt_ = moveTimer_ + m.hitSpacing;

    attack_.active = false;

    AddMeter(blocked ? m.meterGain * 0.35f : m.meterGain);

    if (!blocked) {
        comboCount_ += 1;
        comboTimer_  = kComboWindow;
    }

    const Vector2 point = Vector2{
        (pos_.x + target.Position().x) * 0.5f,
        target.Center().y
    };
    OnHitConfirm(arena, activeSlot_, point);
}

// ---------------------------------------------------------------------------
// Nhận đòn
// ---------------------------------------------------------------------------
void Fighter::ReceiveHit(Arena &arena, Fighter &attacker, const MoveDef &move, bool blocked)
{
    const float sign = (pos_.x < attacker.Position().x) ? -1.0f : 1.0f;

    if (blocked) {
        // Đỡ được: mất ít máu (chip damage), bị đẩy nhẹ, không mất trớn.
        const int chip = std::max(1, (int)(move.damage * 0.12f * def_.defenseScale));
        health_ = std::max(0, health_ - chip);
        blockstun_ = move.blockstun;
        vel_.x = sign * move.knockback * 0.25f;
        AddMeter(4.0f);
        arena.Fx().Burst(Center(), 10, Color{200, 225, 255, 255}, ParticleKind::Spark, 0.6f);
        arena.Shake(2.5f);
        arena.HitStop(0.035f);
        return;
    }

    // Armor: chịu đòn nhưng vẫn mất máu và không bị khựng.
    const bool hasArmor = armorLeft_ > 0.0f;

    int dmg = (int)(move.damage * def_.defenseScale * incomingScale_);
    // Combo càng dài, sát thương mỗi đòn càng giảm - tránh một chuỗi ăn trọn máu.
    if (attacker.ComboCount() > 1) {
        const float scale = std::max(0.42f, 1.0f - 0.10f * (float)(attacker.ComboCount() - 1));
        dmg = (int)(dmg * scale);
    }
    dmg = std::max(1, dmg);

    health_ = std::max(0, health_ - dmg);
    AddMeter(dmg * 0.045f);
    flashTimer_ = 0.14f;

    arena.Fx().Burst(Center(), 16, Color{255, 226, 140, 255}, ParticleKind::Spark, 1.0f);
    arena.Fx().Ring(Center(), Color{255, 255, 255, 190}, 26.0f);
    arena.Shake(move.launcher ? 9.0f : 5.0f);
    arena.HitStop(move.launcher ? 0.10f : 0.06f);

    if (hasArmor) return;

    hitstun_ = move.hitstun;
    vel_.x   = sign * move.knockback;

    if (move.launcher) {
        vel_.y    = -760.0f;
        onGround_ = false;
        SetState(FighterState::Hurt);
    } else if (move.knockback >= kKnockdownThresh && health_ <= 0) {
        SetState(FighterState::Knockdown);
    } else {
        SetState(FighterState::Hurt);
    }

    comboCount_ = 0;
    comboTimer_ = 0.0f;
    animator_.SetSpeed(1.0f);
    PlayAnim(health_ <= 0 ? AnimId::Death : AnimId::TakeHit, true);
}

void Fighter::SetOutcome(bool won)
{
    frozen_ = true;
    attack_.active = false;
    vel_ = Vector2{0.0f, 0.0f};
    // Xoá combo: hiệp đã kết thúc, để nguyên thì chữ "n HIT COMBO" treo lại
    // trên màn hình kết quả vì bộ đếm không còn được cập nhật.
    comboCount_ = 0;
    comboTimer_ = 0.0f;
    animator_.SetSpeed(1.0f);
    if (won) {
        state_ = FighterState::Victory;
        PlayAnim(AnimId::Idle, true);
    } else {
        state_ = FighterState::Defeat;
        PlayAnim(AnimId::Death, true);
    }
}

void Fighter::FreezeForRoundEnd()
{
    frozen_ = true;
    attack_.active = false;
    vel_.x = 0.0f;
    comboCount_ = 0;
    comboTimer_ = 0.0f;
}

// ---------------------------------------------------------------------------
// Tiện ích
// ---------------------------------------------------------------------------
void Fighter::AddMeter(float amount)
{
    meter_ = std::clamp(meter_ + amount * def_.meterRate * meterScale_, 0.0f, kMaxMeter);
}

void Fighter::ConsumeMeter(float amount)
{
    meter_ = std::clamp(meter_ - amount, 0.0f, kMaxMeter);
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
    return Vector2{pos_.x, pos_.y - kBodyHeight * 0.55f};
}

Rectangle Fighter::Hurtbox() const
{
    const float h = (state_ == FighterState::Crouch) ? kCrouchHeight : kBodyHeight;
    return Rectangle{pos_.x - kBodyWidth * 0.5f, pos_.y - h, kBodyWidth, h};
}

Rectangle Fighter::WorldBox(const Rectangle &local) const
{
    // local được viết theo hướng mặt sang phải; khi quay trái thì lật quanh pos_.x.
    if (facingRight_) {
        return Rectangle{pos_.x + local.x, pos_.y + local.y, local.width, local.height};
    }
    return Rectangle{pos_.x - local.x - local.width, pos_.y + local.y, local.width, local.height};
}

void Fighter::SetState(FighterState s)
{
    if (state_ == s) return;
    state_ = s;
    stateTimer_ = 0.0f;
    if (s != FighterState::Attack) animator_.SetSpeed(1.0f);
}

void Fighter::PlayAnim(AnimId id, bool restart)
{
    animator_.Play(&def_.Anim(id), restart);
}

void Fighter::UpdateAnimation(float dt)
{
    // Trạng thái quyết định animation; riêng Attack/Hurt/Death đã đặt sẵn ở nơi
    // gây ra chúng nên chỉ cần để animator chạy tiếp.
    switch (state_) {
        case FighterState::Idle:      PlayAnim(AnimId::Idle, false); break;
        case FighterState::Walk:      PlayAnim(AnimId::Run,  false); break;
        case FighterState::Crouch:    PlayAnim(AnimId::Idle, false); break;
        case FighterState::Block:     PlayAnim(AnimId::Idle, false); break;
        case FighterState::Jump:      PlayAnim(AnimId::Jump, false); break;
        case FighterState::Fall:      PlayAnim(AnimId::Fall, false); break;
        case FighterState::Victory:   PlayAnim(AnimId::Idle, false); break;
        default: break;
    }
    animator_.Update(dt);
}

// ---------------------------------------------------------------------------
// Vẽ
// ---------------------------------------------------------------------------
void Fighter::DrawShadow() const
{
    const float height = std::max(0.0f, kGroundY - pos_.y);
    const float t      = std::clamp(1.0f - height / 420.0f, 0.25f, 1.0f);
    DrawEllipse((int)pos_.x, (int)(kGroundY + 6.0f),
                46.0f * t, 12.0f * t, Color{0, 0, 0, (unsigned char)(120 * t)});
}

void Fighter::Draw(bool debugBoxes) const
{
    const Animation *anim = animator_.Current();
    if (!anim || anim->texture.id == 0) return;

    const float side  = (float)anim->texture.height;
    const float scale = def_.scale;

    Rectangle src = anim->FrameRect(animator_.Frame());
    if (!facingRight_) src.width = -src.width;

    // Neo sprite sao cho điểm anchor trùng với giữa hai bàn chân của nhân vật.
    const float anchorX = facingRight_ ? def_.anchor.x : (side - def_.anchor.x);
    Rectangle dest{
        pos_.x - anchorX * scale,
        pos_.y - def_.anchor.y * scale,
        side * scale,
        side * scale
    };

    DrawBehind(Vector2{dest.x, dest.y}, scale);

    Color tint = WHITE;
    if (flashTimer_ > 0.0f) {
        // Nháy đỏ khi vừa ăn đòn.
        const float k = flashTimer_ / 0.14f;
        tint = Color{255, (unsigned char)(255 - 150 * k), (unsigned char)(255 - 150 * k), 255};
    }
    if (state_ == FighterState::Block) {
        tint = Color{190, 215, 255, 255};
    }

    DrawTexturePro(anim->texture, src, dest, Vector2{0.0f, 0.0f}, 0.0f, tint);

    DrawFront(Vector2{dest.x, dest.y}, scale);

    // Lá chắn khi đỡ đòn
    if (state_ == FighterState::Block) {
        const Vector2 c = Center();
        const float r = 60.0f;
        DrawCircleLines((int)(c.x + FacingSign() * 18.0f), (int)c.y, r,
                        Color{150, 200, 255, 120});
        DrawCircleLines((int)(c.x + FacingSign() * 18.0f), (int)c.y, r - 4.0f,
                        Color{210, 235, 255, 70});
    }

    if (debugBoxes) {
        const Rectangle hb = Hurtbox();
        DrawRectangleLinesEx(hb, 1.0f, Color{80, 200, 255, 200});
        if (attack_.active) DrawRectangleLinesEx(attack_.world, 2.0f, Color{255, 70, 70, 230});
    }
}

} // namespace fighter
