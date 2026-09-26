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

#endif // FONT_VN_H
