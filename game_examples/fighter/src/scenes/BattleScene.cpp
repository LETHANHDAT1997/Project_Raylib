#include "scenes/BattleScene.hpp"
#include "Game.hpp"
#include "characters/Roster.hpp"
#include "core/Text.hpp"
#include "scenes/SelectScene.hpp"
#include "scenes/TitleScene.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace fighter {

namespace {
constexpr float kIntroDuration = 2.3f;
constexpr float kRoundEndHold  = 2.6f;
const char *kPauseItems[] = {"TIẾP TỤC", "ĐÁNH LẠI", "ĐỔI NHÂN VẬT", "VỀ MÀN CHÍNH"};
constexpr int kPauseCount = 4;
} // namespace

void BattleScene::OnEnter(Game &game)
{
    const MatchConfig &cfg = game.Config();
    Roster &roster = Roster::Instance();

    arena_.SetFighters(roster.Create(cfg.p1Character, true),
                       roster.Create(cfg.p2Character, false));

    ctrl1_.reset(new HumanController(KeyBindings::PlayerOne()));
    if (cfg.twoPlayers) ctrl2_.reset(new HumanController(KeyBindings::PlayerTwo()));
    else                ctrl2_.reset(new AIController(cfg.difficulty));

    // Độ khó điều chỉnh cả sức chịu đòn lẫn tốc độ hồi Super của máy.
    if (!cfg.twoPlayers) {
        const DifficultyProfile &dp = GetDifficulty(cfg.difficulty);
        arena_.P2().SetIncomingDamageScale(dp.damageTaken);
        arena_.P2().SetMeterGainScale(dp.meterGain);
    }

    wins_[0] = wins_[1] = 0;
    round_ = 1;
    matchWinner_ = -1;
    debugBoxes_ = cfg.showHitboxes;
    hud_.Reset();
    StartRound(game);
}

void BattleScene::StartRound(Game &game)
{
    (void)game;
    arena_.ResetPositions();
    arena_.SetInputEnabled(false);
    hud_.Reset();
    phase_      = Phase::RoundIntro;
    phaseTimer_ = 0.0f;
    roundClock_ = kRoundSeconds;
    lastWinner_ = -1;
    slowmo_     = 1.0f;
    koFlash_    = 0.0f;
    paused_     = false;
}

void BattleScene::EndRound(Game &game, int winner)
{
    (void)game;
    lastWinner_ = winner;
    phase_      = Phase::RoundOver;
    phaseTimer_ = 0.0f;
    koFlash_    = 1.0f;
    arena_.SetInputEnabled(false);
    arena_.Shake(16.0f);

    if (winner >= 0) {
        wins_[winner] += 1;
        arena_.P1().SetOutcome(winner == 0);
        arena_.P2().SetOutcome(winner == 1);
    } else {
        arena_.P1().FreezeForRoundEnd();
        arena_.P2().FreezeForRoundEnd();
    }
}

void BattleScene::UpdatePause(Game &game)
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) pauseCursor_ = (pauseCursor_ + 1) % kPauseCount;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) pauseCursor_ = (pauseCursor_ + kPauseCount - 1) % kPauseCount;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) {
        switch (pauseCursor_) {
            case 0: paused_ = false; break;
            case 1: OnEnter(game); break;
            case 2: game.ChangeScene(std::unique_ptr<Scene>(new SelectScene())); break;
            default: game.ChangeScene(std::unique_ptr<Scene>(new TitleScene())); break;
        }
    }
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) paused_ = false;
}

