#include "entities/Arena.hpp"
#include "characters/Roster.hpp"
#include <algorithm>
#include <cmath>

namespace fighter {

void Arena::SetFighters(std::unique_ptr<Fighter> p1, std::unique_ptr<Fighter> p2)
{
    p1_ = std::move(p1);
    p2_ = std::move(p2);
    projectiles_.clear();
    fx_.Clear();
    events_.clear();
}

void Arena::ResetPositions()
{
    if (!Ready()) return;
    const float mid = kStageWidth * 0.5f;
    p1_->Reset(mid - 190.0f, true);
    p2_->Reset(mid + 190.0f, false);
    projectiles_.clear();
    fx_.Clear();
    events_.clear();
    hitStop_ = superFreeze_ = shake_ = 0.0f;
    superOwner_ = -1;
    shakeOffset_ = Vector2{0.0f, 0.0f};
    zoom_ = zoomTarget_ = 1.0f;
    UpdateCamera(0.0f, true);
}

Fighter &Arena::Opponent(const Fighter &who)
{
    return (&who == p1_.get()) ? *p2_ : *p1_;
}

// ---------------------------------------------------------------------------
// Đạn
// ---------------------------------------------------------------------------
void Arena::SpawnProjectile(std::unique_ptr<Projectile> p)
{
    if (!p) return;
    Emit({EventType::Projectile, p->OwnerId(), p->Position()});
    projectiles_.push_back(std::move(p));
}

int Arena::CountProjectiles(int ownerId) const
{
    int n = 0;
    for (const auto &p : projectiles_) if (p->Alive() && p->OwnerId() == ownerId) ++n;
    return n;
}

bool Arena::ProjectileThreatens(const Fighter &who) const
{
    const int id = IdOf(who);
    for (const auto &p : projectiles_) {
        if (!p->Alive() || p->OwnerId() == id) continue;
        const float dx = who.Position().x - p->Position().x;
        const bool toward = (dx > 0.0f) == (p->Dir() > 0.0f);
        if ((toward || std::fabs(dx) < 120.0f) && std::fabs(dx) < 460.0f) return true;
    }
    return false;
}

void Arena::CountProjectileHit(Fighter &owner, int damage)
{
    // Đạn trúng cũng tính vào combo của người bắn (để bảng combo hiện đúng).
    Fighter &target = Opponent(owner);
    (void)target;
    owner.NotifyProjectileCombo(damage);
}

// ---------------------------------------------------------------------------
// Hiệu ứng
// ---------------------------------------------------------------------------
void Arena::HitSpark(Vector2 at, bool blocked, SfxWeight w, Color accent)
{
    const float power = 0.8f + 0.35f * (int)w;
    if (blocked) {
        fx_.Spark(at, Color{150, 200, 255, 255}, power * 0.8f);
        fx_.Burst(at, 8, Color{200, 225, 255, 255}, ParticleKind::Spark, 0.6f);
    } else {
        fx_.Spark(at, Color{255, 236, 170, 255}, power);
        fx_.Burst(at, 10 + 6 * (int)w, Color{255, 214, 120, 255}, ParticleKind::Spark, power);
        fx_.Burst(at, 4 + 3 * (int)w, accent, ParticleKind::Shard, power);
        fx_.Ring(at, Color{255, 255, 255, 200}, 18.0f + 10.0f * (int)w);
    }
}

void Arena::HitStop(float seconds)
{
    hitStop_ = std::max(hitStop_, seconds);
}

void Arena::Shake(float strength)
{
    shake_ = std::min(28.0f, shake_ + strength);
}

void Arena::StartSuperFreeze(Fighter &who)
{
    superFreeze_ = 0.75f;
    superOwner_  = IdOf(who);
    zoomTarget_  = 1.08f;
    fx_.Ring(who.Center(), who.Def().accent, 40.0f);
    fx_.Burst(who.Center(), 30, who.Def().accent, ParticleKind::Energy, 1.4f);
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------
void Arena::UpdateCamera(float dt, bool snap)
{
    const float mid = (p1_->Position().x + p2_->Position().x) * 0.5f;
    zoom_ += (zoomTarget_ - zoom_) * (1.0f - std::exp(-6.0f * dt));

    const float view = ViewWidth();
    const float target = std::clamp(mid - view * 0.5f, 0.0f, kStageWidth - view);
    if (snap) camX_ = target;
    else      camX_ += (target - camX_) * (1.0f - std::exp(-10.0f * dt));
}

Camera2D Arena::Camera() const
{
    Camera2D cam{};
    // Neo phóng to tại mặt đất: đường sàn của thế giới luôn trùng đường sàn
    // của nền vẽ sẵn, dù đang zoom thêm lúc K.O.
    cam.target   = Vector2{camX_ + ViewWidth() * 0.5f, kGroundY};
    cam.offset   = Vector2{kCanvasWidth * 0.5f + shakeOffset_.x, kGroundY + shakeOffset_.y};
    cam.rotation = 0.0f;
    cam.zoom     = kCameraZoom * zoom_;
    return cam;
}

// ---------------------------------------------------------------------------
// Cập nhật
// ---------------------------------------------------------------------------
void Arena::Update(const InputState &in1, const InputState &in2, float dt)
{
    if (!Ready()) return;

    if (shake_ > 0.0f) {
        shake_ = std::max(0.0f, shake_ - dt * 45.0f);
        shakeOffset_ = Vector2{
            (float)GetRandomValue(-100, 100) / 100.0f * shake_,
            (float)GetRandomValue(-100, 100) / 100.0f * shake_ * 0.6f
        };
    } else {
        shakeOffset_ = Vector2{0.0f, 0.0f};
    }

    static const InputState kIdle{};
    const InputState &a = inputEnabled_ ? in1 : kIdle;
    const InputState &b = inputEnabled_ ? in2 : kIdle;

    // Siêu chiêu: cả thế giới đứng hình, chỉ hào quang còn chạy.
    if (superFreeze_ > 0.0f) {
        p1_->BufferInput(a);
        p2_->BufferInput(b);
        superFreeze_ -= dt;
        if (superFreeze_ <= 0.0f) { zoomTarget_ = 1.0f; superOwner_ = -1; }
        fx_.Update(dt);
        UpdateCamera(dt, false);
        return;
    }

    // Khựng hình khi trúng đòn - thứ làm cú đánh có "sức nặng".
    if (hitStop_ > 0.0f) {
        p1_->BufferInput(a);
        p2_->BufferInput(b);
        hitStop_ -= dt;
        fx_.Update(dt * 0.3f);
        return;
    }

    FaceEachOther();

    p1_->Update(*this, a, *p2_, dt);
    if (superFreeze_ > 0.0f) return;         // P1 vừa tung siêu chiêu
    p2_->Update(*this, b, *p1_, dt);

    ResolveBodies();
    ResolveAttack(*p1_, *p2_);
    ResolveAttack(*p2_, *p1_);

    for (auto &p : projectiles_) if (p->Alive()) p->Update(*this, dt);
    ResolveProjectiles();
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
                       [](const std::unique_ptr<Projectile> &p) { return !p->Alive(); }),
        projectiles_.end());

