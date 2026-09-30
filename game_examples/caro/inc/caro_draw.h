/**
 * caro_draw.h - Vẽ Cờ Caro và bố cục các vùng bấm được.
 *
 * Mọi hình chữ nhật dùng cho cả vẽ lẫn nhận chuột nằm ở đây, để logic
 * trong caro_game.c không phải lặp lại con số toạ độ nào.
 */
#ifndef CARO_DRAW_H
#define CARO_DRAW_H

#include "caro_types.h"

void DrawCaroGame(const CaroGame *game);

// Chế độ đồ hoạ nhẹ (PerfHintLite): dựng sẵn lớp nền tĩnh. Gọi NGOÀI
// BeginTextureMode (vd. cuối bước Update) vì bên trong có vẽ vào texture riêng.
void CaroDrawPrepare(const CaroGame *game);
void CaroDrawRelease(void);

// Bàn cờ
Rectangle CaroBoardArea(int n);                        // Vùng lưới ô cờ
Rectangle CaroCellRect(int n, int cell);
int       CaroCellAtPoint(int n, Vector2 p);           // -1 nếu ngoài bàn

// Menu
#define CARO_MENU_START_ROW CARO_OPT_COUNT            // Dòng "Bắt đầu" nằm sau các tuỳ chọn
int       CaroMenuOptionCount(CaroMenuOption row);
Rectangle CaroMenuChipRect(CaroMenuOption row, int index);
Rectangle CaroMenuStartRect(void);

// Bảng bên phải khi đang chơi
typedef enum {
    CARO_BTN_UNDO = 0,
    CARO_BTN_HINT,
    CARO_BTN_NEW,
    CARO_BTN_MENU,
    CARO_BTN_COUNT
} CaroPanelButton;
Rectangle CaroPanelButtonRect(CaroPanelButton b);

// Hộp tạm dừng
#define CARO_PAUSE_ROWS 4
Rectangle CaroPauseRowRect(int row);

#endif // CARO_DRAW_H
