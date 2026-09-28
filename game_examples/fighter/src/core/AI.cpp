#include "core/AI.hpp"
#include "characters/Roster.hpp"
#include "entities/Arena.hpp"
#include "entities/Fighter.hpp"
#include <cmath>

namespace fighter {

namespace {
float Chance() { return (float)GetRandomValue(0, 1000) / 1000.0f; }
}

AIController::AIController(Difficulty level, const Arena *arena)
    : level_(level), arena_(arena)
{
}

// Đổi hướng tương đối (numpad) thành phím trái/phải/lên/xuống tuyệt đối.
void AIController::Apply(int rel, bool towardRight)
{
    const int h = (rel - 1) % 3;     // 0 = lùi, 1 = giữa, 2 = tiến
    const int v = (rel - 1) / 3;     // 0 = xuống, 1 = giữa, 2 = lên
    const bool fwd = h == 2, back = h == 0;
    state_.right = towardRight ? fwd : back;
    state_.left  = towardRight ? back : fwd;
    state_.down  = v == 0;
    state_.up    = v == 2;
}

// ---------------------------------------------------------------------------
// Kịch bản bấm phím
// ---------------------------------------------------------------------------
void AIController::PlanCombo(const Fighter &self)
{
    const DifficultyProfile &p = GetDifficulty(level_);
    script_.clear();

    // Mở đòn bằng đứng/ngồi nhẹ (ngồi = đòn thấp, trộn để khó đỡ).
    const bool low = Chance() < 0.45f;
    Step s0; s0.dir = low ? 2 : 5; s0.L = true; script_.push_back(s0);

    if (Chance() < p.comboChance) {
        Step s1; s1.at = 0.15f; s1.dir = low ? 2 : 5; s1.M = true; script_.push_back(s1);
        Step s2; s2.at = 0.34f; s2.dir = 5; s2.H = true; script_.push_back(s2);
        // Hủy đòn mạnh sang chiêu đặc biệt / siêu chiêu.
        if (Chance() < p.comboChance) {
            const bool super = self.MeterStocks() >= 1 && Chance() < 0.5f;
            Step s3; s3.at = 0.56f;
            if (super) s3.U = true;
            else { s3.S = true; s3.dir = (self.Def().id == "wizard") ? 2 : 5; }
            if (!super && (self.Def().id == "kenji" || self.Def().id == "knight")) s3.dir = 6;
            script_.push_back(s3);
        }
    } else if (Chance() < 0.4f) {
        // Quét chân rồi thôi.
        Step s1; s1.at = 0.2f; s1.dir = 2; s1.H = true; script_.push_back(s1);
    }
    scriptTime_ = 0.0f;
    scriptIndex_ = 0;
}

void AIController::PlanSpecial(char slot, char strength)
{
    script_.clear();
    Step s;
    switch (slot) {
        case 'B': s.dir = 6; break;
        case 'C': s.dir = 2; break;
        case 'D': s.dir = 4; break;
        default:  s.dir = 5; break;
    }
    // Nút I + hướng cho lực vừa; muốn lực khác thì mô phỏng lệnh quay tay.
    if (strength == 'M') {
        s.S = true;
        script_.push_back(s);
    } else {
        static const char *motions[] = {"236", "214", "623", "252"};
        const char *m = motions[slot - 'A'];
        float t = 0.0f;
        for (const char *c = m; *c; ++c) {
            Step d; d.at = t; d.dir = *c - '0';
            script_.push_back(d);
            t += 0.03f;
        }
        Step hit; hit.at = t; hit.dir = m[2] - '0';
        if (strength == 'L') hit.L = true; else hit.H = true;
        script_.push_back(hit);
    }
    scriptTime_ = 0.0f;
    scriptIndex_ = 0;
}

void AIController::PlanSuper()
{
    script_.clear();
    Step s; s.U = true;
    script_.push_back(s);
    scriptTime_ = 0.0f;
    scriptIndex_ = 0;
}

void AIController::PlanJumpIn()
{
    script_.clear();
    Step j; j.dir = 9; script_.push_back(j);
    Step hold; hold.at = 0.05f; hold.dir = 6; script_.push_back(hold);
    Step atk; atk.at = 0.36f + 0.1f * Chance(); atk.dir = 6; atk.H = true; script_.push_back(atk);
    scriptTime_ = 0.0f;
    scriptIndex_ = 0;
}

void AIController::PlanThrow()
{
    script_.clear();
    Step s; s.dir = 6; s.L = true; s.M = true;
    script_.push_back(s);
    scriptTime_ = 0.0f;
    scriptIndex_ = 0;
}

void AIController::RunScript(const Fighter &self, const Fighter &opp, float dt)
{
    const bool oppRight = opp.Position().x > self.Position().x;
    scriptTime_ += dt;

    // Hướng của bước gần nhất đã tới lượt được giữ cho tới bước sau.
    int dir = 5;
    while (scriptIndex_ < script_.size() && script_[scriptIndex_].at <= scriptTime_) {
        const Step &s = script_[scriptIndex_];
        state_.lightPressed   |= s.L;
        state_.mediumPressed  |= s.M;
        state_.heavyPressed   |= s.H;
        state_.specialPressed |= s.S;
        state_.superPressed   |= s.U;
        dir = s.dir;
        ++scriptIndex_;
    }
    if (scriptIndex_ > 0 && dir == 5) dir = script_[scriptIndex_ - 1].dir;
    Apply(dir, oppRight);

    if (scriptIndex_ >= script_.size() && scriptTime_ > script_.back().at + 0.1f) {
        script_.clear();
        scriptIndex_ = 0;
    }
}

// ---------------------------------------------------------------------------
// Quyết định
// ---------------------------------------------------------------------------
void AIController::Decide(const Fighter &self, const Fighter &opp)
{
    const DifficultyProfile &p = GetDifficulty(level_);
    const std::string &id = self.Def().id;
    const float dist = std::fabs(opp.Position().x - self.Position().x);
    const bool zoner = id == "wizard";

    // Có siêu chiêu và đối thủ đang hở (vừa hụt đòn ở gần) -> trừng phạt.
    if (self.MeterStocks() >= 1 && dist < 220.0f && opp.IsAttacking() &&
        !opp.InStartupOrActive() && Chance() < p.aggression) {
        PlanSuper();
        return;
    }

    if (dist < 150.0f) {
        const float r = Chance();
        if (id == "knight" && r < 0.25f * p.aggression) { PlanSpecial('D', 'H'); return; }   // vật lệnh
        if (r < 0.15f) { PlanThrow(); return; }
        if (r < 0.15f + p.aggression * 0.7f) { PlanCombo(self); return; }
        walkDir_ = (Chance() < 0.5f) ? 4 : 2;   // lùi thủ hoặc ngồi thủ
        walkTimer_ = 0.25f + Chance() * 0.3f;
        return;
    }

    if (dist < 420.0f) {
        const float r = Chance();
        if (zoner) {
            if (r < 0.35f) { PlanSpecial('B', dist < 300 ? 'L' : 'M'); return; }   // cột đất
            if (r < 0.55f) { walkDir_ = 4; walkTimer_ = 0.4f; return; }
            if (r < 0.65f) { PlanSpecial('D', 'H'); return; }                     // dịch chuyển về xa
        }
        if (id == "knight" && r < 0.2f * p.aggression) { PlanSpecial('A', 'M'); return; }
        if (id == "kenji" && r < 0.25f * p.aggression) { PlanSpecial(Chance() < 0.5f ? 'B' : 'C', 'M'); return; }
        if (id == "samurai" && r < 0.18f) { PlanSpecial('A', 'M'); return; }
        if (r < p.aggression * 0.25f) { PlanJumpIn(); return; }
        walkDir_ = (Chance() < p.aggression) ? 6 : 4;
        walkTimer_ = 0.2f + Chance() * 0.35f;
        return;
    }

    // Xa: bắn đạn hoặc tiến vào.
    const float r = Chance();
    if ((zoner || id == "samurai") && r < 0.5f) { PlanSpecial('A', Chance() < 0.5f ? 'L' : 'H'); return; }
    if (id == "kenji" && r < 0.3f) { PlanSpecial('A', 'M'); return; }
    walkDir_ = 6;
    walkTimer_ = 0.3f + Chance() * 0.4f;
}

void AIController::Update(const Fighter &self, const Fighter &opp, float dt)
{
    const DifficultyProfile &p = GetDifficulty(level_);
    state_.Clear();
    if (self.IsDefeated() || opp.IsDefeated()) return;

    const bool oppRight = opp.Position().x > self.Position().x;
    const float dist = std::fabs(opp.Position().x - self.Position().x);
    thinkTimer_ -= dt;
    walkTimer_  -= dt;
    antiAirCd_  -= dt;

    // Đang chạy kịch bản thì đi hết kịch bản.
    if (!script_.empty()) {
        RunScript(self, opp, dt);
        return;
    }

    // --- Phòng thủ: phát hiện đối thủ bắt đầu vung đòn / có đạn -----------
    const bool oppAttacking = opp.InStartupOrActive() && dist < 320.0f;
    const bool projectile = arena_ && arena_->ProjectileThreatens(self);
    if ((oppAttacking && !oppWasAttacking_) || (projectile && reactTimer_ <= 0.0f && !willBlock_)) {
        willBlock_  = Chance() < p.blockChance;
        reactTimer_ = p.reaction * (0.7f + 0.6f * Chance());
    }
    oppWasAttacking_ = oppAttacking;
    reactTimer_ -= dt;

    if ((oppAttacking || projectile) && willBlock_ && reactTimer_ <= 0.0f) {
        // Đỡ đúng độ cao: ngồi đỡ đòn thấp, đứng đỡ đòn nhảy/bổ.
        const HitHeight h = opp.CurrentMove().height;
        bool crouch = h == HitHeight::Low;
        if (h == HitHeight::Mid) crouch = Chance() < 0.5f;
        if (!opp.OnGround() || h == HitHeight::Overhead) crouch = false;
        Apply(crouch ? 1 : 4, oppRight);
        return;
    }
    if (!oppAttacking && !projectile) willBlock_ = false;

    // --- Chống nhảy: đối thủ lao từ trên xuống ----------------------------
    if (!opp.OnGround() && opp.Velocity().y > -200.0f && dist < 260.0f && antiAirCd_ <= 0.0f &&
        self.OnGround() && !self.IsBusy()) {
        antiAirCd_ = 0.8f;
        if (Chance() < p.aggression * 0.9f) {
            if (self.Def().id == "kenji") {
                script_.clear();
                Step s; s.dir = 5; s.H = true; script_.push_back(s);
                scriptTime_ = 0.0f; scriptIndex_ = 0;
            } else {
                PlanSpecial('C', Chance() < 0.5f ? 'H' : 'M');
            }
            RunScript(self, opp, dt);
            return;
        }
    }

    if (self.IsBusy()) return;

    if (thinkTimer_ <= 0.0f && walkTimer_ <= 0.0f) {
        thinkTimer_ = p.reaction * (0.6f + Chance());
        Decide(self, opp);
        if (!script_.empty()) { RunScript(self, opp, dt); return; }
    }

    if (walkTimer_ > 0.0f) Apply(walkDir_, oppRight);
}

} // namespace fighter
