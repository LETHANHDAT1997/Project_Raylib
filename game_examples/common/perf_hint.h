/**
 * perf_hint.h - Gợi ý "đồ hoạ nhẹ" dùng chung cho Hub và các game.
 *
 * GPU VideoCore IV của Raspberry Pi 0-3 chỉ kham được các cảnh vẽ ít lớp:
 * mỗi lớp phủ bán trong suốt toàn màn hình (bóng mềm nhiều tầng, quầng
 * sáng...) hay shader nặng đều kéo FPS xuống rõ rệt. Game đọc PerfHintLite()
 * để chọn đường vẽ rẻ hơn.
 *
 * Mặc định: Nhẹ khi /proc/device-tree/model là Raspberry Pi đời 0-3.
 * Biến môi trường RAYLIB_ARCADE_LITE=1 / =0 ép bật / tắt (tiện thử trên PC).
 * Arcade Hub ghi đè giá trị này bằng lựa chọn "Chất lượng đồ hoạ" trong Cài đặt.
 */
#ifndef PERF_HINT_H
#define PERF_HINT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Phần cứng được biết trước là yếu (đã xét biến môi trường). Không cần cửa sổ.
bool PerfHintWeakDevice(void);
// Mô tả máy đọc được, "" nếu không rõ.
const char *PerfHintDeviceModel(void);

// Có nên vẽ ở chế độ nhẹ không. Mặc định = PerfHintWeakDevice().
bool PerfHintLite(void);
void PerfHintSetLite(bool lite);

#ifdef __cplusplus
}
#endif

#endif // PERF_HINT_H
