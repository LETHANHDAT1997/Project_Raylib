#include "scenes/BattleScene.hpp"
#include "Game.hpp"
#include "characters/Roster.hpp"
#include "core/AI.hpp"
#include "core/Audio.hpp"
#include "core/Text.hpp"
#include "render/MoveList.hpp"
#include "scenes/SelectScene.hpp"
#include "scenes/TitleScene.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace fighter {

namespace {
constexpr float kIntroDuration = 2.4f;
constexpr float kRoundEndHold  = 3.0f;
const char *kPauseItems[] = {"TIẾP TỤC", "BẢNG CHIÊU", "ĐÁNH LẠI", "ĐỔI NHÂN VẬT", "VỀ MÀN CHÍNH"};
constexpr int kPauseCount = 5;

Sfx SwingFor(SfxWeight w) { return w == SfxWeight::Heavy ? Sfx::SwingHeavy : w == SfxWeight::Medium ? Sfx::SwingMedium : Sfx::SwingLight; }
Sfx HitFor(SfxWeight w)   { return w == SfxWeight::Heavy ? Sfx::HitHeavy   : w == SfxWeight::Medium ? Sfx::HitMedium   : Sfx::HitLight; }
} // namespace

// ---------------------------------------------------------------------------
// Khởi tạo
// ---------------------------------------------------------------------------
void BattleScene::OnEnter(Game &game)
{
    const MatchConfig &cfg = game.Config();
    Roster &roster = Roster::Instance();
    training_    = cfg.training;
    twoPlayers_  = cfg.twoPlayers;
    roundsToWin_ = cfg.roundsToWin;

    arena_.SetFighters(roster.Create(cfg.p1Character, true),
                       roster.Create(cfg.p2Character, false));

    // Chơi 2 người thì mũi tên thuộc về người 2; còn lại người 1 dùng được cả hai bộ.
    ctrl1_.reset(new HumanController(twoPlayers_ && !training_ ? KeyBindings::PlayerOne()
                                                               : KeyBindings::PlayerOneSolo()));
    dummy_ = nullptr;
    if (training_) {
        dummy_ = new DummyController();
        ctrl2_.reset(dummy_);
    } else if (twoPlayers_) {
        ctrl2_.reset(new HumanController(KeyBindings::PlayerTwo()));
    } else {
        ctrl2_.reset(new AIController(cfg.difficulty, &arena_));
        const DifficultyProfile &dp = GetDifficulty(cfg.difficulty);
        arena_.P2().SetIncomingDamageScale(dp.damageTaken);
        arena_.P2().SetMeterGainScale(dp.meterGain);
    }

    wins_[0] = wins_[1] = 0;
    round_ = 1;
    matchWinner_ = -1;
    debugBoxes_ = cfg.showHitboxes;
    paused_ = showMoves_ = false;
    StartRound(game);

    Audio::Instance().PlayMusic(MusicTrack::Battle);
}

void BattleScene::OnExit(Game &game)
{
    (void)game;
    Audio::Instance().PlayMusic(MusicTrack::Menu);
}

void BattleScene::StartRound(Game &game)
{
    (void)game;
    arena_.ResetPositions();
    hud_.Reset();
    roundClock_ = kRoundSeconds;
    lastWinner_ = -1;
    timeout_    = false;
    slowmo_     = 1.0f;
    koFlash_    = 0.0f;
    superCutIn_ = 0.0f;
    paused_     = false;

    if (training_) {
        // Luyện tập: vào đánh luôn, không có màn giới thiệu.
        phase_ = Phase::Fight;
        phaseTimer_ = 0.0f;
        arena_.SetInputEnabled(true);
        return;
    }

    phase_ = Phase::RoundIntro;
    phaseTimer_ = 0.0f;
    arena_.SetInputEnabled(false);
    if (round_ == 1) { arena_.P1().PlayIntro(); arena_.P2().PlayIntro(); }

    Audio &au = Audio::Instance();
    const bool finalRound = wins_[0] == roundsToWin_ - 1 && wins_[1] == roundsToWin_ - 1;
    if (finalRound)       au.Say(Line::FinalRound, 0.25f);
    else if (round_ <= 5) au.Say((Line)((int)Line::Round1 + round_ - 1), 0.25f);
    au.Say(Line::Fight, 1.3f);
}