    // Combo chấm dứt khi đối thủ thoát khỏi trạng thái dính đòn.
    if (!p2_->InHitstun()) p1_->EndCombo();
    if (!p1_->InHitstun()) p2_->EndCombo();

    ConstrainFighters();
    UpdateCamera(dt, false);
    fx_.Update(dt);
}

void Arena::FaceEachOther()
{
    // Chỉ quay người khi đứng dưới đất và rảnh tay: nhảy qua đầu đối thủ thì
    // tới lúc chạm đất mới quay lại, giống Street Fighter.
    if (!p1_->IsBusy() && p1_->OnGround()) p1_->FaceTowards(p2_->Position().x);
    if (!p2_->IsBusy() && p2_->OnGround()) p2_->FaceTowards(p1_->Position().x);
}

void Arena::ResolveBodies()
{
    // Người đang bị vật, đang lướt xuyên, hoặc đang bay cao thì không đẩy nhau.
    if (p1_->State() == FighterState::Thrown || p2_->State() == FighterState::Thrown) return;
    if (p1_->PassesThrough() || p2_->PassesThrough()) return;
    const float dy = std::fabs(p1_->Position().y - p2_->Position().y);
    if (dy > 110.0f) return;

    const float dx  = p2_->Position().x - p1_->Position().x;
    const float min = kBodyRadius * 2.0f;
    const float dist = std::fabs(dx);
    if (dist >= min) return;

    float sign = (dx > 0.0f) ? 1.0f : -1.0f;
    if (dist < 0.5f) sign = p1_->FacingRight() ? 1.0f : -1.0f;
    const float push = (min - dist) * 0.5f;
    p1_->Reposition(p1_->Position().x - sign * push);
    p2_->Reposition(p2_->Position().x + sign * push);
}

