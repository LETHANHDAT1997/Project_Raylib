#pragma once
#include "raylib.h"
#include <string>

namespace fighter {

class Fighter;

// Một bên thanh máu. Giữ giá trị "trễ" để có dải đỏ tụt dần kiểu game đối kháng.
struct HealthBarState {
    float display = 1.0f;   // tỉ lệ máu hiển thị ngay
    float ghost   = 1.0f;   // tỉ lệ máu "bóng", tụt chậm hơn
    float meter   = 0.0f;
    float shake   = 0.0f;
};

// ============================================================================
// Hud - thanh máu, thanh Super, đồng hồ, số round thắng, chữ combo.
// ============================================================================
class Hud {
public:
    void Reset();
    void Update(const Fighter &p1, const Fighter &p2, float dt);
    void Draw(const Fighter &p1, const Fighter &p2,
              int timeLeft, int p1Wins, int p2Wins, int roundsToWin) const;

    void DrawComboPopups(const Fighter &p1, const Fighter &p2) const;

private:
    void DrawSide(const HealthBarState &st, const Fighter &f,
                  bool leftSide, int wins, int roundsToWin) const;

    HealthBarState left_, right_;
    float time_ = 0.0f;
};

// Chữ lớn giữa màn hình ("ROUND 1", "FIGHT!", "K.O.") với hiệu ứng phóng to.
void DrawBanner(const char *text, float t, float duration, Color color);

} // namespace fighter
