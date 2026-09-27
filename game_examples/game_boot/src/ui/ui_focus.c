#include "ui_focus.h"
#include "ui_glass.h"
#include "ui_theme.h"

#include <math.h>
#include <string.h>

#define FOCUS_MAX_ITEMS 64

typedef struct {
    Rectangle rec;
    UiFocusFlags flags;
} FocusItem;

static FocusItem s_items[FOCUS_MAX_ITEMS];
static int  s_count = 0;
static int  s_focused = 0;
static int  s_scope = -1;
static bool s_enabled = true;
static bool s_ringVisible = false;

// Ý định điều hướng, sống trong đúng một khung hình.
static bool s_navUp, s_navDown, s_navLeft, s_navRight, s_activate;

void UiFocusSetNav(bool up, bool down, bool left, bool right, bool activate)
{
    s_navUp = up;
    s_navDown = down;
    s_navLeft = left;
    s_navRight = right;
    s_activate = activate;

    // Bất kỳ thao tác bàn phím nào cũng làm vòng focus hiện ra.
    if (up || down || left || right || activate) s_ringVisible = true;
}

void UiFocusSetEnabled(bool enabled)
{
    s_enabled = enabled;
}

bool UiFocusEnabled(void)
{
    return s_enabled;
}

void UiFocusBeginFrame(int scope)
{
    s_count = 0;

    // Đổi trang thì bắt đầu lại từ mục đầu tiên.
    if (scope != s_scope) {
        s_scope = scope;
        s_focused = 0;
    }
}

int UiFocusRegister(Rectangle rec, UiFocusFlags flags)
{
    if (!s_enabled || s_count >= FOCUS_MAX_ITEMS) return -1;

    int id = s_count++;
    s_items[id].rec = rec;
    s_items[id].flags = flags;

    // Bấm chuột vào đâu thì focus nhảy về đó, để sau đó dùng tiếp bàn phím
    // là đi tiếp từ chính chỗ vừa bấm.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(GetMousePosition(), rec)) {
        s_focused = id;
        s_ringVisible = false;
    }
    return id;
}

bool UiFocusIsFocused(int item)
{
    return s_enabled && item >= 0 && item == s_focused;
}

bool UiFocusActivated(int item)
{
    return s_activate && UiFocusIsFocused(item);
}

int UiFocusAxisH(int item)
{
    if (!UiFocusIsFocused(item)) return 0;
    if (s_navLeft)  return -1;
    if (s_navRight) return  1;
    return 0;
}

bool UiFocusRingVisible(int item)
{
    return s_ringVisible && UiFocusIsFocused(item);
}

static Vector2 CenterOf(Rectangle r)
{
    return (Vector2){r.x + r.width * 0.5f, r.y + r.height * 0.5f};
}

// Tìm mục gần nhất theo hướng (dx, dy). Khoảng cách dọc theo hướng đi được
// tính đủ, còn lệch ngang bị phạt nhẹ - nhờ vậy khi đi xuống thì ưu tiên ô
// thẳng bên dưới thay vì ô ở xa sang một bên.
static int FindNeighbour(int from, float dirX, float dirY)
{
    if (from < 0 || from >= s_count) return -1;

    Vector2 origin = CenterOf(s_items[from].rec);
    int best = -1;
    float bestScore = 0.0f;

    for (int i = 0; i < s_count; i++) {
        if (i == from) continue;

        Vector2 c = CenterOf(s_items[i].rec);
        float dx = c.x - origin.x;
        float dy = c.y - origin.y;

        float along = dx * dirX + dy * dirY;          // Đi đúng hướng bao xa
        if (along <= 1.0f) continue;                   // Sai hướng thì bỏ qua

        float across = fabsf(dx * dirY - dy * dirX);   // Lệch sang bên bao nhiêu
        float score = along + across * 2.0f;

        if (best < 0 || score < bestScore) {
            best = i;
            bestScore = score;
        }
    }
    return best;
}

void UiFocusEndFrame(void)
{
    if (s_enabled && s_count > 0) {
        if (s_focused >= s_count) s_focused = s_count - 1;
        if (s_focused < 0) s_focused = 0;

        bool consumesH = (s_items[s_focused].flags & UI_FOCUS_CONSUMES_H) != 0;

        int next = -1;
        int step = 0;   // Hướng đi theo thứ tự đăng ký, dùng khi tìm theo
                        // không gian không ra kết quả

        if (s_navDown) {
            next = FindNeighbour(s_focused, 0.0f, 1.0f);
            step = 1;
        } else if (s_navUp) {
            next = FindNeighbour(s_focused, 0.0f, -1.0f);
            step = -1;
        } else if (s_navRight && !consumesH) {
            next = FindNeighbour(s_focused, 1.0f, 0.0f);
            step = 1;
        } else if (s_navLeft && !consumesH) {
            next = FindNeighbour(s_focused, -1.0f, 0.0f);
            step = -1;
        }

        // Không có mục nào nằm đúng hướng (ví dụ đang ở mục cuối cột trái,
        // còn nút cần tới lại nằm tít bên cột phải) thì đi tiếp theo thứ tự
        // đăng ký. Nhờ vậy mọi mục đều chắc chắn tới được bằng bàn phím.
        if (next < 0 && step != 0) {
            next = (s_focused + step + s_count) % s_count;
        }

        if (next >= 0) s_focused = next;
    }

    // Ý định chỉ có hiệu lực trong một khung hình.
    s_navUp = s_navDown = s_navLeft = s_navRight = s_activate = false;
}

void UiFocusDrawRing(Rectangle rec, float radius, Color accent)
{
    Rectangle ring = {rec.x - 3.0f, rec.y - 3.0f, rec.width + 6.0f, rec.height + 6.0f};
    UiGlassGlow(ring, radius + 3.0f, 18.0f, UiAlpha(accent, 0.45f));
    UiGlassStroke(ring, radius + 3.0f, 2.0f, UiAlpha(WHITE, 0.92f));
}
