/**
 * boot_viewport.h - Ánh xạ "hệ toạ độ thiết kế" sang cửa sổ thật.
 *
 * Khác với BootCanvas (vẽ vào render texture rồi phóng to - hợp với game
 * pixel art có độ phân giải cố định), viewport chỉ áp một phép biến đổi
 * ma trận rồi vẽ thẳng ở độ phân giải gốc của cửa sổ. Chữ và đường nét của
 * launcher nhờ đó được rasterise đúng độ phân giải màn hình thay vì bị
 * phóng to từ ảnh 1280x720, nên sắc nét hơn hẳn.
 *
 * Mã vẽ phía trên vẫn làm việc với hệ 1280x720 như cũ, không cần biết
 * cửa sổ đang lớn bao nhiêu.
 */
#ifndef BOOT_VIEWPORT_H
#define BOOT_VIEWPORT_H

#include "raylib.h"

typedef struct {
    int designWidth;
    int designHeight;
    float scale;
    Vector2 offset;   // Lề letterbox/pillarbox tính bằng pixel màn hình
} BootViewport;

void  BootViewportUpdate(BootViewport *vp, int designWidth, int designHeight);
void  BootViewportBegin(BootViewport *vp);
void  BootViewportEnd(BootViewport *vp);

// Tỉ lệ phóng đang dùng, để nơi khác chọn độ phân giải texture cho khớp.
float BootViewportScale(void);

#endif // BOOT_VIEWPORT_H
