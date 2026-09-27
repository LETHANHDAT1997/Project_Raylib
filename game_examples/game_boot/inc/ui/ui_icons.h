/**
 * ui_icons.h - Bộ icon vector vẽ bằng primitive của raylib.
 *
 * Dùng hình vẽ thay vì glyph emoji để icon luôn sắc nét ở mọi tỉ lệ scale
 * và không phụ thuộc vào font hệ thống có hỗ trợ emoji hay không.
 * Mọi hàm đều nhận khung vuông `area` và tự căn giữa nội dung bên trong.
 */
#ifndef UI_ICONS_H
#define UI_ICONS_H

#include "raylib.h"

typedef void (*UiIconFn)(Rectangle area, Color tint);

void UiIconGamepad(Rectangle area, Color tint);
void UiIconPlay(Rectangle area, Color tint);
void UiIconEllipsis(Rectangle area, Color tint);
void UiIconChevronRight(Rectangle area, Color tint);
void UiIconClock(Rectangle area, Color tint);
void UiIconCalendar(Rectangle area, Color tint);
void UiIconPerson(Rectangle area, Color tint);
void UiIconDisplay(Rectangle area, Color tint);
void UiIconSparkle(Rectangle area, Color tint);
void UiIconKeyboard(Rectangle area, Color tint);
void UiIconSliders(Rectangle area, Color tint);
void UiIconLibrary(Rectangle area, Color tint);
void UiIconSpeaker(Rectangle area, Color tint);
void UiIconExpand(Rectangle area, Color tint);
void UiIconTrophy(Rectangle area, Color tint);
void UiIconCounter(Rectangle area, Color tint);

#endif // UI_ICONS_H
