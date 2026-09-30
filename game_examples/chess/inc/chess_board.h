/**
 * chess_board.h - Luật cờ vua đầy đủ, không phụ thuộc đồ hoạ.
 *
 * Bàn cờ dạng mảng 64 ô (a1 = 0, h1 = 7, a8 = 56). Hỗ trợ nhập thành, bắt
 * tốt qua đường, phong cấp, chiếu hết, hết nước đi, hoà do 50 nước, lặp lại
 * 3 lần và không đủ quân chiếu hết. Có make/unmake nên AI dùng lại trực tiếp.
 */
#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHESS_NONE = 0,
    CHESS_PAWN,
    CHESS_KNIGHT,
    CHESS_BISHOP,
    CHESS_ROOK,
    CHESS_QUEEN,
    CHESS_KING,
    CHESS_PIECE_TYPES
} ChessPieceType;

typedef enum {
    CHESS_WHITE = 0,
    CHESS_BLACK = 1
} ChessColor;

// Mã quân trong ô: 0 = trống, trắng 1..6, đen 9..14.
#define CHESS_PIECE(color, type) ((unsigned char)((type) | ((color) << 3)))
#define CHESS_TYPE(p)  ((p) & 7)
#define CHESS_COLOR(p) ((p) >> 3)

#define CHESS_SQ(file, rank) ((rank) * 8 + (file))
#define CHESS_FILE(sq) ((sq) & 7)
#define CHESS_RANK(sq) ((sq) >> 3)

// Nước đi gói trong 32 bit: from(6) | to(6) | phong cấp(3) | cờ(4)
typedef uint32_t ChessMove;
#define CHESS_MOVE_NONE 0u

enum {
    CHESS_FLAG_CAPTURE = 1,
    CHESS_FLAG_EP      = 2,
    CHESS_FLAG_CASTLE  = 4,
    CHESS_FLAG_DOUBLE  = 8
};

#define CHESS_MAKE_MOVE(from, to, promo, flags) \
    ((ChessMove)((from) | ((to) << 6) | ((promo) << 12) | ((flags) << 15)))
#define CHESS_MOVE_FROM(m)  ((int)((m) & 63))
#define CHESS_MOVE_TO(m)    ((int)(((m) >> 6) & 63))
#define CHESS_MOVE_PROMO(m) ((int)(((m) >> 12) & 7))
#define CHESS_MOVE_FLAGS(m) ((int)(((m) >> 15) & 15))

enum {
    CHESS_CASTLE_WK = 1,
    CHESS_CASTLE_WQ = 2,
    CHESS_CASTLE_BK = 4,
    CHESS_CASTLE_BQ = 8
};

#define CHESS_MAX_MOVES   256     // Số nước hợp lệ tối đa của một thế cờ
#define CHESS_MAX_HISTORY 1024    // Độ dài ván + độ sâu tìm kiếm

typedef struct {
    ChessMove move;
    unsigned char captured;
    unsigned char castle;
    signed char ep;
    int halfmove;
    uint64_t hash;
} ChessUndo;

typedef struct {
    unsigned char sq[64];
    int side;                 // ChessColor đang tới lượt
    int castle;               // Quyền nhập thành còn lại
    int ep;                   // Ô bắt tốt qua đường (-1 nếu không có)
    int halfmove;             // Đếm cho luật 50 nước
    int fullmove;
    int king[2];
    uint64_t hash;            // Zobrist
    int ply;                  // Số nước đã đi (chỉ số vào undo[])
    ChessUndo undo[CHESS_MAX_HISTORY];
} ChessBoard;

typedef enum {
    CHESS_ONGOING = 0,
    CHESS_CHECKMATE,
    CHESS_STALEMATE,
    CHESS_DRAW_FIFTY,
    CHESS_DRAW_REPETITION,
    CHESS_DRAW_MATERIAL
} ChessStatus;

void ChessBoardInitStart(ChessBoard *b);

void ChessMakeMove(ChessBoard *b, ChessMove m);
void ChessUnmakeMove(ChessBoard *b);
void ChessMakeNullMove(ChessBoard *b);     // Chỉ dùng trong tìm kiếm
void ChessUnmakeNullMove(ChessBoard *b);

bool ChessSquareAttacked(const ChessBoard *b, int sq, int byColor);
bool ChessInCheck(const ChessBoard *b, int color);

// Sinh nước giả hợp lệ (có thể để vua bị chiếu). capturesOnly: chỉ bắt quân
// và phong cấp - dùng cho tìm kiếm tĩnh.
int ChessGenPseudo(const ChessBoard *b, ChessMove *out, bool capturesOnly);
int ChessGenLegal(ChessBoard *b, ChessMove *out);
bool ChessIsLegal(ChessBoard *b, ChessMove m);

// Số lần thế cờ hiện tại đã xuất hiện (tính cả lần này).
int  ChessRepetitionCount(const ChessBoard *b);
bool ChessInsufficientMaterial(const ChessBoard *b);
ChessStatus ChessGetStatus(ChessBoard *b);

// Ký hiệu đại số chuẩn (SAN), vd "Nf3", "exd5", "O-O", "e8=Q#".
// Gọi TRƯỚC khi đi nước đó.
void ChessMoveToSAN(ChessBoard *b, ChessMove m, char *out, int outSize);
void ChessSquareName(int sq, char *out);   // "e4"

#ifdef __cplusplus
}
#endif

#endif // CHESS_BOARD_H