void BattleScene::EndRound(Game &game, int winner, bool timeout)
{
    (void)game;
    lastWinner_ = winner;
    timeout_    = timeout;
    phase_      = Phase::RoundOver;
    phaseTimer_ = 0.0f;
    arena_.SetInputEnabled(false);

    Audio &au = Audio::Instance();
    if (timeout) {
        au.Say(Line::Time);
    } else {
        koFlash_ = 1.0f;
        arena_.Shake(18.0f);
        arena_.SetZoom(1.12f);
        au.Play(Sfx::KOBell, 1.0f);
        const Fighter &loser = (winner == 0) ? arena_.P2() : arena_.P1();
        au.Play(Sfx::VoiceKO, 1.0f, loser.Def().voicePitch);
    }

    if (winner >= 0) {
        wins_[winner] += 1;
        arena_.P1().SetOutcome(winner == 0);
        arena_.P2().SetOutcome(winner == 1);
        const Fighter &w = (winner == 0) ? arena_.P1() : arena_.P2();
        if (w.Health() == w.MaxHealth()) au.Say(Line::Flawless, 1.4f);
    } else {
        arena_.P1().FreezeForRoundEnd();
        arena_.P2().FreezeForRoundEnd();
        au.Say(Line::Tie, 1.0f);
    }
}

// ---------------------------------------------------------------------------
// Sự kiện -> âm thanh + chữ
// ---------------------------------------------------------------------------
void BattleScene::HandleEvents()
{
    Audio &au = Audio::Instance();
    for (const GameEvent &e : arena_.Events()) {
        const Fighter &who   = (e.who == 0) ? arena_.P1() : arena_.P2();
        const Fighter &other = (e.who == 0) ? arena_.P2() : arena_.P1();
        const float pitch = who.Def().voicePitch;

        switch (e.type) {
            case EventType::MoveStart:
                au.Play(SwingFor(e.weight), 0.9f);
                if (e.value == 1) au.Play(Sfx::VoiceShout, 1.0f, pitch);
                break;
            case EventType::SpecialStart:
                au.Play(Sfx::VoiceSpecial, 1.0f, pitch);
                break;
            case EventType::SuperStart:
                au.Play(Sfx::SuperFlash, 1.0f);
                au.Play(Sfx::VoiceSpecial, 1.2f, pitch * 0.95f);
                superCutIn_ = 1.0f;
                superSide_  = e.who;
                superName_  = who.CurrentMove().label;
                break;
            case EventType::Hit:
                au.Play(HitFor(e.weight), 1.0f);
                if (e.weight == SfxWeight::Heavy && GetRandomValue(0, 1) == 0)
                    au.Play(Sfx::VoiceHurt, 1.0f, other.Def().voicePitch);
                break;
            case EventType::CounterHit:
                au.Play(Sfx::HitHeavy, 1.0f);
                au.Play(Sfx::Counter, 0.7f, 1.2f);
                hud_.Popup(e.who, "COUNTER", Color{255, 150, 70, 255});
                break;
            case EventType::Block:     au.Play(Sfx::Block, 1.0f); break;
            case EventType::Throw:     au.Play(Sfx::Throw, 1.0f); au.Play(Sfx::VoiceShout, 1.0f, pitch); break;
            case EventType::ThrowTech:
                au.Play(Sfx::Tech, 1.0f);
                hud_.Popup(1 - e.who, "PHÁ VẬT", Color{160, 220, 255, 255});
                break;
            case EventType::Knockdown: au.Play(Sfx::Knockdown, 0.8f); break;
            case EventType::Land:      au.Play(Sfx::Land, 0.5f); break;
            case EventType::Jump:      au.Play(Sfx::Jump, 0.6f); break;
            case EventType::Dash:      au.Play(Sfx::Dash, 0.8f); break;
            case EventType::Projectile:au.Play(Sfx::Magic, 0.9f); break;
            case EventType::Clash:     au.Play(Sfx::Clash, 1.0f); break;
            case EventType::Thunder:   au.Play(Sfx::Thunder, 1.0f); break;
            case EventType::Teleport:  au.Play(Sfx::Teleport, 1.0f); break;
            case EventType::Counter:
                au.Play(Sfx::Counter, 1.0f);
                hud_.Popup(e.who, "PHẢN ĐÒN!", who.Def().accent);
                break;
            default:
                break;
        }
    }
    arena_.Events().clear();
}