void Arena::ConstrainFighters()
{
    // Tường hai đầu sàn
    for (Fighter *f : {p1_.get(), p2_.get()}) {
        f->Reposition(std::clamp(f->Position().x, kStageLeft, kStageRight));
    }

    // Không cho hai người cách nhau quá một màn hình: ai đang đi ra xa thì bị
    // chặn lại (người kia coi như "kéo" camera).
    const float dx = p2_->Position().x - p1_->Position().x;
    const float over = std::fabs(dx) - kMaxSeparation;
    if (over > 0.0f) {
        const float s = (dx > 0.0f) ? 1.0f : -1.0f;
        const bool p1Away = p1_->Velocity().x * -s > 0.0f;
        const bool p2Away = p2_->Velocity().x *  s > 0.0f;
        float a = 0.5f, b = 0.5f;
        if (p1Away && !p2Away) { a = 1.0f; b = 0.0f; }
        if (p2Away && !p1Away) { a = 0.0f; b = 1.0f; }
        p1_->Reposition(p1_->Position().x + s * over * a);
        p2_->Reposition(p2_->Position().x - s * over * b);
    }
}

void Arena::ResolveAttack(Fighter &attacker, Fighter &defender)
{
    const ActiveAttack &atk = attacker.Attack();
    if (!atk.active || !atk.def) return;
    if (defender.IsDefeated() && defender.State() != FighterState::AirHurt) return;
    if (!CheckCollisionRecs(atk.world, defender.Hurtbox())) return;

    // Đòn nhiều hit có "hit kết": riêng hit cuối làm ngã và đẩy xa hơn.
    MoveDef m = *atk.def;
    if (m.finisherKnockdown && attacker.HitsLeft() == 1) {
        m.knockdown = true;
        m.knockback *= 1.6f;
    }
    const int aid = IdOf(attacker);

    // Siêu chiêu được phép bỏ qua giới hạn tung hứng để đủ số hit.
    const bool ignoreJuggle = m.meterCost > 0.0f && defender.State() == FighterState::AirHurt;
    if (defender.IsInvulnerable() && !ignoreJuggle) return;

    // --- Vật ---------------------------------------------------------------
    if (m.height == HitHeight::Throw) {
        if (!defender.CanBeThrown()) return;
        if (m.techable && defender.PressedThrowRecently()) {
            const float dir = (attacker.Position().x < defender.Position().x) ? 1.0f : -1.0f;
            attacker.BreakThrow(-dir);
            defender.BreakThrow(dir);
            Emit({EventType::ThrowTech, aid, defender.Center(), SfxWeight::Medium});
            HitSpark(defender.Center(), true, SfxWeight::Medium, WHITE);
            return;
        }
        attacker.BeginThrow(*this, defender);
        return;
    }

    // --- Phản đòn ----------------------------------------------------------
    if (defender.OnIncomingHit(*this, attacker, m)) {
        attacker.WhiffCurrentHit();
        return;
    }

    const bool blocked = defender.WantsToBlock(attacker) && defender.CanBlockHeight(m.height);
    const bool counter = !blocked && defender.InStartupOrActive();

    const float dir = (attacker.Position().x < defender.Position().x) ? 1.0f : -1.0f;
    const int dmg = defender.ReceiveHit(*this, attacker, m, blocked, counter);
    attacker.NotifyAttackLanded(*this, defender, blocked, dmg);

    // Đối thủ dính tường thì người đánh bị đẩy lùi thay (tránh dồn góc vô hạn).
    const bool atWall = defender.Position().x <= kStageLeft + 4.0f ||
                        defender.Position().x >= kStageRight - 4.0f;
    if (atWall && attacker.OnGround()) attacker.ApplyKnockback(-dir * m.knockback * 0.6f, 0.0f);

    Vector2 spark{(attacker.Position().x + defender.Position().x) * 0.5f + dir * 10.0f,
                  atk.world.y + atk.world.height * 0.5f};
    spark.y = std::clamp(spark.y, defender.Hurtbox().y + 10.0f,
                         defender.Hurtbox().y + defender.Hurtbox().height - 10.0f);
    HitSpark(spark, blocked, m.sfx, attacker.Def().accent);

    const float weight = 1.0f + (int)m.sfx;
    if (blocked) {
        HitStop(0.035f + 0.01f * weight);
        Shake(1.5f * weight);
        Emit({EventType::Block, aid, spark, m.sfx, dmg});
    } else {
        HitStop(0.045f + 0.025f * weight + (counter ? 0.04f : 0.0f));
        Shake(2.5f * weight + (m.launcher || m.knockdown ? 4.0f : 0.0f));
        Emit({counter ? EventType::CounterHit : EventType::Hit, aid, spark, m.sfx, dmg});
    }

    if (defender.IsDefeated()) {
        Emit({EventType::KO, aid, spark, SfxWeight::Heavy, dmg});
    }
}

