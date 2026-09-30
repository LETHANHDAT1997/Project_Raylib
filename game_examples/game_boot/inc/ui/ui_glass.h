/**
 * ui_glass.h - Lõi render "Liquid Glass".
 *
 * Cách hoạt động mỗi khung hình:
 *   1. UiGlassBeginBackdrop() ... vẽ hình nền ... UiGlassEndBackdrop()
 *      Nền được vẽ vào một RenderTexture riêng rồi làm mờ nhiều lớp.
 *   2. Trong canvas chính: vẽ lại hình nền ở độ phân giải gốc.
 *   3. UiGlassPanel() vẽ từng tấm kính - shader lấy mẫu nền đã blur, bẻ cong
 *      ảnh ở mép (khúc xạ), thêm highlight đặc trưng của kính Apple.
 *
 * Bước 1 phải chạy TRƯỚC khi BeginTextureMode của canvas chính vì raylib
 * không cho lồng hai render target.
 */
#ifndef UI_GLASS_H
#define UI_GLASS_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    // Vật liệu kính được mô tả bằng ĐỘ SÁNG đích chứ không bằng một màu cụ
    // thể: sắc màu của tấm kính luôn đến từ hậu cảnh phía sau nó.
    float targetLum;     // Độ sáng mà mặt kính hướng tới (0..1)
    float level;         // Mức kéo hậu cảnh về targetLum (0 = giữ nguyên hẳn)
    float saturation;    // <1 nhạt màu, >1 đậm màu

    Color tint;          // Màu phủ THÊM - chỉ dùng cho bề mặt màu nhấn
    float tintStrength;  // 0 = không phủ màu nào

    float refraction;    // Độ bẻ cong ảnh nền ở mép (pixel)
    float edgeWidth;     // Bề dày dải mép nhận highlight (pixel)
    float highlight;     // Cường độ phản sáng viền
    float innerShadow;   // Bóng đổ phía trong mép dưới
    float alpha;         // Độ mờ của cả tấm kính
} UiGlassStyle;

// Vòng đời module -----------------------------------------------------------
void UiGlassInit(int width, int height);
void UiGlassShutdown(void);
bool UiGlassIsShaderReady(void);

// Chế độ Nhẹ: bỏ mọi shader và render target, vẽ bằng primitive thường.
// Dành cho GPU yếu (Raspberry Pi 0-3, trình dựng phần mềm).
void UiGlassSetLite(bool lite);
bool UiGlassIsLite(void);

// Độ mờ của lớp hậu cảnh mà kính lấy mẫu: 0 = nhìn xuyên gần như nguyên bản,
// 1 = mờ tối đa. Điều khiển cả số vòng lọc lẫn bán kính nên chỉnh liên tục
// mà không bị nhảy bậc.
void UiGlassSetBlurAmount(float amount);

// Mức "đục" chung: nhân vào `level` của các preset kính. Ở 0 thì tấm kính
// hiện gần như nguyên bản hậu cảnh, ở 1 thì kéo hẳn về độ sáng đích.
void UiGlassSetLevelScale(float scale);

// Quy đổi giá trị 0..1 của thanh trượt trong Cài đặt sang hệ số cho
// UiGlassSetLevelScale. Đặt ở đây để màn hình Cài đặt và bước khởi tạo
// dùng chung một công thức.
float GlassLevelScale(float slider01);

// Nền -----------------------------------------------------------------------
void      UiGlassBeginBackdrop(void);
void      UiGlassEndBackdrop(void);
Texture2D UiGlassBlurredTexture(void);

// Các preset kính -----------------------------------------------------------
UiGlassStyle UiGlassStyleWindow(void);   // Cửa sổ lớn ngoài cùng
UiGlassStyle UiGlassStylePanel(void);    // Panel nội dung
UiGlassStyle UiGlassStyleRaised(void);   // Thẻ/hàng nổi lên
UiGlassStyle UiGlassStyleSunken(void);   // Vùng lõm
UiGlassStyle UiGlassStyleAccent(Color accent); // Nút nhấn màu nhấn

// Vẽ ------------------------------------------------------------------------
void UiGlassPanel(Rectangle rec, float radius, UiGlassStyle style);

// Vẽ tấm kính nhưng lấy mẫu từ texture chỉ định (dùng cho tranh hero bo góc).
void UiGlassImagePanel(Rectangle rec, float radius, Texture2D tex, bool flipY, UiGlassStyle style);

// Viền sáng mảnh chạy quanh mép - tách riêng để có thể nhấn mạnh khi hover.
void UiGlassStroke(Rectangle rec, float radius, float thickness, Color color);

// Quầng sáng toả ra ngoài, vẽ bằng blend cộng.
void UiGlassGlow(Rectangle rec, float radius, float spread, Color color);

// Bóng đổ mềm phía dưới một tấm kính.
void UiGlassShadow(Rectangle rec, float radius, float spread, float alpha);

#endif // UI_GLASS_H
