#ifndef CARO_TYPES_H
#define CARO_TYPES_H

#include "raylib.h"
#include <stdbool.h>

// Canvas ảo 16:9 - bàn cờ bên trái, bảng thông tin bên phải.
#define CARO_CANVAS_W 1280
#define CARO_CANVAS_H 720

#define CARO_MAX_N     19            // Bàn lớn nhất hỗ trợ
#define CARO_MAX_CELLS (CARO_MAX_N * CARO_MAX_N)
#define CARO_WIN_LEN   5

#define CARO_MAX_PARTICLES 260

typedef enum {
    CARO_EMPTY = 0,
    CARO_X = 1,                      // Luôn đi trước
    CARO_O = 2
} CaroStone;

static inline CaroStone CaroOpponent(CaroStone s) { return (s == CARO_X) ? CARO_O : CARO_X; }

typedef enum {
    CARO_DIFF_EASY = 0,
    CARO_DIFF_NORMAL,
    CARO_DIFF_HARD,
    CARO_DIFF_COUNT
} CaroDifficulty;

typedef enum {
    CARO_MODE_VS_AI = 0,
    CARO_MODE_TWO_PLAYERS,
    CARO_MODE_COUNT
} CaroMode;

// Luật thắng: "Tự do" = đủ 5 là thắng; "Chặn hai đầu" (luật phổ biến ở
// Việt Nam) = hàng 5 bị quân đối phương chặn cả hai đầu thì không tính.
typedef enum {
    CARO_RULE_FREE = 0,
    CARO_RULE_BLOCKED,
    CARO_RULE_COUNT
} CaroRule;

// Trạng thái thuần luật chơi - AI sao chép nguyên struct này để tìm nước.
typedef struct {
    int n;                            // Kích thước cạnh bàn (15 hoặc 19)
    CaroRule rule;
    unsigned char cells[CARO_MAX_CELLS];
    short moves[CARO_MAX_CELLS];      // Lịch sử nước đi (chỉ số ô)
    int moveCount;
    CaroStone toMove;
} CaroBoard;

typedef enum {
    CARO_STATE_MENU = 0,
    CARO_STATE_PLAYING,
    CARO_STATE_PAUSED,
    CARO_STATE_OVER
} CaroState;

typedef enum {
    CARO_RESULT_NONE = 0,
    CARO_RESULT_X_WINS,
    CARO_RESULT_O_WINS,
    CARO_RESULT_DRAW
} CaroResult;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float size;
    float rot;
    float spin;
    float life;
    float maxLife;
    bool active;
} CaroParticle;

// Các dòng tuỳ chọn trong menu - thứ tự này là thứ tự hiển thị.
typedef enum {
    CARO_OPT_MODE = 0,
    CARO_OPT_DIFFICULTY,
    CARO_OPT_SIZE,
    CARO_OPT_RULE,
    CARO_OPT_FIRST,
    CARO_OPT_COUNT
} CaroMenuOption;

typedef struct {
    CaroState state;
    CaroBoard board;

    // Tuỳ chọn ván
    CaroMode mode;
    CaroDifficulty difficulty;
    int sizeChoice;                   // 0 = 15x15, 1 = 19x19
    CaroRule rule;
    bool humanFirst;                  // Người chơi cầm X và đi trước
    CaroStone humanStone;
    int menuRow;

    // Tiến trình ván
    CaroResult result;
    int winCells[2];                  // Hai đầu của hàng thắng (chỉ số ô)
    float winAnim;
    float resultTime;
    int cursor;                       // Ô đang trỏ bằng bàn phím
    bool keyboardCursor;              // Chỉ hiện con trỏ khi đang dùng phím
    int hoverCell;                    // Ô chuột đang trỏ (-1 nếu ngoài bàn)
    int hintCell;                     // Gợi ý của máy cho người chơi
    float hintTimer;
    float placeAnim[CARO_MAX_CELLS];  // 0 -> 1: hiệu ứng vẽ nét quân vừa đặt
    int pauseRow;

    // AI chạy trên luồng riêng để giao diện không bị đứng
    bool aiThinking;
    bool hintPending;
    float aiTimer;                    // Thời gian máy đã "suy nghĩ"

    // Thống kê (lưu đĩa)
    int wins[CARO_DIFF_COUNT];
    int losses[CARO_DIFF_COUNT];
    int draws[CARO_DIFF_COUNT];
    int sessionX, sessionO, sessionDraw;   // Tỉ số trong phiên chơi hiện tại

    CaroParticle particles[CARO_MAX_PARTICLES];
    float shake;
    float stateTime;
    float globalTime;
    float gameTime;
    bool soundEnabled;
    bool quitRequested;
} CaroGame;

#endif // CARO_TYPES_H
