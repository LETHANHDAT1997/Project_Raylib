#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H

#include "raylib.h"
#include "chess_board.h"
#include "chess_ai.h"
#include <stdbool.h>

// Canvas ảo 16:9. Cảnh 3D phủ kín canvas, bảng thông tin nằm đè bên phải.
#define CHESS_CANVAS_W 1600
#define CHESS_CANVAS_H 900

#define CHESS_MAX_VISUALS 32          // Mỗi quân trên bàn là một "hình" có vị trí 3D riêng
#define CHESS_MAX_SAN     (CHESS_MAX_HISTORY)

typedef enum {
    CHESS_STATE_MENU = 0,
    CHESS_STATE_PLAYING,
    CHESS_STATE_PROMOTION,            // Đang chọn quân phong cấp
    CHESS_STATE_PAUSED,
    CHESS_STATE_OVER
} ChessState;

typedef enum {
    CHESS_MODE_VS_AI = 0,
    CHESS_MODE_TWO_PLAYERS,
    CHESS_MODE_COUNT
} ChessMode;

typedef enum {
    CHESS_SIDE_WHITE = 0,
    CHESS_SIDE_BLACK,
    CHESS_SIDE_RANDOM,
    CHESS_SIDE_COUNT
} ChessSideChoice;

typedef enum {
    CHESS_OPT_MODE = 0,
    CHESS_OPT_DIFFICULTY,
    CHESS_OPT_SIDE,
    CHESS_OPT_QUALITY,
    CHESS_OPT_COUNT
} ChessMenuOption;

typedef enum {
    CHESS_RESULT_NONE = 0,
    CHESS_RESULT_WHITE_WINS,
    CHESS_RESULT_BLACK_WINS,
    CHESS_RESULT_DRAW
} ChessResult;

// Hình 3D của một quân: vị trí hiện tại + hoạt ảnh di chuyển / bị bắt.
typedef struct {
    int type;                 // ChessPieceType
    int color;
    int square;               // Ô logic (-1 khi đã bị bắt)
    Vector3 pos;
    Vector3 from, to;
    float t;                  // 0 -> 1 tiến độ di chuyển
    float duration;
    float delay;              // Chờ trước khi bắt đầu (quân bị bắt đợi quân kia tới)
    float arc;                // Độ cao cung bay
    int promoteTo;            // Đổi hình khi hạ cánh (phong cấp)
    bool captured;
    bool moving;
    bool alive;
} ChessVisual;

typedef struct {
    float yaw, pitch, distance;           // Góc hiện tại (mượt dần về đích)
    float targetYaw, targetPitch, targetDistance;
    bool dragging;
    bool topDown;                         // Góc nhìn từ trên xuống
} ChessCameraRig;

typedef struct {
    ChessState state;
    ChessBoard board;

    // Tuỳ chọn
    ChessMode mode;
    ChessAILevel difficulty;              // EASY..HARD
    ChessSideChoice sideChoice;
    bool highQuality;                     // Khử răng cưa siêu lấy mẫu + bóng đổ nét
    int humanColor;                       // Màu quân người chơi khi đấu máy
    int menuRow;

    // Tương tác
    int selected;                         // Ô đang chọn (-1)
    int hoverSquare;
    int cursor;                           // Con trỏ bàn phím
    bool keyboardCursor;
    ChessMove legal[CHESS_MAX_MOVES];     // Nước hợp lệ của thế cờ hiện tại
    int legalCount;
    ChessMove pendingPromotion;           // Nước phong cấp chờ chọn quân
    int promotionChoice;
    int pauseRow;

    // Lịch sử hiển thị
    char san[CHESS_MAX_SAN][12];
    int sanCount;
    int moveListScroll;
    ChessMove lastMove;
    ChessMove hintMove;
    float hintTimer;

    // Kết quả
    ChessResult result;
    ChessStatus endReason;
    bool resigned;
    float resultTime;
    bool overCardHidden;                  // Đã đóng thẻ kết quả để xem lại bàn cờ

    // AI
    bool aiThinking;
    bool hintPending;
    float aiTimer;

    // Thống kê (lưu đĩa)
    int wins[3], losses[3], draws[3];

    // Cảnh 3D
    ChessVisual visuals[CHESS_MAX_VISUALS];
    int capturedCount[2];                 // Số quân mỗi màu đã bị bắt (xếp ra ngoài bàn)
    ChessCameraRig cam;
    float checkFlash;

    float stateTime;
    float globalTime;
    bool soundEnabled;
    bool quitRequested;
} ChessGame;

#endif // CHESS_TYPES_H
