#include "scenes/TitleScene.hpp"
#include "Game.hpp"
#include "characters/Roster.hpp"
#include "core/Config.hpp"
#include "core/Text.hpp"
#include "scenes/SelectScene.hpp"
#include <cmath>

namespace fighter {

namespace {
const char *kMenu[] = {"1 NGƯỜI  ·  ĐẤU MÁY", "2 NGƯỜI  ·  ĐẤU NHAU", "THOÁT"};
constexpr int kMenuCount = 3;
} // namespace

void TitleScene::OnEnter(Game &game)
{
    (void)game;
    Roster &roster = Roster::Instance();
    for (int i = 0; i < 4; ++i) {
        const int idx = i % std::max(1, roster.Count());
        demo_[i].Play(&roster.At(idx).Anim(AnimId::Idle), true);
    }
}

void TitleScene::Update(Game &game, float dt)
{
    time_  += dt;
    blink_ += dt;
    for (auto &a : demo_) a.Update(dt);

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) cursor_ = (cursor_ + 1) % kMenuCount;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) cursor_ = (cursor_ + kMenuCount - 1) % kMenuCount;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) {
        switch (cursor_) {
            case 0:
                game.Config().twoPlayers = false;
                game.ChangeScene(std::unique_ptr<Scene>(new SelectScene()));
                break;
            case 1:
                game.Config().twoPlayers = true;
                game.ChangeScene(std::unique_ptr<Scene>(new SelectScene()));
                break;
            default:
                game.RequestExit();
                break;
        }
    }
}

void TitleScene::DrawMenu() const
{
    const float cx = kCanvasWidth * 0.5f;
    float y = 452.0f;

    for (int i = 0; i < kMenuCount; ++i) {
        const bool sel = (i == cursor_);
        const float size = sel ? 30.0f : 25.0f;
        const float w    = TextWidthBold(kMenu[i], size);

        if (sel) {
            const float pulse = 0.5f + 0.5f * std::sin(blink_ * 6.0f);
            DrawRectangleRounded(Rectangle{cx - w * 0.5f - 28.0f, y - 8.0f, w + 56.0f, size + 16.0f},
                                 0.4f, 8, Color{255, 190, 80, (unsigned char)(40 + 40 * pulse)});
            DrawRectangleRoundedLines(Rectangle{cx - w * 0.5f - 28.0f, y - 8.0f, w + 56.0f, size + 16.0f},
                                      0.4f, 8, Color{255, 208, 120, 220});
        }

        DrawTextBoldCentered(kMenu[i], cx + 2.0f, y + 2.0f, size, Color{0, 0, 0, 160});
        DrawTextBoldCentered(kMenu[i], cx, y, size,
                             sel ? Color{255, 232, 170, 255} : Color{190, 184, 176, 230});
        y += size + 30.0f;
    }
}

void TitleScene::Draw(Game &game)
{
    game.Backdrop().Update(time_ * 22.0f, GetFrameTime());
    game.Backdrop().Draw();

    DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight, Color{8, 6, 18, 120});

    // Bốn nhân vật đứng dàn hàng phía sau làm nền.
    Roster &roster = Roster::Instance();
    const int n = std::min(4, roster.Count());
    for (int i = 0; i < n; ++i) {
        const CharacterDef &def = roster.At(i);
        const Animation &a = def.Anim(AnimId::Idle);
        if (a.texture.id == 0) continue;

        const float side  = (float)a.texture.height;
        const float scale = def.scale * 0.92f;
        const float x = kCanvasWidth * (0.18f + 0.215f * i);
        const float y = kGroundY + 8.0f + std::sin(time_ * 1.2f + i) * 3.0f;

        Rectangle src = a.FrameRect(demo_[i].Frame());
        if (i >= n / 2) src.width = -src.width;
        const float anchorX = (i >= n / 2) ? (side - def.anchor.x) : def.anchor.x;

        DrawTexturePro(a.texture, src,
                       Rectangle{x - anchorX * scale, y - def.anchor.y * scale,
                                 side * scale, side * scale},
                       Vector2{0.0f, 0.0f}, 0.0f, Color{120, 120, 150, 190});
    }

    DrawRectangleGradientV(0, 0, kCanvasWidth, 420, Color{8, 6, 20, 210}, Color{8, 6, 20, 60});

    // --- Logo --------------------------------------------------------------
    const float cx = kCanvasWidth * 0.5f;
    const float bob = std::sin(time_ * 1.6f) * 4.0f;

    DrawTextBoldCentered("RAYLIB", cx + 4.0f, 118.0f + bob + 4.0f, 44.0f, Color{0, 0, 0, 170});
    DrawTextBoldCentered("RAYLIB", cx, 118.0f + bob, 44.0f, Color{226, 210, 180, 255});

    DrawTextBoldCentered("ĐẤU SĨ", cx + 6.0f, 166.0f + bob + 6.0f, 104.0f, Color{0, 0, 0, 180});
    DrawTextBoldCentered("ĐẤU SĨ", cx, 166.0f + bob, 104.0f, Color{255, 190, 72, 255});
    DrawTextBoldCentered("ĐẤU SĨ", cx, 162.0f + bob, 104.0f, Color{255, 236, 170, 90});

    DrawTextCentered("Đối kháng 2D · 4 nhân vật · 4 mức độ khó",
                     cx, 292.0f, 22.0f, Color{200, 196, 190, 220});

    // Dải tối phía sau menu để chữ không chìm vào rừng cây.
    DrawRectangleGradientV(0, 400, kCanvasWidth, 120, Color{6, 5, 14, 0}, Color{6, 5, 14, 200});
    DrawRectangle(0, 520, kCanvasWidth, 130, Color{6, 5, 14, 200});
    DrawRectangleGradientV(0, 650, kCanvasWidth, 70, Color{6, 5, 14, 200}, Color{6, 5, 14, 0});

    DrawMenu();

    DrawTextCentered("↑ ↓ chọn  ·  Enter xác nhận  ·  F11 toàn màn hình",
                     cx, kCanvasHeight - 52.0f, 17.0f, Color{160, 156, 150, 200});
}

} // namespace fighter
