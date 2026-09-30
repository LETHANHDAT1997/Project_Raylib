/**
 * boot_perf.h - Quyết định Hub chạy đồ hoạ "Đẹp" hay "Nhẹ".
 *
 * Hiệu ứng kính (shader SDF + blur nhiều lượt + vài render target mỗi khung
 * hình) nặng hơn mọi game trong thư viện cộng lại. GPU VideoCore IV của
 * Raspberry Pi 0-3 hay trình dựng phần mềm (llvmpipe) chỉ kéo được vài FPS,
 * nên ở đó Hub chuyển sang vẽ bằng primitive thường.
 *
 * Chế độ "Tự động" chọn Nhẹ khi:
 *   - máy là Raspberry Pi đời 0-3 (xem common/perf_hint.h), hoặc
 *   - vài giây đầu ở Hub đo được FPS quá thấp.
 *
 * Kết quả được chuyển tiếp sang PerfHintSetLite() để Caro và Cờ Vua cũng
 * vẽ theo cùng chế độ.
 */
#ifndef BOOT_PERF_H
#define BOOT_PERF_H

#include <stdbool.h>

// Gọi TRƯỚC InitWindow: chỉ đọc thông tin phần cứng, không cần context GL.
void BootPerfInit(void);

// Phần cứng được biết trước là yếu (dùng để bỏ MSAA khi mở cửa sổ).
bool BootPerfWeakDevice(void);

// Nạp thời lượng khung hình thật (chưa kẹp) khi Hub đang vẽ ở chế độ Đẹp.
void BootPerfSample(float frameTime);

// Đo lại từ đầu, gọi khi người dùng đổi chế độ đồ hoạ.
void BootPerfResetMeasure(void);

// Kết quả cuối cùng sau khi xét cả cài đặt người dùng.
bool BootPerfLite(void);

// Lý do chế độ Tự động chọn Nhẹ, "" nếu không chọn.
const char *BootPerfReason(void);

#endif // BOOT_PERF_H