// ---------------------------------------------------------------------------
// Tạm dừng
// ---------------------------------------------------------------------------
void BattleScene::UpdatePause(Game &game)
{
    Audio &au = Audio::Instance();

    if (showMoves_) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D)) {
            movesSide_ = 1 - movesSide_;
            au.Play(Sfx::UiMove);
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ENTER)) {
            showMoves_ = false;
            au.Play(Sfx::UiBack);
        }
        return;
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { pauseCursor_ = (pauseCursor_ + 1) % kPauseCount; au.Play(Sfx::UiMove); }
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) { pauseCursor_ = (pauseCursor_ + kPauseCount - 1) % kPauseCount; au.Play(Sfx::UiMove); }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) {
        au.Play(Sfx::UiConfirm);
        switch (pauseCursor_) {
            case 0: paused_ = false; break;
            case 1: showMoves_ = true; movesSide_ = 0; break;
            case 2: OnEnter(game); break;
            case 3: game.ChangeScene(std::unique_ptr<Scene>(new SelectScene())); break;
            default: game.ChangeScene(std::unique_ptr<Scene>(new TitleScene())); break;
        }
    }
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) { paused_ = false; au.Play(Sfx::UiBack); }
}

// ---------------------------------------------------------------------------
// Luyện tập
// ---------------------------------------------------------------------------
void BattleScene::UpdateTraining(float dt)
{
    (void)dt;
    arena_.P1().RefillForTraining();
    arena_.P2().RefillForTraining();
    if (IsKeyPressed(KEY_R)) {
        arena_.ResetPositions();
        hud_.Reset();
        Audio::Instance().Play(Sfx::UiConfirm);
    }
    if (IsKeyPressed(KEY_T) && dummy_) {
        dummy_->Cycle();
        Audio::Instance().Play(Sfx::UiMove);
    }
}

// ---------------------------------------------------------------------------
// Cập nhật
// ---------------------------------------------------------------------------
void BattleScene::Update(Game &game, float dt)
{
    time_ += dt;
    if (IsKeyPressed(KEY_F1)) debugBoxes_ = !debugBoxes_;

    if (paused_) { UpdatePause(game); return; }
    if ((IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) && phase_ != Phase::MatchOver) {
        paused_ = true;
        pauseCursor_ = 0;
        Audio::Instance().Play(Sfx::UiConfirm);
        return;
    }

    phaseTimer_ += dt;
    koFlash_    = std::max(0.0f, koFlash_ - dt * 1.4f);
    superCutIn_ = std::max(0.0f, superCutIn_ - dt);

    game.Backdrop().Update(arena_.CameraX(), dt);

    switch (phase_) {
        case Phase::RoundIntro:
            arena_.Update(InputState{}, InputState{}, dt);
            hud_.Update(arena_.P1(), arena_.P2(), dt);
            if (phaseTimer_ >= kIntroDuration) {
                phase_ = Phase::Fight;
                phaseTimer_ = 0.0f;
                arena_.SetInputEnabled(true);
            }
            break;

        case Phase::Fight: {
            ctrl1_->Update(arena_.P1(), arena_.P2(), dt);
            ctrl2_->Update(arena_.P2(), arena_.P1(), dt);
            arena_.Update(ctrl1_->State(), ctrl2_->State(), dt);
            hud_.Update(arena_.P1(), arena_.P2(), dt);

            if (training_) { UpdateTraining(dt); break; }

            // Đồng hồ đứng yên trong lúc khựng hình / tung siêu chiêu.
            if (!arena_.Frozen()) roundClock_ = std::max(0.0f, roundClock_ - dt);

            const bool p1Dead = arena_.P1().IsDefeated();
            const bool p2Dead = arena_.P2().IsDefeated();
            if (p1Dead || p2Dead) {
                int winner = -1;
                if (p1Dead && !p2Dead) winner = 1;
                else if (p2Dead && !p1Dead) winner = 0;
                EndRound(game, winner, false);
            } else if (roundClock_ <= 0.0f) {
                const float a = arena_.P1().HealthRatio();
                const float b = arena_.P2().HealthRatio();
                int winner = -1;
                if (a > b + 0.001f)      winner = 0;
                else if (b > a + 0.001f) winner = 1;
                EndRound(game, winner, true);
            }
            break;
        }

        case Phase::RoundOver:
            // Quay chậm cú kết liễu, rồi trả tốc độ và thu zoom.
            slowmo_ = (phaseTimer_ < 1.1f) ? 0.3f : std::min(1.0f, slowmo_ + dt);
            if (phaseTimer_ > 1.1f) arena_.SetZoom(1.0f);
            arena_.Update(InputState{}, InputState{}, dt * slowmo_);
            hud_.Update(arena_.P1(), arena_.P2(), dt);

            if (phaseTimer_ >= kRoundEndHold) {
                if (wins_[0] >= roundsToWin_ || wins_[1] >= roundsToWin_) {
                    matchWinner_ = (wins_[0] >= roundsToWin_) ? 0 : 1;
                    phase_ = Phase::MatchOver;
                    phaseTimer_ = 0.0f;
                    Audio &au = Audio::Instance();
                    if (twoPlayers_) {
                        au.Say(matchWinner_ == 0 ? Line::Player1 : Line::Player2);
                        au.Say(Line::Winner, 0.9f);
                    } else {
                        au.Say(matchWinner_ == 0 ? Line::YouWin : Line::YouLose);
                    }
                } else {
                    ++round_;
                    StartRound(game);
                }
            }
            break;

        case Phase::MatchOver:
            arena_.Update(InputState{}, InputState{}, dt);
            if (phaseTimer_ > 1.0f) {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) {
                    Audio::Instance().Play(Sfx::UiConfirm);
                    game.ChangeScene(std::unique_ptr<Scene>(new SelectScene()));
                }
                if (IsKeyPressed(KEY_R)) { Audio::Instance().Play(Sfx::UiConfirm); OnEnter(game); }
                if (IsKeyPressed(KEY_ESCAPE)) {
                    Audio::Instance().Play(Sfx::UiBack);
                    game.ChangeScene(std::unique_ptr<Scene>(new TitleScene()));
                }
            }
            break;
    }

    HandleEvents();
}

