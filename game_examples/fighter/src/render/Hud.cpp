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
constexpr float kBarMargin = 108.0f;
}

void Hud::Reset()
{
    for (auto &b : bars_) b = HealthBarState{};
    for (auto &c : combo_) c = ComboShow{};
    for (auto &p : popups_) p.clear();
    time_ = 0.0f;
}

void Hud::Popup(int side, const std::string &text, Color color)
{
    if (side < 0 || side > 1) return;
    popups_[side].push_back({text, color, 1.1f});
    if (popups_[side].size() > 3) popups_[side].erase(popups_[side].begin());
}

void Hud::Update(const Fighter &p1, const Fighter &p2, float dt)
{
    time_ += dt;
    const Fighter *fs[2] = {&p1, &p2};

    for (int i = 0; i < 2; ++i) {
        HealthBarState &st = bars_[i];
        const float target = fs[i]->HealthRatio();
        if (target < st.display - 0.001f) { st.shake = 1.0f; st.ghostHold = 0.45f; }
        st.display = target;
        if (st.ghost > st.display) {
            // Dải đỏ giữ nguyên trong lúc combo còn tiếp diễn, rồi mới tụt.
            st.ghostHold -= dt;
            if (st.ghostHold <= 0.0f) st.ghost = std::max(st.display, st.ghost - dt * 0.6f);
        } else {
            st.ghost = st.display;
        }
        st.meter = fs[i]->Meter() / Fighter::kMaxMeterValue;
        st.shake = std::max(0.0f, st.shake - dt * 3.0f);

        // Bảng combo: bám theo combo đang chạy, giữ lại một lúc khi đã dứt.
        ComboShow &c = combo_[i];
        if (fs[i]->ComboCount() >= 2) {
            c.count = fs[i]->ComboCount();
            c.damage = fs[i]->ComboDamage();
            c.timer = 1.4f;
        } else {
            c.timer = std::max(0.0f, c.timer - dt);
        }

        for (auto &p : popups_[i]) p.life -= dt;
        popups_[i].erase(std::remove_if(popups_[i].begin(), popups_[i].end(),
                                        [](const PopupText &p) { return p.life <= 0.0f; }),
                         popups_[i].end());
    }
}

