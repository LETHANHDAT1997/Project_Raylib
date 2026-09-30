/**
 * caro_rules.h - Luật cờ caro: đặt quân, hoàn tác, kiểm tra thắng.
 *
 * Không phụ thuộc đồ hoạ, nên AI dùng lại trực tiếp trên bản sao bàn cờ.
 */
#ifndef CARO_RULES_H
#define CARO_RULES_H

#include "caro_types.h"

// Bốn hướng quét hàng: ngang, dọc, chéo xuôi, chéo ngược.
extern const int CARO_DIR_X[4];
extern const int CARO_DIR_Y[4];

void CaroBoardInit(CaroBoard *b, int n, CaroRule rule);

static inline int  CaroIndex(const CaroBoard *b, int x, int y) { return y * b->n + x; }
static inline bool CaroInside(const CaroBoard *b, int x, int y) { return x >= 0 && y >= 0 && x < b->n && y < b->n; }

bool CaroPlace(CaroBoard *b, int cell);     // Đặt quân của bên đang đi; false nếu ô đã có quân
void CaroUndo(CaroBoard *b);                // Gỡ nước cuối cùng
bool CaroBoardFull(const CaroBoard *b);

// Quân vừa đặt ở `cell` có tạo thành hàng thắng không. Nếu có, trả về hai
// ô ở hai đầu hàng (để vẽ vạch thắng).
bool CaroIsWinningMove(const CaroBoard *b, int cell, int *endA, int *endB);

// Toạ độ ô kiểu "H8" để hiển thị (cột chữ cái bỏ chữ I như bàn cờ vây).
const char *CaroCellName(const CaroBoard *b, int cell);

#endif // CARO_RULES_H
