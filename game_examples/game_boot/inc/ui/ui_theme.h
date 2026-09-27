/**
 * ui_theme.h - Bảng màu, khoảng cách và cỡ chữ của theme Liquid Glass.
 *
 * Mọi hằng số thị giác tập trung tại đây. Đổi theme = sửa duy nhất file này.
 */
#ifndef UI_THEME_H
#define UI_THEME_H

#include "raylib.h"

// ---------------------------------------------------------------- Kích thước
#define UI_PAD_XS   6.0f
#define UI_PAD_SM   12.0f
#define UI_PAD_MD   18.0f
#define UI_PAD_LG   24.0f
#define UI_PAD_XL   34.0f

#define UI_RADIUS_SM   10.0f
#define UI_RADIUS_MD   16.0f
#define UI_RADIUS_LG   22.0f
#define UI_RADIUS_XL   30.0f
#define UI_RADIUS_PILL 999.0f

// ------------------------------------------------------------------- Cỡ chữ
#define UI_FS_DISPLAY 44.0f
#define UI_FS_H1      30.0f
#define UI_FS_H2      21.0f
#define UI_FS_H3      16.0f
#define UI_FS_BODY    14.0f
#define UI_FS_SMALL   12.5f
#define UI_FS_TINY    11.0f

// Khoảng cách giữa các ký tự (đơn vị pixel) cho từng cấp chữ.
#define UI_TRACK_TIGHT  0.0f
#define UI_TRACK_NORMAL 0.6f
#define UI_TRACK_WIDE   2.2f

// Lưu ý: bảng màu này KHÔNG chứa màu nền cho kính. Vật liệu kính lấy màu
// từ hậu cảnh và chỉ chuẩn hoá độ sáng (xem ui_glass.h), nên mọi màu cố
// định ở đây đều là chữ, nét viền hoặc màu chức năng.
typedef struct {
    // Chữ
    Color textPrimary;
    Color textSecondary;
    Color textMuted;
    Color textOnAccent;

    // Nét viền
    Color strokeSoft;
    Color strokeStrong;
    Color strokeHighlight;

    // Màu chức năng
    Color accent;
    Color success;
    Color warning;
    Color danger;
} UiTheme;

extern const UiTheme UI;

// Trộn hai màu theo hệ số t (0..1).
Color UiMix(Color a, Color b, float t);
// Nhân alpha của một màu.
Color UiAlpha(Color c, float alpha);
// Làm sáng (amount > 0) hoặc tối (amount < 0) một màu.
Color UiShade(Color c, float amount);

#endif // UI_THEME_H
