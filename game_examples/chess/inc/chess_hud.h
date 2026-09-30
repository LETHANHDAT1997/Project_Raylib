/**
 * chess_hud.h - Giao diện 2D vẽ đè lên cảnh 3D: bảng thông tin, menu, hộp
 * phong cấp, thẻ kết quả, hộp tạm dừng. Các hình chữ nhật bấm được nằm ở đây
 * để logic dùng chung với phần vẽ.
 */
#ifndef CHESS_HUD_H
#define CHESS_HUD_H

#include "chess_types.h"

void DrawChessHud(const ChessGame *game);

bool ChessPointInUi(const ChessGame *game, Vector2 p);   // Chuột đang ở trên bảng / hộp thoại

// Menu
#define CHESS_MENU_START_ROW CHESS_OPT_COUNT
int       ChessMenuOptionCount(ChessMenuOption row);
Rectangle ChessMenuChipRect(ChessMenuOption row, int index);
Rectangle ChessMenuStartRect(void);

// Nút trên bảng bên phải
typedef enum {
    CHESS_BTN_UNDO = 0,
    CHESS_BTN_HINT,
    CHESS_BTN_FLIP,
    CHESS_BTN_VIEW,
    CHESS_BTN_NEW,
    CHESS_BTN_PAUSE,
    CHESS_BTN_COUNT
} ChessPanelButton;
Rectangle ChessPanelButtonRect(ChessPanelButton b);

// Hộp phong cấp: 4 lựa chọn Hậu / Xe / Tượng / Mã
extern const int CHESS_PROMOTION_TYPES[4];
Rectangle ChessPromotionRect(int index);

// Thẻ kết quả: Ván mới / Xem bàn cờ / Về menu
#define CHESS_OVER_BUTTONS 3
Rectangle ChessOverButtonRect(int index);

#define CHESS_PAUSE_ROWS 4
Rectangle ChessPauseRowRect(int row);

#endif // CHESS_HUD_H
