#pragma once
#include "raylib.h"
#include "core/Animation.hpp"

namespace fighter {

// ============================================================================
// Toàn bộ đòn mà một nhân vật có thể tung ra. Đòn thường (Normal) có 9 biến thể
// theo tư thế x lực; 4 chiêu đặc biệt + 1 siêu chiêu do từng lớp con định nghĩa.
// ============================================================================
enum class MoveId {
    StandL, StandM, StandH,
    CrouchL, CrouchM, CrouchH,
    AirL, AirM, AirH,
    Throw,
    SpecialA,   // 236 + đòn   | Special
    SpecialB,   // 214 + đòn   | Special + tiến
    SpecialC,   // 623 + đòn   | Special + xuống
    SpecialD,   // 22  + đòn   | Special + lùi
    Super,      // 236236 + đòn | Super
    Count
};

enum class Strength { Light = 0, Medium, Heavy };

// Độ cao đòn quyết định cách đỡ:
//   Mid      - đứng hay ngồi đỡ đều được
//   Low      - phải NGỒI đỡ (quét chân)
//   Overhead - phải ĐỨNG đỡ (đòn nhảy, chém bổ)
//   Throw    - không đỡ được, chỉ phá được bằng cách vật lại
enum class HitHeight { Mid, Low, Overhead, Throw };

enum class Pose { Stand, Crouch, Air };

// Kiểu vệt chém vẽ theo đòn - thứ giúp cùng một animation trông thành nhiều
// đòn khác nhau.
enum class SlashFx { None, Jab, Horizontal, Overhead, Low, Sweep, Rising, AirDiag, Spin, Thrust };

enum class SfxWeight { Light, Medium, Heavy };

bool IsNormal(MoveId id);
bool IsSpecial(MoveId id);

// ============================================================================
// MoveDef - frame data của một đòn: startup (vung tay) -> active (hitbox bật)
// -> recovery (thu đòn), cộng mọi thuộc tính mà hệ thống combo cần.
// ============================================================================
struct MoveDef {
    MoveId   id       = MoveId::StandL;
    Strength strength = Strength::Light;

    // Hình ảnh --------------------------------------------------------------
    AnimId anim      = AnimId::Attack1;
    int    frameFrom = 0;
    int    frameTo   = -1;          // -1 = hết animation
    Pose   pose      = Pose::Stand;
    SlashFx fx       = SlashFx::None;
    float  fxRadius  = 110.0f;
    Color  fxColor   = {255, 255, 255, 0};   // alpha 0 = dùng màu nhân vật

    // Thời gian -------------------------------------------------------------
    float startup  = 0.10f;
    float active   = 0.08f;
    float recovery = 0.18f;

    // Hitbox: gốc là điểm giữa 2 bàn chân, hướng mặt sang phải.
    Rectangle box = {40.0f, -150.0f, 90.0f, 60.0f};

    // Sát thương ------------------------------------------------------------
    int   damage    = 60;
    int   chip      = 0;            // mất máu dù đỡ được (chiêu đặc biệt)
    float knockback = 240.0f;
    float hitstun   = 0.28f;
    float blockstun = 0.16f;
    HitHeight height = HitHeight::Mid;
    bool  knockdown = false;        // ngã hẳn xuống đất
    bool  launcher  = false;        // hất tung lên (cho phép tung hứng)
    float launchVy  = -780.0f;
    int   hits      = 1;
    float hitSpacing = 0.06f;
    bool  finisherKnockdown = false;  // đòn nhiều hit: hit cuối làm ngã

    // Thanh Super -----------------------------------------------------------
    float meterGain = 6.0f;
    float meterCost = 0.0f;

    // Hủy đòn (cancel) - linh hồn của combo ---------------------------------
    bool chainable     = false;     // nối sang đòn thường mạnh hơn
    bool specialCancel = false;     // hủy sang chiêu đặc biệt khi trúng/đỡ
    bool superCancel   = false;     // hủy sang siêu chiêu

    // Di chuyển -------------------------------------------------------------
    bool    setStartVel  = false;   // đặt vận tốc lúc bắt đầu đòn
    Vector2 startVel     = {0.0f, 0.0f};
    bool    setActiveVel = false;   // đặt vận tốc lúc hitbox bật
    Vector2 activeVel    = {0.0f, 0.0f};
    bool    noGravity    = false;   // lơ lửng trong suốt đòn
    bool    keepMomentum = false;   // đòn trên không: giữ quán tính nhảy
    bool    spin         = false;   // xoay người liên tục (đổi hướng mặt)
    bool    landCancel   = false;   // chạm đất thì kết thúc đòn ngay
    bool    passThrough  = false;   // lướt xuyên qua người đối thủ

    // Phòng thủ -------------------------------------------------------------
    float invulnFrom = 0.0f;        // khoảng bất tử (tính từ đầu đòn)
    float invulnTo   = 0.0f;
    bool  armor      = false;       // chịu được 1 đòn trong startup+active
    bool  projInvuln = false;       // xuyên qua đạn

    // Điều kiện dùng ----------------------------------------------------------
    bool groundOk = true;
    bool airOk    = false;

    // Đòn vật (height == Throw) ----------------------------------------------
    bool  techable  = true;         // bấm vật lại để phá (đòn vật thường)
    bool  slam      = false;        // nhấc bổng rồi nện xuống đất (vật lệnh)
    float throwHold = 0.32f;        // thời gian giữ đối thủ trước khi quăng

    // Âm thanh --------------------------------------------------------------
    SfxWeight sfx = SfxWeight::Light;
    bool voice = false;             // hét khi ra đòn

    const char *label = "";

    float Total() const { return startup + active + recovery; }
};

// Hitbox đang bật của một nhân vật ở frame hiện tại, đã đổi ra toạ độ thế giới.
struct ActiveAttack {
    bool      active = false;
    Rectangle world{};
    const MoveDef *def = nullptr;
};

} // namespace fighter
