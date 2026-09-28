#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace fighter {

class Fighter;

// Một bên thanh máu. Giữ giá trị "trễ" để có dải đỏ tụt dần kiểu game đối kháng.
struct HealthBarState {
    float display = 1.0f;
    float ghost   = 1.0f;
    float ghostHold = 0.0f;    // dải đỏ đứng yên một nhịp rồi mới tụt
    float meter   = 0.0f;
    float shake   = 0.0f;
};

// ============================================================================
// Hud - thanh máu, thanh Super 2 nấc, đồng hồ, số hiệp thắng, bảng combo và
// chữ bật lên (COUNTER, PHẢN ĐÒN...).
// ============================================================================
class Hud {
public:
    void Reset();
    void Update(const Fighter &p1, const Fighter &p2, float dt);
    // timeLeft < 0 = không giới hạn thời gian (luyện tập).
    void Draw(const Fighter &p1, const Fighter &p2,
              int timeLeft, int p1Wins, int p2Wins, int roundsToWin) const;

    void Popup(int side, const std::string &text, Color color);

private:
    struct ComboShow { int count = 0; int damage = 0; float timer = 0.0f; };
    struct PopupText { std::string text; Color color; float life; };

    void DrawSide(const HealthBarState &st, const Fighter &f,
                  bool leftSide, int wins, int roundsToWin) const;
    void DrawCombo(int side, const Fighter &f) const;

    HealthBarState bars_[2];
    ComboShow combo_[2];
    std::vector<PopupText> popups_[2];
    float time_ = 0.0f;
};

// Chữ lớn giữa màn hình ("HIỆP 1", "ĐÁNH!", "K.O.") với hiệu ứng phóng to.
void DrawBanner(const char *text, float t, float duration, Color color);

} // namespace fighter
