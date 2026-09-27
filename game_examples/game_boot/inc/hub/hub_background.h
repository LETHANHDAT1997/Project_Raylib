/**
 * hub_background.h - Hình nền "mesh gradient" trôi chậm phía sau lớp kính.
 *
 * Nền ngả dần sang màu nhấn của game đang chọn, tạo cảm giác cả giao diện
 * phản ứng theo lựa chọn của người dùng.
 */
#ifndef HUB_BACKGROUND_H
#define HUB_BACKGROUND_H

#include "raylib.h"

void HubBackgroundUpdate(Color accent, float dt, bool reduceMotion);
void HubBackgroundDraw(int width, int height, float time);

#endif // HUB_BACKGROUND_H
