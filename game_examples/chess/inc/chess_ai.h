/**
 * chess_ai.h - Máy chơi cờ vua, tìm nước trên một luồng riêng.
 *
 *   Dễ     : nhìn 1 nước + tìm kiếm tĩnh, cộng nhiễu lớn và thỉnh thoảng
 *            đi nước ngẫu nhiên - cỡ người mới học.
 *   Thường : alpha-beta độ sâu 3 cộng tìm kiếm tĩnh, nhiễu nhỏ ở gốc.
 *   Khó    : đào sâu dần với PVS, bảng chuyển vị, nước sát thủ, cắt tỉa
 *            nước rỗng và giảm độ sâu nước muộn trong ~2.5 giây.
 */
#ifndef CHESS_AI_H
#define CHESS_AI_H

#include "chess_board.h"

typedef enum {
    CHESS_AI_EASY = 0,
    CHESS_AI_NORMAL,
    CHESS_AI_HARD,
    CHESS_AI_HINT,          // Dùng cho nút gợi ý: mạnh như Khó nhưng nhanh hơn
    CHESS_AI_LEVELS
} ChessAILevel;

void ChessAIRequest(const ChessBoard *board, ChessAILevel level);
bool ChessAIPoll(ChessMove *outMove);
bool ChessAIBusy(void);
void ChessAICancel(void);

#endif // CHESS_AI_H
