#pragma once
#include "raylib.h"
#include "core/Input.hpp"
#include "entities/Arena.hpp"
#include "render/Hud.hpp"
#include "scenes/Scene.hpp"
#include <memory>

namespace fighter {

// ============================================================================
// BattleScene - luật trận: đếm hiệp, đồng hồ, chữ "FIGHT!"/"K.O.", tạm dừng,
// bảng kết quả. Vật lý và va chạm nằm ở Arena.
// ============================================================================
class BattleScene final : public Scene {
public:
    void OnEnter(Game &game) override;
    void Update(Game &game, float dt) override;
    void Draw(Game &game) override;

private:
    enum class Phase { RoundIntro, Fight, RoundOver, MatchOver };

    void StartRound(Game &game);
    void EndRound(Game &game, int winner);   // -1 = hoà
    void UpdatePause(Game &game);
    void DrawPauseMenu() const;
    void DrawResult(Game &game) const;

    Arena arena_;
    Hud   hud_;

    std::unique_ptr<Controller> ctrl1_, ctrl2_;

    Phase phase_       = Phase::RoundIntro;
    float phaseTimer_  = 0.0f;
    float roundClock_  = kRoundSeconds;
    int   round_       = 1;
    int   wins_[2]     = {0, 0};
    int   lastWinner_  = -1;
    int   matchWinner_ = -1;

    bool  paused_      = false;
    int   pauseCursor_ = 0;
    float slowmo_      = 1.0f;
    float koFlash_     = 0.0f;
    bool  debugBoxes_  = false;
};

} // namespace fighter
