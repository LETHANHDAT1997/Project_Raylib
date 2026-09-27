#pragma once
#include "raylib.h"

namespace fighter {

// Bọc font tiếng Việt (viết bằng C ở game_examples/common) cho phía C++.
void  TextInit();
void  TextShutdown();
void  DrawText(const char *s, Vector2 pos, float size, Color color);
void  DrawTextBold(const char *s, Vector2 pos, float size, Color color);
void  DrawTextCentered(const char *s, float cx, float y, float size, Color color);
void  DrawTextBoldCentered(const char *s, float cx, float y, float size, Color color);
float TextWidth(const char *s, float size);
float TextWidthBold(const char *s, float size);

// Vẽ đoạn văn tự ngắt dòng theo chiều rộng cho trước; trả về chiều cao đã dùng.
// Cần thiết vì phần giới thiệu nhân vật dài ngắn khác nhau.
float DrawTextWrapped(const char *s, Rectangle box, float size, float lineGap,
                      Color color, bool centered);

} // namespace fighter
