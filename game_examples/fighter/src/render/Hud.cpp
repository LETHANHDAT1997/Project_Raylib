#include "render/Hud.hpp"
#include "characters/Roster.hpp"
#include "core/Config.hpp"
#include "core/Text.hpp"
#include "entities/Fighter.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace fighter {

namespace {

constexpr float kBarTop    = 34.0f;
constexpr float kBarHeight = 26.0f;
constexpr float kBarWidth  = 452.0f;
constexpr float kBarMargin = 108.0f;   // chừa chỗ cho khung chân dung

void DrawBevelRect(Rectangle r, Color fill, Color light, Color dark)
{
    DrawRectangleRec(r, fill);
    DrawRectangle((int)r.x, (int)r.y, (int)r.width, 2, light);
    DrawRectangle((int)r.x, (int)(r.y + r.height - 2), (int)r.width, 2, dark);
}

} // namespace

void Hud::Reset()
{
    left_  = HealthBarState{};
    right_ = HealthBarState{};
    time_  = 0.0f;
}

void Hud::Update(const Fighter &p1, const Fighter &p2, float dt)
{
    time_ += dt;

    auto step = [dt](HealthBarState &st, const Fighter &f) {
        const float target = f.HealthRatio();
        st.display = target;                       // thanh chính bám sát ngay
        // Thanh "bóng" tụt chậm để thấy rõ vừa mất bao nhiêu máu.
        if (st.ghost > st.display) st.ghost = std::max(st.display, st.ghost - dt * 0.55f);
        else                        st.ghost = st.display;
        st.meter = f.MeterRatio();
        st.shake = std::max(0.0f, st.shake - dt * 3.0f);
    };

    if (left_.ghost - p1.HealthRatio() > 0.001f) left_.shake = 1.0f;
    if (right_.ghost - p2.HealthRatio() > 0.001f) right_.shake = 1.0f;

    step(left_,  p1);
    step(right_, p2);
}