void BattleScene::Update(Game &game, float dt)
{
    if (IsKeyPressed(KEY_F1)) debugBoxes_ = !debugBoxes_;

    if (paused_) { UpdatePause(game); return; }

    if ((IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) && phase_ != Phase::MatchOver) {
        paused_ = true;
        pauseCursor_ = 0;
        return;
    }

    phaseTimer_ += dt;
    koFlash_ = std::max(0.0f, koFlash_ - dt * 1.6f);

    // Camera bám điểm giữa hai đấu sĩ để nền cuộn theo trận đánh.
    const float mid = (arena_.P1().Position().x + arena_.P2().Position().x) * 0.5f;
    game.Backdrop().Update(mid - kCanvasWidth * 0.5f, dt);

    switch (phase_) {
        case Phase::RoundIntro: {
            arena_.Update(InputState{}, InputState{}, dt);
            hud_.Update(arena_.P1(), arena_.P2(), dt);
            if (phaseTimer_ >= kIntroDuration) {
                phase_ = Phase::Fight;
                phaseTimer_ = 0.0f;
                arena_.SetInputEnabled(true);
            }
            break;
        }

        case Phase::Fight: {
            ctrl1_->Update(arena_.P1(), arena_.P2(), dt);
            ctrl2_->Update(arena_.P2(), arena_.P1(), dt);
            arena_.Update(ctrl1_->State(), ctrl2_->State(), dt);
            hud_.Update(arena_.P1(), arena_.P2(), dt);

            roundClock_ = std::max(0.0f, roundClock_ - dt);

            const bool p1Dead = arena_.P1().IsDefeated();
            const bool p2Dead = arena_.P2().IsDefeated();

            if (p1Dead || p2Dead) {
                int winner = -1;
                if (p1Dead && !p2Dead) winner = 1;
                else if (p2Dead && !p1Dead) winner = 0;
                EndRound(game, winner);
            } else if (roundClock_ <= 0.0f) {
                // Hết giờ: bên nào còn nhiều máu hơn (theo tỉ lệ) thì thắng hiệp.
                const float a = arena_.P1().HealthRatio();
                const float b = arena_.P2().HealthRatio();
                int winner = -1;
                if (a > b + 0.001f)      winner = 0;
                else if (b > a + 0.001f) winner = 1;
                EndRound(game, winner);
            }
            break;
        }

        case Phase::RoundOver: {
            // Quay chậm dần để thấy rõ cú kết liễu.
            slowmo_ = std::max(0.35f, slowmo_ - dt * 0.9f);
            arena_.Update(InputState{}, InputState{}, dt * slowmo_);
            hud_.Update(arena_.P1(), arena_.P2(), dt);

            if (phaseTimer_ >= kRoundEndHold) {
                const int need = game.Config().roundsToWin;
                if (wins_[0] >= need || wins_[1] >= need) {
                    matchWinner_ = (wins_[0] >= need) ? 0 : 1;
                    phase_ = Phase::MatchOver;
                    phaseTimer_ = 0.0f;
                } else {
                    ++round_;
                    StartRound(game);
                }
            }
            break;
        }

        case Phase::MatchOver: {
            arena_.Update(InputState{}, InputState{}, dt * 0.6f);
            if (phaseTimer_ > 0.8f) {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) {
                    game.ChangeScene(std::unique_ptr<Scene>(new SelectScene()));
                }
                if (IsKeyPressed(KEY_R)) OnEnter(game);
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game.ChangeScene(std::unique_ptr<Scene>(new TitleScene()));
                }
            }
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Vẽ
// ---------------------------------------------------------------------------
void BattleScene::DrawPauseMenu() const
{
    DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight, Color{0, 0, 0, 175});

    const float cx = kCanvasWidth * 0.5f;
    const Rectangle box{cx - 200.0f, 200.0f, 400.0f, 300.0f};
    DrawRectangleRounded(box, 0.06f, 8, Color{16, 15, 24, 240});
    DrawRectangleRoundedLines(box, 0.06f, 8, Color{224, 206, 150, 220});

    DrawTextBoldCentered("TẠM DỪNG", cx, box.y + 26.0f, 32.0f, Color{255, 208, 120, 255});

    float y = box.y + 92.0f;
    for (int i = 0; i < kPauseCount; ++i) {
        const bool sel = (i == pauseCursor_);
        if (sel) {
            DrawRectangleRounded(Rectangle{box.x + 40.0f, y - 6.0f, box.width - 80.0f, 36.0f},
                                 0.35f, 8, Color{255, 200, 100, 34});
            DrawRectangleRoundedLines(Rectangle{box.x + 40.0f, y - 6.0f, box.width - 80.0f, 36.0f},
                                      0.35f, 8, Color{255, 210, 130, 210});
        }
        DrawTextBoldCentered(kPauseItems[i], cx, y, 22.0f,
                             sel ? Color{255, 232, 170, 255} : Color{186, 182, 178, 230});
        y += 46.0f;
    }
}