// ---------------------------------------------------------------------------
// Vẽ
// ---------------------------------------------------------------------------
void BattleScene::DrawSuperCutIn() const
{
    if (superCutIn_ <= 0.0f) return;
    const Fighter &f = (superSide_ == 0) ? arena_.P1() : arena_.P2();
    const CharacterDef &def = f.Def();
    const float k = 1.0f - superCutIn_;          // 0 -> 1
    const bool left = superSide_ == 0;

    // Dải chéo màu nhân vật quét ngang màn hình.
    const float slide = (k < 0.2f) ? (1.0f - k / 0.2f) : 0.0f;
    const float alpha = (k > 0.75f) ? (1.0f - (k - 0.75f) / 0.25f) : 1.0f;
    const float cy = kCanvasHeight * 0.44f;
    const float off = (left ? -1.0f : 1.0f) * slide * kCanvasWidth;

    Vector2 a{0 + off, cy - 70}, b{(float)kCanvasWidth + off, cy - 110};
    Vector2 c{(float)kCanvasWidth + off, cy + 40}, d{0 + off, cy + 80};
    Color band = def.accentDeep; band.a = (unsigned char)(230 * alpha);
    DrawTriangle(a, d, c, band);
    DrawTriangle(a, c, b, band);
    Color edge = def.accent; edge.a = (unsigned char)(255 * alpha);
    DrawLineEx(a, b, 4.0f, edge);
    DrawLineEx(d, c, 4.0f, edge);

    // Chân dung lớn trong dải
    const Animation &idle = def.Anim(AnimId::Idle);
    if (idle.texture.id != 0) {
        const float side = (float)idle.texture.height;
        Rectangle src{def.anchor.x - side * 0.16f, def.anchor.y - side * 0.36f, side * 0.32f, side * 0.24f};
        if (!left) src.width = -src.width;
        const float pw = 360.0f, ph = 270.0f;
        const float px = (left ? 80.0f : kCanvasWidth - 80.0f - pw) + off;
        DrawTexturePro(idle.texture, src, Rectangle{px, cy - 150.0f, pw, ph}, Vector2{0, 0}, 0.0f,
                       Color{255, 255, 255, (unsigned char)(255 * alpha)});
    }

    const float tx = left ? kCanvasWidth * 0.58f : kCanvasWidth * 0.42f;
    DrawTextBoldCentered(superName_.c_str(), tx + off + 4, cy - 34 + 4, 58.0f, Color{0, 0, 0, (unsigned char)(180 * alpha)});
    DrawTextBoldCentered(superName_.c_str(), tx + off, cy - 34, 58.0f, Color{255, 248, 225, (unsigned char)(255 * alpha)});
    DrawTextCentered("SIÊU CHIÊU", tx + off, cy + 26, 18.0f, Color{255, 220, 150, (unsigned char)(230 * alpha)});
}

