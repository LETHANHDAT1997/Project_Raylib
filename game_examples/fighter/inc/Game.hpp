#pragma once
#include "raylib.h"
#include "core/Difficulty.hpp"
#include "render/Background.hpp"
#include "scenes/Scene.hpp"
#include <memory>

namespace fighter {

// Lựa chọn của người chơi ở màn Settings, truyền sang màn đấu.
struct MatchConfig {
    int  p1Character = 0;
    int  p2Character = 1;
    Difficulty difficulty = Difficulty::Normal;
    int  roundsToWin = 2;      // 1 / 2 / 3 -> BO1 / BO3 / BO5
    bool twoPlayers  = false;  // true: người thứ hai cầm phím, false: đánh máy
    bool training    = false;  // luyện tập: không giới hạn giờ, hồi máu, hình nộm
    bool showHitboxes = false; // bật khung debug bằng F1
};

// ============================================================================
// Game - vỏ ngoài: giữ scene hiện tại, cấu hình trận và tài nguyên dùng chung.
// ============================================================================
class Game {
public:
    Game();
    ~Game();

    void Init();
    void Update(float dt);
    void Draw();
    void Shutdown();

    // Đổi màn hình (thực hiện cuối frame để tránh xoá scene đang chạy).
    void ChangeScene(std::unique_ptr<Scene> next);

    MatchConfig &Config() { return config_; }
    const MatchConfig &Config() const { return config_; }
    Background &Backdrop() { return background_; }

    bool ShouldExit() const { return exitRequested_; }
    void RequestExit() { exitRequested_ = true; }

private:
    void ApplyPendingScene();

    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Scene> pending_;
    MatchConfig config_{};
    Background  background_;
    bool exitRequested_ = false;
    bool initialized_   = false;
};

} // namespace fighter
