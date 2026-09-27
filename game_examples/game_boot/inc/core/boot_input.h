/**
 * boot_input.h - Tầng xử lý input tập trung của Arcade Hub.
 *
 * Mọi nơi trong ứng dụng hỏi "người dùng muốn làm gì" (hành động) thay vì
 * "phím nào đang được nhấn". Nhờ vậy:
 *   - Muốn đổi phím tắt chỉ cần sửa bảng ánh xạ trong boot_input.c.
 *   - Một hành động gắn được nhiều phím (mũi tên và WASD) mà nơi dùng
 *     không phải liệt kê lại từng phím.
 *   - Cơ chế giữ phím để lặp nằm một chỗ, không lặp lại ở từng màn hình.
 *
 * Gọi BootInputUpdate() đúng một lần ở đầu mỗi khung hình, trước mọi thứ khác.
 */
#ifndef BOOT_INPUT_H
#define BOOT_INPUT_H

#include <stdbool.h>

typedef enum {
    // Điều hướng trong giao diện
    BOOT_ACTION_NAV_UP = 0,
    BOOT_ACTION_NAV_DOWN,
    BOOT_ACTION_NAV_LEFT,
    BOOT_ACTION_NAV_RIGHT,
    BOOT_ACTION_ACTIVATE,     // Enter / Space: kích hoạt mục đang focus
    BOOT_ACTION_BACK,         // Esc: lùi một cấp

    // Chuyển trang
    BOOT_ACTION_NEXT_TAB,
    BOOT_ACTION_PREV_TAB,

    // Toàn cục
    BOOT_ACTION_HOME,         // F1 / Home: thoát game về Hub
    BOOT_ACTION_FULLSCREEN,   // F11
    BOOT_ACTION_SCREENSHOT,   // F12

    BOOT_ACTION_COUNT
} BootAction;

void BootInputUpdate(float dt);

// Vừa nhấn trong khung hình này. Các hành động điều hướng còn tự lặp lại
// khi giữ phím, nên giữ mũi tên là chạy liên tục trong danh sách.
bool BootInputPressed(BootAction action);
bool BootInputDown(BootAction action);

// Phím số 1..9 để chọn nhanh; trả về 0 nếu không có phím nào.
int  BootInputDigitPressed(void);

// Người dùng vừa động tới chuột. Dùng để ẩn vòng focus bàn phím, giống
// hành vi :focus-visible trên web.
bool BootInputMouseActive(void);

// Có bất kỳ thao tác bàn phím điều hướng nào trong khung hình này không.
bool BootInputKeyboardActive(void);

#endif // BOOT_INPUT_H