void BattleScene::DrawPauseMenu() const
{
    DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight, Color{0, 0, 0, 180});

    if (showMoves_) {
        const Fighter &f = (movesSide_ == 0) ? arena_.P1() : arena_.P2();
        DrawMoveList(f.Def(), Rectangle{190, 70, 900, 560}, time_);
        DrawTextCentered("← → xem nhân vật bên kia  ·  Esc quay lại", kCanvasWidth * 0.5f, 650, 16,
                         Color{190, 186, 180, 220});
        return;
    }

    const float cx = kCanvasWidth * 0.5f;
    const Rectangle box{cx - 210.0f, 170.0f, 420.0f, 370.0f};
    DrawRectangleRounded(box, 0.06f, 8, Color{16, 15, 24, 240});
    DrawRectangleRoundedLines(box, 0.06f, 8, Color{224, 206, 150, 220});
    DrawTextBoldCentered("TẠM DỪNG", cx, box.y + 26.0f, 32.0f, Color{255, 208, 120, 255});

    float y = box.y + 94.0f;
    for (int i = 0; i < kPauseCount; ++i) {
        const bool sel = (i == pauseCursor_);
        if (sel) {
            DrawRectangleRounded(Rectangle{box.x + 40.0f, y - 6.0f, box.width - 80.0f, 38.0f}, 0.35f, 8,
                                 Color{255, 200, 100, 34});
            DrawRectangleRoundedLines(Rectangle{box.x + 40.0f, y - 6.0f, box.width - 80.0f, 38.0f}, 0.35f, 8,
                                      Color{255, 210, 130, 210});
        }
        DrawTextBoldCentered(kPauseItems[i], cx, y, 22.0f,
                             sel ? Color{255, 232, 170, 255} : Color{186, 182, 178, 230});
        y += 50.0f;
    }
}

void BattleScene::DrawTrainingPanel() const
{
    const Rectangle r{kCanvasWidth * 0.5f - 250.0f, kCanvasHeight - 112.0f, 500.0f, 52.0f};
    DrawRectangleRounded(r, 0.3f, 8, Color{10, 10, 18, 200});
    char line[160];
    std::snprintf(line, sizeof(line), "LUYỆN TẬP  ·  Hình nộm: %s",
                  dummy_ ? DummyController::ModeName(dummy_->CurrentMode()) : "-");
    DrawTextBoldCentered(line, r.x + r.width * 0.5f, r.y + 7.0f, 17.0f, Color{160, 230, 170, 255});
    DrawTextCentered("WASD hoặc mũi tên  ·  T đổi hình nộm  ·  R đặt lại  ·  F1 khung va chạm  ·  P bảng chiêu",
                     r.x + r.width * 0.5f, r.y + 30.0f, 13.0f, Color{180, 176, 172, 220});
}

