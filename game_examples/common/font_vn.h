#ifndef FONT_VN_H
#define FONT_VN_H

#include "raylib.h"

// Khởi tạo và giải phóng font Tiếng Việt
void InitVietnameseFont(void);
Font GetVietnameseFont(void);
Font GetVietnameseFontBold(void);
void CloseVietnameseFont(void);

// Các hàm vẽ chữ và đo kích thước hỗ trợ đầy đủ Tiếng Việt có dấu và Unicode symbols
void DrawTextVN(const char *text, int posX, int posY, int fontSize, Color color);
void DrawTextVNBold(const char *text, int posX, int posY, int fontSize, Color color);
int MeasureTextVN(const char *text, int fontSize);
int MeasureTextVNBold(const char *text, int fontSize);

// Biến thể toạ độ/cỡ chữ kiểu float kèm tuỳ chỉnh khoảng cách ký tự.
// Tầng giao diện Liquid Glass dùng các hàm này để căn chỉnh chính xác hơn.
void    DrawTextVNPro(const char *text, Vector2 pos, float fontSize, float spacing, Color color);
void    DrawTextVNBoldPro(const char *text, Vector2 pos, float fontSize, float spacing, Color color);
Vector2 MeasureTextVNPro(const char *text, float fontSize, float spacing);
Vector2 MeasureTextVNBoldPro(const char *text, float fontSize, float spacing);

#endif // FONT_VN_H