void Hud::DrawSide(const HealthBarState &st, const Fighter &f,
                   bool leftSide, int wins, int roundsToWin) const
{
    const CharacterDef &def = f.Def();

    const float x = leftSide ? kBarMargin : (kCanvasWidth - kBarMargin - kBarWidth);
    const float y = kBarTop + (st.shake > 0.0f ? std::sin(time_ * 70.0f) * st.shake * 1.6f : 0.0f);

    // --- khung ngoài -------------------------------------------------------
    DrawRectangleRec(Rectangle{x - 4, y - 4, kBarWidth + 8, kBarHeight + 8},
                     Color{16, 14, 22, 220});
    DrawRectangleLinesEx(Rectangle{x - 4, y - 4, kBarWidth + 8, kBarHeight + 8}, 2,
                         Color{224, 206, 150, 220});

    // --- nền thanh máu -----------------------------------------------------
    DrawRectangleRec(Rectangle{x, y, kBarWidth, kBarHeight}, Color{48, 18, 20, 255});

    auto fillRect = [&](float ratio) {
        const float w = kBarWidth * std::clamp(ratio, 0.0f, 1.0f);
        return leftSide ? Rectangle{x + kBarWidth - w, y, w, kBarHeight}
                        : Rectangle{x, y, w, kBarHeight};
    };

    // Dải đỏ (máu vừa mất) rồi mới tới dải vàng (máu thật).
    DrawRectangleRec(fillRect(st.ghost), Color{214, 58, 48, 255});

    const Rectangle hp = fillRect(st.display);
    DrawRectangleGradientEx(hp,
                            Color{255, 226, 96, 255}, Color{236, 168, 34, 255},
                            Color{236, 168, 34, 255}, Color{255, 226, 96, 255});
    // Ánh sáng chạy dọc mép trên thanh máu
    DrawRectangle((int)hp.x, (int)hp.y + 2, (int)hp.width, 4, Color{255, 248, 200, 140});

    DrawRectangleLinesEx(Rectangle{x, y, kBarWidth, kBarHeight}, 2, Color{28, 20, 16, 255});

    // --- tên nhân vật ------------------------------------------------------
    const float nameY = y + kBarHeight + 8.0f;
    if (leftSide) DrawTextBold(def.name.c_str(), Vector2{x + 2, nameY}, 20.0f, Color{255, 236, 190, 255});
    else          DrawTextBold(def.name.c_str(),
                               Vector2{x + kBarWidth - TextWidthBold(def.name.c_str(), 20.0f) - 2, nameY},
                               20.0f, Color{255, 236, 190, 255});

    // --- thanh Super -------------------------------------------------------
    const float mY = nameY + 26.0f;
    const float mW = 260.0f;
    const float mX = leftSide ? x : (x + kBarWidth - mW);
    DrawRectangleRec(Rectangle{mX, mY, mW, 12.0f}, Color{20, 22, 34, 230});

    const float fillW = mW * std::clamp(st.meter, 0.0f, 1.0f);
    const Rectangle mFill = leftSide ? Rectangle{mX, mY, fillW, 12.0f}
                                     : Rectangle{mX + mW - fillW, mY, fillW, 12.0f};
    const bool full = st.meter >= 0.999f;
    Color mc = full
             ? Color{(unsigned char)(200 + 55 * std::sin(time_ * 12.0f)), 240, 255, 255}
             : Color{92, 186, 255, 255};
    DrawRectangleRec(mFill, mc);
    DrawRectangleLinesEx(Rectangle{mX, mY, mW, 12.0f}, 1, Color{150, 180, 220, 200});

    if (full) {
        const char *ready = "SUPER SẴN SÀNG";
        const float tx = leftSide ? mX + mW + 10.0f
                                  : mX - TextWidthBold(ready, 14.0f) - 10.0f;
        DrawTextBold(ready, Vector2{tx, mY - 1.0f}, 14.0f,
                     Color{255, 255, 255, (unsigned char)(160 + 90 * std::sin(time_ * 10.0f))});
    }

    // --- chân dung ---------------------------------------------------------
    const float pw = 92.0f;
    const float px = leftSide ? 8.0f : (kCanvasWidth - pw - 8.0f);
    const Rectangle frame{px, y - 6.0f, pw, pw};
    DrawRectangleRec(frame, Color{14, 14, 22, 230});
    DrawRectangleLinesEx(frame, 2, def.accent);

    const Animation &idle = def.Anim(AnimId::Idle);
    if (idle.texture.id != 0) {
        // Cắt phần thân trên của frame đầu tiên làm ảnh đại diện.
        const float side = (float)idle.texture.height;
        Rectangle src{def.anchor.x - side * 0.14f,
                      def.anchor.y - side * 0.34f,
                      side * 0.28f, side * 0.28f};
        if (!leftSide) src.width = -src.width;
        DrawTexturePro(idle.texture, src,
                       Rectangle{frame.x + 4, frame.y + 4, pw - 8, pw - 8},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    }

    // --- số hiệp đã thắng --------------------------------------------------
    // Đặt sát mép trong của thanh máu, ngang hàng với tên nhân vật.
    for (int i = 0; i < roundsToWin; ++i) {
        const float r  = 8.0f;
        const float gx = leftSide ? (x + kBarWidth - 14.0f - i * 24.0f)
                                  : (x + 14.0f + i * 24.0f);
        const float gy = nameY + 11.0f;
        // Nền tối phía sau: nếu không, vòng tròn rỗng chìm nghỉm khi mặt trăng
        // hay đám mây sáng nằm ngay sau HUD.
        DrawCircle((int)gx, (int)gy, r + 2.0f, Color{12, 10, 18, 210});
        if (i < wins) {
            DrawCircle((int)gx, (int)gy, r, Color{255, 208, 72, 255});
            DrawCircleLines((int)gx, (int)gy, r + 2.0f, Color{255, 246, 200, 200});
        } else {
            DrawCircleLines((int)gx, (int)gy, r, Color{180, 170, 150, 160});
        }
    }
}

void Hud::Draw(const Fighter &p1, const Fighter &p2,
               int timeLeft, int p1Wins, int p2Wins, int roundsToWin) const
{
    DrawSide(left_,  p1, true,  p1Wins, roundsToWin);
    DrawSide(right_, p2, false, p2Wins, roundsToWin);

    // --- đồng hồ giữa màn hình ---------------------------------------------
    const float cx = kCanvasWidth * 0.5f;
    const Rectangle box{cx - 54.0f, kBarTop - 10.0f, 108.0f, 76.0f};
    DrawBevelRect(box, Color{18, 16, 24, 235}, Color{90, 84, 70, 255}, Color{8, 8, 12, 255});
    DrawRectangleLinesEx(box, 2, Color{224, 206, 150, 230});

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d", std::max(0, timeLeft));
    const Color timeColor = (timeLeft <= 10)
        ? Color{255, (unsigned char)(90 + 80 * std::sin(time_ * 14.0f)), 80, 255}
        : Color{255, 236, 190, 255};
    DrawTextBoldCentered(buf, cx, box.y + 12.0f, 46.0f, timeColor);
    DrawTextCentered("TIME", cx, box.y + 56.0f, 13.0f, Color{190, 180, 160, 220});

    DrawComboPopups(p1, p2);
}

void Hud::DrawComboPopups(const Fighter &p1, const Fighter &p2) const
{
    auto popup = [](const Fighter &f, bool leftSide) {
        if (f.ComboCount() < 2) return;
        const float k = std::clamp(f.ComboTimer() / kComboWindow, 0.0f, 1.0f);
        const float x = leftSide ? 120.0f : kCanvasWidth - 300.0f;
        const float y = 178.0f;

        char buf[48];
        std::snprintf(buf, sizeof(buf), "%d HIT COMBO", f.ComboCount());
        const float size = 30.0f + (1.0f - k) * 6.0f;
        Color c = f.Def().accent;
        c.a = (unsigned char)(255 * std::min(1.0f, k * 2.0f));
        DrawTextBold(buf, Vector2{x + 2, y + 2}, size, Color{0, 0, 0, c.a});
        DrawTextBold(buf, Vector2{x, y}, size, c);
    };

    popup(p1, true);
    popup(p2, false);
}

void DrawBanner(const char *text, float t, float duration, Color color)
{
    if (t < 0.0f || t > duration) return;

    const float k = t / duration;
    // Phóng to rồi co lại: vào nhanh, giữ, mờ dần.
    float scale = 1.0f;
    float alpha = 1.0f;
    if (k < 0.18f)      scale = 2.4f - 1.4f * (k / 0.18f);
    else if (k > 0.72f) alpha = 1.0f - (k - 0.72f) / 0.28f;

    const float size = 82.0f * scale;
    const float cx   = kCanvasWidth * 0.5f;
    const float y    = kCanvasHeight * 0.34f - size * 0.5f;

    Color shadow{0, 0, 0, (unsigned char)(180 * alpha)};
    Color main = color;
    main.a = (unsigned char)(255 * alpha);

    DrawTextBoldCentered(text, cx + 5.0f, y + 5.0f, size, shadow);
    DrawTextBoldCentered(text, cx, y, size, main);
}

} // namespace fighter
