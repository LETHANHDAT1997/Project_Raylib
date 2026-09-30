/**
 * caro_ai.h - Máy chơi caro, tìm nước trên một luồng riêng.
 *
 * Ba mức độ:
 *   Dễ     : tham lam theo điểm "bộ năm ô", cố tình chọn lệch và đôi khi
 *            bỏ sót nước chặn - người mới vẫn thắng được.
 *   Thường : tham lam đầy đủ công + thủ, luôn thắng khi có thể và luôn chặn.
 *   Khó    : tìm chuỗi tứ liên tiếp buộc thắng (VCF) rồi alpha-beta đào sâu
 *            dần trong giới hạn thời gian.
 *
 * Giao diện dạng "gửi yêu cầu - hỏi kết quả" để vòng lặp game không bị chặn.
 */
#ifndef CARO_AI_H
#define CARO_AI_H

#include "caro_types.h"

// Bắt đầu tìm nước cho bên đang tới lượt trên bản sao của `board`.
// Bỏ qua nếu đang có một lượt tìm khác chạy.
void CaroAIRequest(const CaroBoard *board, CaroDifficulty difficulty);

// Trả về true (một lần) khi đã có kết quả, ghi ô được chọn vào outCell.
bool CaroAIPoll(int *outCell);

bool CaroAIBusy(void);

// Huỷ lượt tìm đang chạy (nếu có) và chờ luồng kết thúc.
void CaroAICancel(void);

#endif // CARO_AI_H
