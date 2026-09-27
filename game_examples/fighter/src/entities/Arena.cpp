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
}

void Arena::ResetPositions()
{
    if (!Ready()) return;
    p1_->Reset(kCanvasWidth * 0.32f, true);
    p2_->Reset(kCanvasWidth * 0.68f, false);
    projectiles_.clear();
    fx_.Clear();
    hitStop_ = 0.0f;
    shake_   = 0.0f;
    shakeOffset_ = Vector2{0.0f, 0.0f};
}

Fighter &Arena::Opponent(const Fighter &who)
{
    return (&who == p1_.get()) ? *p2_ : *p1_;
}

void Arena::SpawnProjectile(std::unique_ptr<Projectile> p)
{
    if (p) projectiles_.push_back(std::move(p));
}

void Arena::HitStop(float seconds)
{
    hitStop_ = std::max(hitStop_, seconds);
}

void Arena::Shake(float strength)
{
    shake_ = std::min(26.0f, shake_ + strength);
}

void Arena::Update(const InputState &in1, const InputState &in2, float dt)
{
    if (!Ready()) return;

    // Rung camera luôn chạy, kể cả khi đang khựng hình - nhờ vậy cú đánh "nảy".
    if (shake_ > 0.0f) {
        shake_ = std::max(0.0f, shake_ - dt * 42.0f);
        shakeOffset_ = Vector2{
            (float)GetRandomValue(-100, 100) / 100.0f * shake_,
            (float)GetRandomValue(-100, 100) / 100.0f * shake_ * 0.6f
        };
    } else {
        shakeOffset_ = Vector2{0.0f, 0.0f};
    }

    if (hitStop_ > 0.0f) {
        hitStop_ -= dt;
        fx_.Update(dt * 0.25f);
        return;
    }

    FaceEachOther();

    static const InputState kIdle{};
    const InputState &a = inputEnabled_ ? in1 : kIdle;
    const InputState &b = inputEnabled_ ? in2 : kIdle;

    p1_->Update(*this, a, *p2_, dt);
    p2_->Update(*this, b, *p1_, dt);

    ResolveBodies();
    ResolveAttack(*p1_, *p2_);
    ResolveAttack(*p2_, *p1_);

    for (auto &p : projectiles_) {
        if (p->Alive()) p->Update(*this, dt);
    }
    ResolveProjectiles();
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
                       [](const std::unique_ptr<Projectile> &p) { return !p->Alive(); }),
        projectiles_.end());

    fx_.Update(dt);
}

void Arena::FaceEachOther()
{
    // Chỉ xoay người khi rảnh tay, nếu không đòn đang vung sẽ bị quay ngược.
    if (!p1_->IsBusy()) p1_->FaceTowards(p2_->Position().x);
    if (!p2_->IsBusy()) p2_->FaceTowards(p1_->Position().x);
}

void Arena::ResolveBodies()
{
    const float dx  = p2_->Position().x - p1_->Position().x;
    const float min = kBodyRadius * 2.0f;
    const float dist = std::fabs(dx);
    if (dist >= min || dist < 0.001f) return;

    const float push = (min - dist) * 0.5f;
    const float sign = (dx > 0.0f) ? 1.0f : -1.0f;

    // Đẩy bằng cách dịch thẳng toạ độ: cộng vào vận tốc sẽ làm hai người rung.
    const float ax = std::clamp(p1_->Position().x - sign * push, kStageLeft, kStageRight);
    const float bx = std::clamp(p2_->Position().x + sign * push, kStageLeft, kStageRight);
    p1_->Reposition(ax);
    p2_->Reposition(bx);
}

void Arena::ResolveAttack(Fighter &attacker, Fighter &defender)
{
    const ActiveAttack &atk = attacker.Attack();
    if (!atk.active || !atk.def) return;
    if (defender.IsDefeated()) return;
    if (!CheckCollisionRecs(atk.world, defender.Hurtbox())) return;

    // Đỡ được khi đang ở thế đỡ VÀ quay mặt về phía người tấn công.
    const bool facingAttacker =
        (defender.Position().x < attacker.Position().x) == defender.FacingRight();
    const bool blocked = defender.IsBlocking() && facingAttacker;

    defender.ReceiveHit(*this, attacker, *atk.def, blocked);
    attacker.NotifyAttackLanded(*this, defender, blocked);
}

void Arena::ResolveProjectiles()
{
    for (auto &p : projectiles_) {
        if (!p->Alive()) continue;
        Fighter *target = ById(1 - p->OwnerId());
        if (!target || target->IsDefeated()) continue;
        if (CheckCollisionRecs(p->Box(), target->Hurtbox())) {
            p->OnHit(*this, *target);
        }
    }
}

void Arena::DrawWorld(bool debugBoxes) const
{
    if (!Ready()) return;

    p1_->DrawShadow();
    p2_->DrawShadow();

    // Người ở xa camera hơn (y nhỏ hơn) vẽ trước để chồng lớp trông tự nhiên.
    if (p1_->Position().y <= p2_->Position().y) {
        p1_->Draw(debugBoxes);
        p2_->Draw(debugBoxes);
    } else {
        p2_->Draw(debugBoxes);
        p1_->Draw(debugBoxes);
    }

    for (const auto &p : projectiles_) {
        if (p->Alive()) p->Draw();
    }

    fx_.Draw();
}

} // namespace fighter
