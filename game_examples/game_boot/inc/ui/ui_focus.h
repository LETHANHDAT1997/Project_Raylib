/**
 * ui_focus.h - Vòng focus bàn phím cho giao diện immediate-mode.
 *
 * Cách dùng trong một khung hình:
 *   UiFocusSetNav(...)        // nạp ý định điều hướng, gọi ở bước Update
 *   UiFocusBeginFrame(scope)  // đầu bước Draw
 *     ... mỗi widget gọi UiFocusRegister() khi tự vẽ ...
 *   UiFocusEndFrame()         // cuối bước Draw: giải quyết điều hướng
 *
 * Widget không cần giữ trạng thái: mỗi khung hình chúng đăng ký lại hình
 * chữ nhật của mình, và module này nhớ xem mục thứ mấy đang được focus.
 * Điều hướng theo KHÔNG GIAN (dựa trên vị trí thật của các ô) nên thứ tự
 * vẽ không cần trùng với thứ tự người dùng cảm nhận.
 */
#ifndef UI_FOCUS_H
#define UI_FOCUS_H

#include "raylib.h"
#include <stdbool.h>

// Truyền vào các widget *Ex để chúng tự đăng ký vùng focus của riêng mình.
// Truyền một id thật (lấy từ UiFocusRegister) nếu muốn cả một hàng lớn hơn
// làm điểm dừng chung - khi đó bên gọi tự lo vẽ vòng focus.
#define UI_FOCUS_AUTO (-2)

typedef enum {
    UI_FOCUS_FLAG_NONE  = 0,
    // Widget tự dùng phím Trái/Phải để chỉnh giá trị (thanh trượt, nhóm
    // nút chọn), nên điều hướng ngang sẽ không nhảy sang widget khác.
    UI_FOCUS_CONSUMES_H = 1 << 0
} UiFocusFlags;

// Ý định điều hướng của khung hình này, do tầng trên nạp vào.
void UiFocusSetNav(bool up, bool down, bool left, bool right, bool activate);

// scope thay đổi (ví dụ đổi tab) thì focus được đặt lại về mục đầu tiên.
void UiFocusBeginFrame(int scope);
void UiFocusEndFrame(void);

// Bật/tắt toàn bộ hệ thống - trang nào tự xử lý phím thì tắt đi.
void UiFocusSetEnabled(bool enabled);
bool UiFocusEnabled(void);

// Đăng ký một vùng có thể focus. Trả về chỉ số của nó trong khung hình này.
int  UiFocusRegister(Rectangle rec, UiFocusFlags flags);

bool UiFocusIsFocused(int item);
// Mục này đang focus VÀ người dùng vừa nhấn Enter/Space.
bool UiFocusActivated(int item);
// -1 / 0 / +1 khi mục này đang focus và người dùng nhấn Trái/Phải.
int  UiFocusAxisH(int item);

// Chỉ hiện vòng focus khi người dùng thật sự đang dùng bàn phím.
bool UiFocusRingVisible(int item);
void UiFocusDrawRing(Rectangle rec, float radius, Color accent);

#endif // UI_FOCUS_H
