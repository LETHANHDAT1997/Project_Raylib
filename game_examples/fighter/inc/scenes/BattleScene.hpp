#pragma once
#include "raylib.h"
#include "core/Input.hpp"
#include "entities/Arena.hpp"
#include "render/Hud.hpp"
#include "scenes/Scene.hpp"
#include <memory>
#include <string>

namespace fighter {

// ============================================================================
// BattleScene - luật trận: hiệp, đồng hồ, "HIỆP 1 / ĐÁNH! / K.O.", tạm dừng,
// bảng kết quả, chế độ luyện tập. Đọc hàng đợi sự kiện của Arena để phát âm
// thanh và hiện chữ. Vật lý và va chạm nằm ở Arena.
// ============================================================================
class BattleScene final : public Scene {
public:
    void OnEnter(Game &game) override;
    void OnExit(Game &game) override;
    void Update(Game &game, float dt) override;
    void Draw(Game &game) override;

private:
    enum class Phase { RoundIntro, Fight, RoundOver, MatchOver };

    void StartRound(Game &game);
    void EndRound(Game &game, int winner, bool timeout);
    void HandleEvents();
    void UpdatePause(Game &game);
    void UpdateTraining(float dt);

    void DrawSuperCutIn() const;
    void DrawPauseMenu() const;
    void DrawResult(Game &game) const;
    void DrawTrainingPanel() const;

    Arena arena_;
    Hud   hud_;

    std::unique_ptr<Controller> ctrl1_, ctrl2_;
    DummyController *dummy_ = nullptr;   // trỏ vào ctrl2_ khi luyện tập

    Phase phase_       = Phase::RoundIntro;
    float phaseTimer_  = 0.0f;
    float roundClock_  = kRoundSeconds;
    int   round_       = 1;
    int   wins_[2]     = {0, 0};
    int   lastWinner_  = -1;
    int   matchWinner_ = -1;
    bool  timeout_     = false;
    bool  training_    = false;
    bool  twoPlayers_  = false;
    int   roundsToWin_ = 2;

    bool  paused_      = false;
    int   pauseCursor_ = 0;
    bool  showMoves_   = false;
    int   movesSide_   = 0;

    float slowmo_      = 1.0f;
    float koFlash_     = 0.0f;
    bool  debugBoxes_  = false;

    float superCutIn_  = 0.0f;
    int   superSide_   = 0;
    std::string superName_;
    float time_        = 0.0f;
};

} // namespace fighter
