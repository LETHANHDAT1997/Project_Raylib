/**
 * boot_settings.h - Tuỳ chọn người dùng, lưu cùng chỗ với thống kê.
 */
#ifndef BOOT_SETTINGS_H
#define BOOT_SETTINGS_H

#include <stdbool.h>

#define BOOT_WALLPAPER_ID_MAX 64

// Chất lượng đồ hoạ: áp cho Hub và các game có chế độ nhẹ (Caro, Cờ Vua).
typedef enum {
    BOOT_GFX_AUTO = 0,    // Tự chọn theo phần cứng / FPS đo được (xem boot_perf.h)
    BOOT_GFX_QUALITY,     // Luôn bật hiệu ứng kính
    BOOT_GFX_LITE,        // Luôn vẽ bằng primitive thường, cho máy yếu
    BOOT_GFX_COUNT
} BootGraphicsMode;

typedef struct {
    // Hai thông số của lớp kính, chỉnh liên tục trong trang Cài đặt:
    float glassBlur;      // 0 = kính trong suốt thấy rõ nền, 1 = mờ tối đa
    float glassTint;      // 0 = kính gần như không màu, 1 = đục nhất
    int   graphicsMode;   // BootGraphicsMode
    bool  showFps;
    bool  reduceMotion;   // Giảm chuyển động cho người nhạy cảm với animation
    float masterVolume;   // 0.0 .. 1.0
    char  wallpaperId[BOOT_WALLPAPER_ID_MAX];  // Khoá hình nền, khớp tên file trong assets/wallpapers
} BootSettings;

void          BootSettingsLoad(void);
void          BootSettingsSave(void);
BootSettings *BootSettingsGet(void);
void          BootSettingsApply(void);  // Đẩy các giá trị xuống raylib

#endif // BOOT_SETTINGS_H
