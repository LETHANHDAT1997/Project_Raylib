/**
 * ui_widgets.h - Các thành phần giao diện dựng trên nền ui_glass.
 *
 * Phong cách immediate-mode: hàm vừa vẽ vừa trả về trạng thái tương tác,
 * nên màn hình gọi chúng trực tiếp trong hàm Draw mà không cần giữ state.
 */
#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "raylib.h"
#include "ui_glass.h"
#include <stdbool.h>

typedef struct {
    bool hovered;
    bool pressed;   // Đang giữ chuột trái
    bool clicked;   // Vừa nhả chuột trong vùng
} UiInteraction;

typedef enum {
    UI_ALIGN_LEFT = 0,
    UI_ALIGN_CENTER,
    UI_ALIGN_RIGHT
} UiAlign;

// Con trỏ chuột trong hệ toạ độ canvas ảo.
Vector2       UiMouse(void);
UiInteraction UiHitTest(Rectangle rec);

// Chữ ----------------------------------------------------------------------
void  UiText(const char *text, Vector2 pos, float size, Color color);
void  UiTextBold(const char *text, Vector2 pos, float size, Color color);
void  UiTextTracked(const char *text, Vector2 pos, float size, float tracking, Color color, bool bold);
float UiTextWidth(const char *text, float size, bool bold);
// Vẽ chữ căn theo một khung; trả về chiều cao đã dùng.
float UiTextBox(const char *text, Rectangle box, float size, float lineGap, Color color, UiAlign align, bool bold);
// Cắt chữ quá dài và thêm dấu "…".
const char *UiTextEllipsis(const char *text, float size, bool bold, float maxWidth);

// Thành phần ---------------------------------------------------------------
UiInteraction UiGlassButton(Rectangle rec, const char *label, Color accent, bool primary, bool enabled);
UiInteraction UiIconButton(Rectangle rec, void (*drawIcon)(Rectangle area, Color tint), Color accent, bool active);
void          UiChip(Rectangle rec, const char *label, Color accent, bool solid);
float         UiChipRow(Vector2 origin, const char *const *labels, int count, float height, float gap, Color accent);
void          UiProgressBar(Rectangle rec, float value01, Color accent);
void          UiStatLine(Rectangle rec, void (*drawIcon)(Rectangle area, Color tint), const char *label, const char *value);
void          UiSeparator(Vector2 start, float length, bool vertical);
UiInteraction UiToggle(Rectangle rec, bool value, Color accent);
UiInteraction UiSlider(Rectangle rec, float *value01, Color accent);
UiInteraction UiSegmented(Rectangle rec, const char *const *labels, int count, int *index, Color accent);

// Biến thể nhận sẵn id focus: dùng khi muốn cả hàng chứa widget làm một
// điểm dừng bàn phím thay vì chỉ riêng ô điều khiển nhỏ bên trong.
UiInteraction UiToggleEx(Rectangle rec, bool value, Color accent, int focusId);
UiInteraction UiSliderEx(Rectangle rec, float *value01, Color accent, int focusId);

#endif // UI_WIDGETS_H