void BattleScene::DrawResult(Game &game) const
{
    (void)game;
    const float k = std::min(1.0f, phaseTimer_ / 0.5f);
    DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight, Color{0, 0, 0, (unsigned char)(190 * k)});

    const float cx = kCanvasWidth * 0.5f;
    const Fighter &winner = (matchWinner_ == 0) ? arena_.P1() : arena_.P2();

    const char *headline;
    if (twoPlayers_) headline = (matchWinner_ == 0) ? "NGƯỜI 1 THẮNG" : "NGƯỜI 2 THẮNG";
    else             headline = (matchWinner_ == 0) ? "BẠN THẮNG!" : "BẠN THUA";
    const Color hc = (matchWinner_ == 0 || twoPlayers_) ? Color{255, 208, 120, 255} : Color{255, 120, 120, 255};

    DrawTextBoldCentered(headline, cx + 4.0f, 214.0f, 72.0f, Color{0, 0, 0, (unsigned char)(200 * k)});
    DrawTextBoldCentered(headline, cx, 210.0f, 72.0f, Color{hc.r, hc.g, hc.b, (unsigned char)(255 * k)});

    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s  —  %d : %d", winner.Def().name.c_str(), wins_[0], wins_[1]);
    DrawTextBoldCentered(buf, cx, 304.0f, 28.0f, Color{230, 226, 220, (unsigned char)(255 * k)});

    if (!twoPlayers_) {
        std::snprintf(buf, sizeof(buf), "Độ khó: %s", DifficultyName(game.Config().difficulty));
        DrawTextCentered(buf, cx, 346.0f, 19.0f, Color{180, 176, 172, (unsigned char)(230 * k)});
    }

    if (phaseTimer_ > 1.0f) {
        const float pulse = 0.5f + 0.5f * std::sin(phaseTimer_ * 5.0f);
        DrawTextCentered("Enter: chọn lại nhân vật   ·   R: đánh lại   ·   Esc: màn chính",
                         cx, 436.0f, 19.0f, Color{255, 232, 180, (unsigned char)(140 + 110 * pulse)});
    }
}

void BattleScene::Draw(Game &game)
{
    // --- nền (toạ độ màn hình, chỉ rung theo camera) -----------------------
    Camera2D shakeCam{};
    shakeCam.offset = arena_.ShakeOffset();
    shakeCam.zoom = 1.0f;
    BeginMode2D(shakeCam);
        game.Backdrop().Draw();
    EndMode2D();

    // Tung siêu chiêu: nền tối sầm lại, chỉ còn nhân vật sáng.
    if (arena_.SuperFreeze() > 0.0f || superCutIn_ > 0.0f) {
        const float k = std::min(1.0f, std::max(arena_.SuperFreeze(), superCutIn_) * 3.0f);
        DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight, Color{6, 4, 16, (unsigned char)(170 * k)});
    }

    // --- thế giới ----------------------------------------------------------
    BeginMode2D(arena_.Camera());
        arena_.DrawWorld(debugBoxes_);
    EndMode2D();

    if (koFlash_ > 0.0f) {
        DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight, Color{255, 255, 255, (unsigned char)(160 * koFlash_)});
    }

    hud_.Draw(arena_.P1(), arena_.P2(), training_ ? -1 : (int)std::ceil(roundClock_),
              wins_[0], wins_[1], roundsToWin_);

    DrawSuperCutIn();

    switch (phase_) {
        case Phase::RoundIntro: {
            char buf[32];
            const bool finalRound = wins_[0] == roundsToWin_ - 1 && wins_[1] == roundsToWin_ - 1 && round_ > 1;
            if (finalRound) std::snprintf(buf, sizeof(buf), "HIỆP CUỐI");
            else            std::snprintf(buf, sizeof(buf), "HIỆP %d", round_);
            DrawBanner(buf, phaseTimer_, 1.25f, Color{255, 232, 170, 255});
            DrawBanner("ĐÁNH!", phaseTimer_ - 1.3f, 1.0f, Color{255, 110, 80, 255});
            break;
        }
        case Phase::RoundOver:
            if (timeout_)             DrawBanner("HẾT GIỜ", phaseTimer_, kRoundEndHold, Color{200, 214, 255, 255});
            else if (lastWinner_ < 0) DrawBanner("HOÀ",     phaseTimer_, kRoundEndHold, Color{200, 214, 255, 255});
            else                      DrawBanner("K.O.",    phaseTimer_, kRoundEndHold, Color{255, 90, 64, 255});
            break;
        case Phase::MatchOver:
            DrawResult(game);
            break;
        default:
            break;
    }

    if (training_ && !paused_) DrawTrainingPanel();
    if (debugBoxes_ && !training_) {
        DrawTextBold("F1: tắt khung va chạm", Vector2{16.0f, kCanvasHeight - 82.0f}, 15.0f, Color{255, 120, 120, 220});
    }
    if (paused_) DrawPauseMenu();
}

} // namespace fighter
