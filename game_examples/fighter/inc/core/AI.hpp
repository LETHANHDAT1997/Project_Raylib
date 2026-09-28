#pragma once
#include "core/Difficulty.hpp"
#include "core/Input.hpp"
#include <string>
#include <vector>

namespace fighter {

class Arena;

// ============================================================================
// AIController - máy đánh. Dùng đúng những gì người chơi có: hướng + nút.
// Không đọc trộm trạng thái nội bộ ngoài những gì mắt người thấy được (đối
// thủ đang vung đòn gì, đứng hay nhảy, có đạn bay tới không).
//
// Độ khó điều chỉnh: thời gian phản ứng, tỉ lệ đỡ đúng, tỉ lệ nối combo trọn
// vẹn, độ hung hăng - toàn bộ lấy từ DifficultyProfile.
// ============================================================================
class AIController : public Controller {
public:
    AIController(Difficulty level, const Arena *arena);
    void Update(const Fighter &self, const Fighter &opponent, float dt) override;

private:
    // Một bước trong "kịch bản" bấm phím (combo, chiêu, vật...).
    struct Step {
        float at = 0.0f;           // giây kể từ lúc bắt đầu kịch bản
        int   dir = 5;             // hướng numpad TƯƠNG ĐỐI (6 = tiến)
        bool  L = false, M = false, H = false, S = false, U = false;
    };

    void Decide(const Fighter &self, const Fighter &opp);
    void RunScript(const Fighter &self, const Fighter &opp, float dt);
    void Apply(int relDir, bool towardRight);

    void PlanCombo(const Fighter &self);
    void PlanSpecial(char slot, char strength = 'M');   // 'A'..'D'
    void PlanSuper();
    void PlanJumpIn();
    void PlanThrow();

    Difficulty level_;
    const Arena *arena_;

    std::vector<Step> script_;
    float scriptTime_ = 0.0f;
    size_t scriptIndex_ = 0;

    // Phòng thủ
    bool  oppWasAttacking_ = false;
    bool  willBlock_  = false;
    float reactTimer_ = 0.0f;

    // Di chuyển
    int   walkDir_   = 0;          // 6 tiến, 4 lùi, 5 đứng, 2 ngồi
    float walkTimer_ = 0.0f;
    float thinkTimer_ = 0.0f;
    float antiAirCd_ = 0.0f;
};

} // namespace fighter