void BattleScene::DrawResult(Game &game) const
{
    const float k = std::min(1.0f, phaseTimer_ / 0.5f);
    DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight,
                  Color{0, 0, 0, (unsigned char)(190 * k)});

    const float cx = kCanvasWidth * 0.5f;
    const Fighter &winner = (matchWinner_ == 0) ? arena_.P1() : arena_.P2();

    const char *headline;
    if (game.Config().twoPlayers) headline = (matchWinner_ == 0) ? "NGƯỜI 1 THẮNG" : "NGƯỜI 2 THẮNG";
    else                          headline = (matchWinner_ == 0) ? "BẠN THẮNG!" : "BẠN THUA";

    const Color hc = (matchWinner_ == 0 || game.Config().twoPlayers)
                   ? Color{255, 208, 120, 255} : Color{255, 120, 120, 255};

    DrawTextBoldCentered(headline, cx + 4.0f, 214.0f, 68.0f, Color{0, 0, 0, (unsigned char)(200 * k)});
    DrawTextBoldCentered(headline, cx, 210.0f, 68.0f,
                         Color{hc.r, hc.g, hc.b, (unsigned char)(255 * k)});

    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s  —  %d : %d",
                  winner.Def().name.c_str(), wins_[0], wins_[1]);
    DrawTextBoldCentered(buf, cx, 300.0f, 28.0f, Color{230, 226, 220, (unsigned char)(255 * k)});

    if (!game.Config().twoPlayers) {
        std::snprintf(buf, sizeof(buf), "Độ khó: %s", DifficultyName(game.Config().difficulty));
        DrawTextCentered(buf, cx, 342.0f, 19.0f,
                         Color{180, 176, 172, (unsigned char)(230 * k)});
    }

    if (phaseTimer_ > 0.8f) {
        const float pulse = 0.5f + 0.5f * std::sin(phaseTimer_ * 5.0f);
        DrawTextCentered("Enter: chọn lại nhân vật   ·   R: đánh lại   ·   Esc: màn chính",
                         cx, 432.0f, 19.0f,
                         Color{255, 232, 180, (unsigned char)(140 + 110 * pulse)});
    }
}

void BattleScene::Draw(Game &game)
{
    // Camera2D chỉ dùng để dịch cả khung hình khi rung - HUD vẽ ngoài nên đứng yên.
    Camera2D cam{};
    cam.offset = arena_.ShakeOffset();
    cam.target = Vector2{0.0f, 0.0f};
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;

    BeginMode2D(cam);
        game.Backdrop().Draw();
        arena_.DrawWorld(debugBoxes_);
    EndMode2D();

    // Chớp trắng lúc K.O.
    if (koFlash_ > 0.0f) {
        DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight,
                      Color{255, 255, 255, (unsigned char)(150 * koFlash_)});
    }

    hud_.Draw(arena_.P1(), arena_.P2(), (int)std::ceil(roundClock_),
              wins_[0], wins_[1], game.Config().roundsToWin);

    switch (phase_) {
        case Phase::RoundIntro: {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "HIỆP %d", round_);
            DrawBanner(buf, phaseTimer_, 1.2f, Color{255, 232, 170, 255});
            DrawBanner("ĐÁNH!", phaseTimer_ - 1.25f, 1.0f, Color{255, 120, 90, 255});
            break;
        }

        case Phase::RoundOver: {
            if (lastWinner_ < 0) DrawBanner("HẾT GIỜ", phaseTimer_, kRoundEndHold, Color{200, 214, 255, 255});
            else                 DrawBanner("K.O.",    phaseTimer_, kRoundEndHold, Color{255, 96, 72, 255});
            break;
        }

        case Phase::MatchOver:
            DrawResult(game);
            break;

        default:
            break;
    }

    if (debugBoxes_) {
        DrawTextBold("F1: tắt khung va chạm", Vector2{16.0f, kCanvasHeight - 30.0f}, 16.0f,
                     Color{255, 120, 120, 220});
    }

    if (paused_) DrawPauseMenu();
}

} // namespace fighter