void Hud::DrawSide(const HealthBarState &st, const Fighter &f,
                   bool leftSide, int wins, int roundsToWin) const
{
    const CharacterDef &def = f.Def();
    const float x = leftSide ? kBarMargin : (kCanvasWidth - kBarMargin - kBarWidth);
    const float y = kBarTop + (st.shake > 0.0f ? std::sin(time_ * 70.0f) * st.shake * 1.6f : 0.0f);

    // --- khung + thanh máu -------------------------------------------------
    DrawRectangleRec(Rectangle{x - 4, y - 4, kBarWidth + 8, kBarHeight + 8}, Color{16, 14, 22, 220});
    DrawRectangleLinesEx(Rectangle{x - 4, y - 4, kBarWidth + 8, kBarHeight + 8}, 2, Color{224, 206, 150, 220});
    DrawRectangleRec(Rectangle{x, y, kBarWidth, kBarHeight}, Color{48, 18, 20, 255});

    auto fillRect = [&](float ratio) {
        const float w = kBarWidth * std::clamp(ratio, 0.0f, 1.0f);
        return leftSide ? Rectangle{x + kBarWidth - w, y, w, kBarHeight}
                        : Rectangle{x, y, w, kBarHeight};
    };

    DrawRectangleRec(fillRect(st.ghost), Color{220, 60, 48, 255});
    const Rectangle hp = fillRect(st.display);
    const bool danger = st.display < 0.25f;
    const Color top = danger ? Color{255, 120, 80, 255} : Color{255, 226, 96, 255};
    const Color bot = danger ? Color{220, 60, 40, 255}  : Color{236, 168, 34, 255};
    DrawRectangleGradientEx(hp, top, bot, bot, top);
    DrawRectangle((int)hp.x, (int)hp.y + 2, (int)hp.width, 4, Color{255, 248, 200, 140});
    DrawRectangleLinesEx(Rectangle{x, y, kBarWidth, kBarHeight}, 2, Color{28, 20, 16, 255});

    // --- tên ---------------------------------------------------------------
    const float nameY = y + kBarHeight + 8.0f;
    const float nameW = TextWidthBold(def.name.c_str(), 20.0f);
    DrawTextBold(def.name.c_str(), Vector2{leftSide ? x + 2 : x + kBarWidth - nameW - 2, nameY},
                 20.0f, Color{255, 236, 190, 255});

    // --- hiệp đã thắng (ngang hàng tên, sát mép trong) ----------------------
    for (int i = 0; i < roundsToWin; ++i) {
        const float gx = leftSide ? (x + kBarWidth - 14.0f - i * 24.0f) : (x + 14.0f + i * 24.0f);
        const float gy = nameY + 11.0f;
        DrawCircle((int)gx, (int)gy, 10.0f, Color{12, 10, 18, 210});
        if (i < wins) {
            DrawCircle((int)gx, (int)gy, 8.0f, Color{255, 208, 72, 255});
            DrawCircleLines((int)gx, (int)gy, 10.0f, Color{255, 246, 200, 200});
        } else {
            DrawCircleLines((int)gx, (int)gy, 8.0f, Color{180, 170, 150, 160});
        }
    }

    // --- chân dung ---------------------------------------------------------
    const float pw = 92.0f;
    const float px = leftSide ? 8.0f : (kCanvasWidth - pw - 8.0f);
    const Rectangle frame{px, kBarTop - 6.0f, pw, pw};
    DrawRectangleRec(frame, Color{14, 14, 22, 230});
    DrawRectangleLinesEx(frame, 2, def.accent);
    const Animation &idle = def.Anim(AnimId::Idle);
    if (idle.texture.id != 0) {
        const float side = (float)idle.texture.height;
        Rectangle src{def.anchor.x - side * 0.14f, def.anchor.y - side * 0.34f, side * 0.28f, side * 0.28f};
        if (!leftSide) src.width = -src.width;
        DrawTexturePro(idle.texture, src, Rectangle{frame.x + 4, frame.y + 4, pw - 8, pw - 8},
                       Vector2{0, 0}, 0.0f, WHITE);
    }

    // --- thanh Super 2 nấc (dưới cùng màn hình, kiểu SF) --------------------
    const float mW = 300.0f, mH = 16.0f;
    const float mX = leftSide ? 40.0f : kCanvasWidth - 40.0f - mW;
    const float mY = kCanvasHeight - 46.0f;
    DrawRectangleRec(Rectangle{mX - 3, mY - 3, mW + 6, mH + 6}, Color{10, 10, 18, 220});
    const int stocks = f.MeterStocks();
    for (int s = 0; s < 2; ++s) {
        const float segW = (mW - 4.0f) * 0.5f;
        const float sx = leftSide ? mX + s * (segW + 4.0f) : mX + mW - (s + 1) * segW - s * 4.0f;
        const float fill = std::clamp(st.meter * 2.0f - s, 0.0f, 1.0f);
        DrawRectangleRec(Rectangle{sx, mY, segW, mH}, Color{26, 30, 48, 255});
        const bool full = fill >= 0.999f;
        const Color c = full ? Color{(unsigned char)(150 + 100 * std::fabs(std::sin(time_ * 5.0f + s))), 225, 255, 255}
                             : Color{70, 140, 230, 255};
        const float w = segW * fill;
        DrawRectangleRec(leftSide ? Rectangle{sx, mY, w, mH} : Rectangle{sx + segW - w, mY, w, mH}, c);
        DrawRectangleLinesEx(Rectangle{sx, mY, segW, mH}, 1, Color{150, 180, 230, 200});
    }
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%d", stocks);
    const float lx = leftSide ? mX + mW + 12.0f : mX - 34.0f;
    DrawTextBold(buf, Vector2{lx, mY - 8.0f}, 30.0f,
                 stocks > 0 ? Color{160, 230, 255, 255} : Color{90, 100, 120, 220});
    DrawText("SUPER", Vector2{leftSide ? mX : mX + mW - TextWidth("SUPER", 12.0f), mY - 16.0f}, 12.0f,
             Color{170, 190, 220, 220});
}