void Arena::ResolveProjectiles()
{
    // Đạn chạm đạn: triệt tiêu
    for (size_t i = 0; i < projectiles_.size(); ++i) {
        for (size_t j = i + 1; j < projectiles_.size(); ++j) {
            Projectile &a = *projectiles_[i];
            Projectile &b = *projectiles_[j];
            if (!a.Alive() || !b.Alive() || a.OwnerId() == b.OwnerId()) continue;
            if (!a.Clashable() || !b.Clashable() || !a.Harmful() || !b.Harmful()) continue;
            if (!CheckCollisionRecs(a.Box(), b.Box())) continue;
            const Vector2 at{(a.Position().x + b.Position().x) * 0.5f, a.Position().y};
            a.OnClash(*this);
            b.OnClash(*this);
            Emit({EventType::Clash, a.OwnerId(), at, SfxWeight::Medium});
        }
    }

    for (auto &p : projectiles_) {
        if (!p->Alive() || !p->Harmful()) continue;
        Fighter *target = ById(1 - p->OwnerId());
        if (!target) continue;
        bool invulnerable = target->IsProjInvulnerable();
        if (invulnerable && p->FromSuper() && target->State() == FighterState::AirHurt) invulnerable = false;
        if (invulnerable) continue;
        if (target->IsDefeated() && target->State() != FighterState::AirHurt) continue;
        if (!CheckCollisionRecs(p->Box(), target->Hurtbox())) continue;

        const bool blocked = target->WantsToBlockFrom(p->Position().x) &&
                             target->CanBlockHeight(p->Height());
        p->OnHit(*this, *target, blocked);
        HitStop(blocked ? 0.04f : 0.07f);
        Shake(blocked ? 2.0f : 5.0f);
        if (target->IsDefeated()) {
            Emit({EventType::KO, p->OwnerId(), p->Position(), SfxWeight::Heavy});
        }
    }
}

// ---------------------------------------------------------------------------
// Vẽ (toạ độ thế giới)
// ---------------------------------------------------------------------------
void Arena::DrawWorld(bool debugBoxes) const
{
    if (!Ready()) return;

    p1_->DrawShadow();
    p2_->DrawShadow();

    // Người đang tấn công vẽ sau cùng để đòn đánh nằm trên người bị đánh.
    const Fighter *first  = p1_.get();
    const Fighter *second = p2_.get();
    if (p1_->IsAttacking() && !p2_->IsAttacking()) std::swap(first, second);
    first->Draw(debugBoxes);
    second->Draw(debugBoxes);

    // Vệt chém vẽ blend thường: blend cộng sáng trên nền hoàng hôn sáng sẽ
    // cháy thành một mảng trắng.
    p1_->DrawTrails();
    p2_->DrawTrails();

    for (const auto &p : projectiles_) if (p->Alive()) p->Draw();

    fx_.Draw();

    if (debugBoxes) {
        for (const auto &p : projectiles_) {
            if (p->Alive()) DrawRectangleLinesEx(p->Box(), 1.0f, Color{255, 160, 60, 220});
        }
    }
}

} // namespace fighter
