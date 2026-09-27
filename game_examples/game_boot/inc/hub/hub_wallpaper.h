/**
 * hub_wallpaper.h - Quản lý hình nền của Arcade Hub.
 *
 * Thư mục assets/wallpapers được quét lúc khởi động: mọi ảnh bỏ vào đó đều
 * tự xuất hiện trong trang Cài đặt, không cần sửa mã. Định dạng hỗ trợ:
 * .png .jpg .jpeg .bmp .qoi .tga. File không giải mã được sẽ bị bỏ qua và
 * ghi cảnh báo, kèm số lượng hiển thị ngay trong lưới chọn.
 * Mục đầu tiên luôn là hình nền gradient động dựng bằng mã (không cần file).
 *
 * Bộ nhớ: mỗi hình chỉ giữ một thumbnail nhỏ để vẽ ô chọn; ảnh đầy đủ chỉ
 * nạp cho hình đang dùng và được giải phóng khi đổi sang hình khác.
 */
#ifndef HUB_WALLPAPER_H
#define HUB_WALLPAPER_H

#include "raylib.h"
#include <stdbool.h>

#define WALLPAPER_GRADIENT_ID "gradient"

void HubWallpaperInit(const char *selectedId);
void HubWallpaperShutdown(void);

int         HubWallpaperCount(void);
int         HubWallpaperSkippedCount(void);  // Số file trong thư mục không giải mã được
const char *HubWallpaperId(int index);
const char *HubWallpaperLabel(int index);
Texture2D   HubWallpaperThumbnail(int index);  // texture.id == 0 nghĩa là mục gradient
int         HubWallpaperIndexOfId(const char *id);

int  HubWallpaperCurrent(void);
void HubWallpaperSelect(int index);

// Vẽ hình nền đang chọn. Trả về false nếu đang dùng gradient động,
// khi đó bên gọi tự vẽ nền dựng bằng mã.
bool HubWallpaperDraw(int width, int height, float time, Color accent, bool reduceMotion);

#endif // HUB_WALLPAPER_H