void Hud::DrawCombo(int side, const Fighter &f) const
{
    (void)f;
    const ComboShow &c = combo_[side];
    const bool left = side == 0;
    const float x = left ? 40.0f : kCanvasWidth - 40.0f;

    if (c.timer > 0.0f && c.count >= 2) {
        const float a = std::min(1.0f, c.timer / 0.4f);
        char num[16], dmg[32];
        std::snprintf(num, sizeof(num), "%d", c.count);
        std::snprintf(dmg, sizeof(dmg), "%d SÁT THƯƠNG", c.damage);
        const float big = 58.0f;
        const float nw = TextWidthBold(num, big);
        const float hw = TextWidthBold("HITS", 24.0f);
        const float bx = left ? x : x - nw - hw - 8.0f;
        Color accent = Color{255, 214, 90, (unsigned char)(255 * a)};
        DrawTextBold(num, Vector2{bx + 3, 172 + 3}, big, Color{0, 0, 0, (unsigned char)(170 * a)});
        DrawTextBold(num, Vector2{bx, 172}, big, accent);
        DrawTextBold("HITS", Vector2{bx + nw + 8, 204}, 24.0f, Color{255, 240, 210, (unsigned char)(255 * a)});
        const float dw = TextWidth(dmg, 15.0f);
        DrawText(dmg, Vector2{left ? x : x - dw, 236}, 15.0f, Color{230, 220, 210, (unsigned char)(220 * a)});
    }

    float py = 262.0f;
    for (const auto &p : popups_[side]) {
        const float a = std::min(1.0f, p.life / 0.3f);
        const float slide = (1.1f - p.life) * 12.0f;
        const float size = 26.0f;
        const float w = TextWidthBold(p.text.c_str(), size);
        const float px = left ? x + slide : x - w - slide;
        DrawTextBold(p.text.c_str(), Vector2{px + 2, py + 2}, size, Color{0, 0, 0, (unsigned char)(170 * a)});
        DrawTextBold(p.text.c_str(), Vector2{px, py}, size,
                     Color{p.color.r, p.color.g, p.color.b, (unsigned char)(255 * a)});
        py += 32.0f;
    }
}

void Hud::Draw(const Fighter &p1, const Fighter &p2,
               int timeLeft, int p1Wins, int p2Wins, int roundsToWin) const
{
    DrawSide(bars_[0], p1, true,  p1Wins, roundsToWin);
    DrawSide(bars_[1], p2, false, p2Wins, roundsToWin);

    const float cx = kCanvasWidth * 0.5f;
    const Rectangle box{cx - 54.0f, kBarTop - 10.0f, 108.0f, 76.0f};
    DrawRectangleRec(box, Color{18, 16, 24, 235});
    DrawRectangleLinesEx(box, 2, Color{224, 206, 150, 230});

    if (timeLeft < 0) {
        DrawTextBoldCentered("∞", cx, box.y + 6.0f, 50.0f, Color{255, 236, 190, 255});
    } else {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d", timeLeft);
        const Color c = (timeLeft <= 10)
            ? Color{255, (unsigned char)(90 + 80 * std::sin(time_ * 14.0f)), 80, 255}
            : Color{255, 236, 190, 255};
        DrawTextBoldCentered(buf, cx, box.y + 12.0f, 46.0f, c);
    }
    DrawTextCentered("TIME", cx, box.y + 56.0f, 13.0f, Color{190, 180, 160, 220});

    DrawCombo(0, p1);
    DrawCombo(1, p2);
}

void DrawBanner(const char *text, float t, float duration, Color color)
{
    if (t < 0.0f || t > duration) return;

    const float k = t / duration;
    float scale = 1.0f, alpha = 1.0f;
    if (k < 0.15f)      scale = 2.4f - 1.4f * (k / 0.15f);
    else if (k > 0.75f) alpha = 1.0f - (k - 0.75f) / 0.25f;

    const float size = 88.0f * scale;
    const float cx   = kCanvasWidth * 0.5f;
    const float y    = kCanvasHeight * 0.36f - size * 0.5f;

    DrawTextBoldCentered(text, cx + 5.0f, y + 5.0f, size, Color{0, 0, 0, (unsigned char)(180 * alpha)});
    DrawTextBoldCentered(text, cx, y, size, Color{color.r, color.g, color.b, (unsigned char)(255 * alpha)});
}

} // namespace fighter
