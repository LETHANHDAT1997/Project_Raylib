#include "chess_board.h"
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------------ bảng tra

static const int KNIGHT_DF[8] = {1, 2, 2, 1, -1, -2, -2, -1};
static const int KNIGHT_DR[8] = {2, 1, -1, -2, -2, -1, 1, 2};
static const int KING_DF[8]   = {1, 1, 0, -1, -1, -1, 0, 1};
static const int KING_DR[8]   = {0, 1, 1, 1, 0, -1, -1, -1};
// 0..3: hướng xe (N, S, E, W) - 4..7: hướng tượng (NE, NW, SE, SW)
static const int RAY_DF[8]    = {0, 0, 1, -1, 1, -1, 1, -1};
static const int RAY_DR[8]    = {1, -1, 0, 0, 1, 1, -1, -1};

static signed char s_knight[64][8];
static unsigned char s_knightCount[64];
static signed char s_king[64][8];
static unsigned char s_kingCount[64];
static signed char s_ray[64][8][8];
static unsigned char s_rayCount[64][8];

// Quyền nhập thành còn lại sau khi một quân rời / đến ô này.
static unsigned char s_castleMask[64];

static uint64_t s_zPiece[16][64];
static uint64_t s_zSide;
static uint64_t s_zCastle[16];
static uint64_t s_zEp[8];
static bool s_tablesReady = false;

static uint64_t SplitMix(uint64_t *state)
{
    uint64_t z = (*state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static void InitTables(void)
{
    if (s_tablesReady) return;
    for (int sq = 0; sq < 64; sq++) {
        int f = CHESS_FILE(sq), r = CHESS_RANK(sq);
        s_knightCount[sq] = s_kingCount[sq] = 0;
        for (int i = 0; i < 8; i++) {
            int nf = f + KNIGHT_DF[i], nr = r + KNIGHT_DR[i];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) s_knight[sq][s_knightCount[sq]++] = (signed char)CHESS_SQ(nf, nr);
            nf = f + KING_DF[i];
            nr = r + KING_DR[i];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) s_king[sq][s_kingCount[sq]++] = (signed char)CHESS_SQ(nf, nr);
        }
        for (int d = 0; d < 8; d++) {
            s_rayCount[sq][d] = 0;
            int nf = f + RAY_DF[d], nr = r + RAY_DR[d];
            while (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
                s_ray[sq][d][s_rayCount[sq][d]++] = (signed char)CHESS_SQ(nf, nr);
                nf += RAY_DF[d];
                nr += RAY_DR[d];
            }
        }
        s_castleMask[sq] = 15;
    }
    s_castleMask[CHESS_SQ(4, 0)] &= (unsigned char)~(CHESS_CASTLE_WK | CHESS_CASTLE_WQ);
    s_castleMask[CHESS_SQ(7, 0)] &= (unsigned char)~CHESS_CASTLE_WK;
    s_castleMask[CHESS_SQ(0, 0)] &= (unsigned char)~CHESS_CASTLE_WQ;
    s_castleMask[CHESS_SQ(4, 7)] &= (unsigned char)~(CHESS_CASTLE_BK | CHESS_CASTLE_BQ);
    s_castleMask[CHESS_SQ(7, 7)] &= (unsigned char)~CHESS_CASTLE_BK;
    s_castleMask[CHESS_SQ(0, 7)] &= (unsigned char)~CHESS_CASTLE_BQ;

    uint64_t seed = 0xC0FFEE1234ull;
    for (int p = 0; p < 16; p++)
        for (int sq = 0; sq < 64; sq++) s_zPiece[p][sq] = SplitMix(&seed);
    s_zSide = SplitMix(&seed);
    for (int i = 0; i < 16; i++) s_zCastle[i] = SplitMix(&seed);
    for (int i = 0; i < 8; i++) s_zEp[i] = SplitMix(&seed);
    s_tablesReady = true;
}

static uint64_t ComputeHash(const ChessBoard *b)
{
    uint64_t h = 0;
    for (int sq = 0; sq < 64; sq++) {
        if (b->sq[sq]) h ^= s_zPiece[b->sq[sq]][sq];
    }
    if (b->side == CHESS_BLACK) h ^= s_zSide;
    h ^= s_zCastle[b->castle];
    if (b->ep >= 0) h ^= s_zEp[CHESS_FILE(b->ep)];
    return h;
}

// ------------------------------------------------------------------ khởi tạo

void ChessBoardInitStart(ChessBoard *b)
{
    InitTables();
    memset(b, 0, sizeof(*b));
    static const unsigned char back[8] = {
        CHESS_ROOK, CHESS_KNIGHT, CHESS_BISHOP, CHESS_QUEEN, CHESS_KING, CHESS_BISHOP, CHESS_KNIGHT, CHESS_ROOK
    };
    for (int f = 0; f < 8; f++) {
        b->sq[CHESS_SQ(f, 0)] = CHESS_PIECE(CHESS_WHITE, back[f]);
        b->sq[CHESS_SQ(f, 1)] = CHESS_PIECE(CHESS_WHITE, CHESS_PAWN);
        b->sq[CHESS_SQ(f, 6)] = CHESS_PIECE(CHESS_BLACK, CHESS_PAWN);
        b->sq[CHESS_SQ(f, 7)] = CHESS_PIECE(CHESS_BLACK, back[f]);
    }
    b->side = CHESS_WHITE;
    b->castle = CHESS_CASTLE_WK | CHESS_CASTLE_WQ | CHESS_CASTLE_BK | CHESS_CASTLE_BQ;
    b->ep = -1;
    b->halfmove = 0;
    b->fullmove = 1;
    b->king[CHESS_WHITE] = CHESS_SQ(4, 0);
    b->king[CHESS_BLACK] = CHESS_SQ(4, 7);
    b->ply = 0;
    b->hash = ComputeHash(b);
}

// ------------------------------------------------------------------ tấn công

bool ChessSquareAttacked(const ChessBoard *b, int sq, int by)
{
    int f = CHESS_FILE(sq);
    // Tốt: tốt trắng tấn công lên trên, nên nằm ở hàng dưới ô bị tấn công
    if (by == CHESS_WHITE) {
        if (sq >= 8) {
            if (f > 0 && b->sq[sq - 9] == CHESS_PIECE(CHESS_WHITE, CHESS_PAWN)) return true;
            if (f < 7 && b->sq[sq - 7] == CHESS_PIECE(CHESS_WHITE, CHESS_PAWN)) return true;
        }
    } else {
        if (sq < 56) {
            if (f > 0 && b->sq[sq + 7] == CHESS_PIECE(CHESS_BLACK, CHESS_PAWN)) return true;
            if (f < 7 && b->sq[sq + 9] == CHESS_PIECE(CHESS_BLACK, CHESS_PAWN)) return true;
        }
    }
    unsigned char knight = CHESS_PIECE(by, CHESS_KNIGHT);
    for (int i = 0; i < s_knightCount[sq]; i++) {
        if (b->sq[s_knight[sq][i]] == knight) return true;
    }
    unsigned char king = CHESS_PIECE(by, CHESS_KING);
    for (int i = 0; i < s_kingCount[sq]; i++) {
        if (b->sq[s_king[sq][i]] == king) return true;
    }
    unsigned char queen = CHESS_PIECE(by, CHESS_QUEEN);
    unsigned char rook = CHESS_PIECE(by, CHESS_ROOK);
    unsigned char bishop = CHESS_PIECE(by, CHESS_BISHOP);
    for (int d = 0; d < 8; d++) {
        unsigned char slider = (d < 4) ? rook : bishop;
        for (int i = 0; i < s_rayCount[sq][d]; i++) {
            unsigned char p = b->sq[s_ray[sq][d][i]];
            if (!p) continue;
            if (p == slider || p == queen) return true;
            break;
        }
    }
    return false;
}

bool ChessInCheck(const ChessBoard *b, int color)
{
    return ChessSquareAttacked(b, b->king[color], color ^ 1);
}

// ------------------------------------------------------------------ sinh nước

static int AddPawnMoves(int from, int to, int flags, ChessMove *out, int n, bool promote)
{
    if (promote) {
        out[n++] = CHESS_MAKE_MOVE(from, to, CHESS_QUEEN, flags);
        out[n++] = CHESS_MAKE_MOVE(from, to, CHESS_KNIGHT, flags);
        out[n++] = CHESS_MAKE_MOVE(from, to, CHESS_ROOK, flags);
        out[n++] = CHESS_MAKE_MOVE(from, to, CHESS_BISHOP, flags);
    } else {
        out[n++] = CHESS_MAKE_MOVE(from, to, 0, flags);
    }
    return n;
}

int ChessGenPseudo(const ChessBoard *b, ChessMove *out, bool capturesOnly)
{
    int n = 0;
    int us = b->side, them = us ^ 1;
    int dir = (us == CHESS_WHITE) ? 8 : -8;
    int startRank = (us == CHESS_WHITE) ? 1 : 6;
    int promoRank = (us == CHESS_WHITE) ? 7 : 0;

    for (int from = 0; from < 64; from++) {
        unsigned char p = b->sq[from];
        if (!p || CHESS_COLOR(p) != us) continue;
        int type = CHESS_TYPE(p);
        int f = CHESS_FILE(from);

        if (type == CHESS_PAWN) {
            int to = from + dir;
            bool promote = CHESS_RANK(to) == promoRank;
            if (to >= 0 && to < 64 && !b->sq[to]) {
                if (!capturesOnly || promote) {
                    n = AddPawnMoves(from, to, 0, out, n, promote);
                    int two = to + dir;
                    if (!capturesOnly && CHESS_RANK(from) == startRank && !b->sq[two]) {
                        out[n++] = CHESS_MAKE_MOVE(from, two, 0, CHESS_FLAG_DOUBLE);
                    }
                }
            }
            for (int side = -1; side <= 1; side += 2) {
                if ((side < 0 && f == 0) || (side > 0 && f == 7)) continue;
                int cap = from + dir + side;
                if (cap < 0 || cap >= 64) continue;
                if (b->sq[cap] && CHESS_COLOR(b->sq[cap]) == them) {
                    n = AddPawnMoves(from, cap, CHESS_FLAG_CAPTURE, out, n, CHESS_RANK(cap) == promoRank);
                } else if (cap == b->ep) {
                    out[n++] = CHESS_MAKE_MOVE(from, cap, 0, CHESS_FLAG_CAPTURE | CHESS_FLAG_EP);
                }
            }
        } else if (type == CHESS_KNIGHT || type == CHESS_KING) {
            const signed char *targets = (type == CHESS_KNIGHT) ? s_knight[from] : s_king[from];
            int count = (type == CHESS_KNIGHT) ? s_knightCount[from] : s_kingCount[from];
            for (int i = 0; i < count; i++) {
                int to = targets[i];
                unsigned char q = b->sq[to];
                if (!q) {
                    if (!capturesOnly) out[n++] = CHESS_MAKE_MOVE(from, to, 0, 0);
                } else if (CHESS_COLOR(q) == them) {
                    out[n++] = CHESS_MAKE_MOVE(from, to, 0, CHESS_FLAG_CAPTURE);
                }
            }
        } else {
            int d0 = (type == CHESS_BISHOP) ? 4 : 0;
            int d1 = (type == CHESS_ROOK) ? 4 : 8;
            for (int d = d0; d < d1; d++) {
                for (int i = 0; i < s_rayCount[from][d]; i++) {
                    int to = s_ray[from][d][i];
                    unsigned char q = b->sq[to];
                    if (!q) {
                        if (!capturesOnly) out[n++] = CHESS_MAKE_MOVE(from, to, 0, 0);
                        continue;
                    }
                    if (CHESS_COLOR(q) == them) out[n++] = CHESS_MAKE_MOVE(from, to, 0, CHESS_FLAG_CAPTURE);
                    break;
                }
            }
        }
    }

    // Nhập thành: vua không bị chiếu, không đi qua / tới ô bị khống chế
    if (!capturesOnly) {
        int r = (us == CHESS_WHITE) ? 0 : 7;
        int k = CHESS_SQ(4, r);
        int kRight = (us == CHESS_WHITE) ? CHESS_CASTLE_WK : CHESS_CASTLE_BK;
        int qRight = (us == CHESS_WHITE) ? CHESS_CASTLE_WQ : CHESS_CASTLE_BQ;
        if ((b->castle & (kRight | qRight)) && b->sq[k] == CHESS_PIECE(us, CHESS_KING) &&
            !ChessSquareAttacked(b, k, them)) {
            if ((b->castle & kRight) && !b->sq[k + 1] && !b->sq[k + 2] &&
                b->sq[k + 3] == CHESS_PIECE(us, CHESS_ROOK) &&
                !ChessSquareAttacked(b, k + 1, them) && !ChessSquareAttacked(b, k + 2, them)) {
                out[n++] = CHESS_MAKE_MOVE(k, k + 2, 0, CHESS_FLAG_CASTLE);
            }
            if ((b->castle & qRight) && !b->sq[k - 1] && !b->sq[k - 2] && !b->sq[k - 3] &&
                b->sq[k - 4] == CHESS_PIECE(us, CHESS_ROOK) &&
                !ChessSquareAttacked(b, k - 1, them) && !ChessSquareAttacked(b, k - 2, them)) {
                out[n++] = CHESS_MAKE_MOVE(k, k - 2, 0, CHESS_FLAG_CASTLE);
            }
        }
    }
    return n;
}

// ------------------------------------------------------------------ đi / lùi

static void MovePiece(ChessBoard *b, int from, int to)
{
    unsigned char p = b->sq[from];
    b->hash ^= s_zPiece[p][from] ^ s_zPiece[p][to];
    b->sq[to] = p;
    b->sq[from] = 0;
}

void ChessMakeMove(ChessBoard *b, ChessMove m)
{
    int from = CHESS_MOVE_FROM(m), to = CHESS_MOVE_TO(m);
    int flags = CHESS_MOVE_FLAGS(m), promo = CHESS_MOVE_PROMO(m);
    int us = b->side;
    unsigned char piece = b->sq[from];

    ChessUndo *u = &b->undo[b->ply++];
    u->move = m;
    u->castle = (unsigned char)b->castle;
    u->ep = (signed char)b->ep;
    u->halfmove = b->halfmove;
    u->hash = b->hash;
    u->captured = 0;

    if (b->ep >= 0) b->hash ^= s_zEp[CHESS_FILE(b->ep)];
    b->ep = -1;
    b->halfmove++;

    if (flags & CHESS_FLAG_EP) {
        int capSq = to + (us == CHESS_WHITE ? -8 : 8);
        u->captured = b->sq[capSq];
        b->hash ^= s_zPiece[b->sq[capSq]][capSq];
        b->sq[capSq] = 0;
    } else if (b->sq[to]) {
        u->captured = b->sq[to];
        b->hash ^= s_zPiece[b->sq[to]][to];
        b->sq[to] = 0;
    }
    if (u->captured || CHESS_TYPE(piece) == CHESS_PAWN) b->halfmove = 0;

    MovePiece(b, from, to);

    if (promo) {
        unsigned char np = CHESS_PIECE(us, promo);
        b->hash ^= s_zPiece[piece][to] ^ s_zPiece[np][to];
        b->sq[to] = np;
    }
    if (flags & CHESS_FLAG_CASTLE) {
        if (to > from) MovePiece(b, to + 1, to - 1);   // Nhập thành cánh vua
        else MovePiece(b, to - 2, to + 1);             // Nhập thành cánh hậu
    }
    if (flags & CHESS_FLAG_DOUBLE) {
        b->ep = (from + to) / 2;
        b->hash ^= s_zEp[CHESS_FILE(b->ep)];
    }
    if (CHESS_TYPE(piece) == CHESS_KING) b->king[us] = to;

    b->hash ^= s_zCastle[b->castle];
    b->castle &= s_castleMask[from] & s_castleMask[to];
    b->hash ^= s_zCastle[b->castle];

    if (us == CHESS_BLACK) b->fullmove++;
    b->side ^= 1;
    b->hash ^= s_zSide;
}

void ChessUnmakeMove(ChessBoard *b)
{
    if (b->ply <= 0) return;
    ChessUndo *u = &b->undo[--b->ply];
    ChessMove m = u->move;
    int from = CHESS_MOVE_FROM(m), to = CHESS_MOVE_TO(m);
    int flags = CHESS_MOVE_FLAGS(m);

    b->side ^= 1;
    int us = b->side;
    if (us == CHESS_BLACK) b->fullmove--;

    unsigned char piece = b->sq[to];
    if (CHESS_MOVE_PROMO(m)) piece = CHESS_PIECE(us, CHESS_PAWN);
    b->sq[from] = piece;
    b->sq[to] = 0;

    if (flags & CHESS_FLAG_EP) {
        b->sq[to + (us == CHESS_WHITE ? -8 : 8)] = u->captured;
    } else if (u->captured) {
        b->sq[to] = u->captured;
    }
    if (flags & CHESS_FLAG_CASTLE) {
        if (to > from) {
            b->sq[to + 1] = b->sq[to - 1];
            b->sq[to - 1] = 0;
        } else {
            b->sq[to - 2] = b->sq[to + 1];
            b->sq[to + 1] = 0;
        }
    }
    if (CHESS_TYPE(piece) == CHESS_KING) b->king[us] = from;

    b->castle = u->castle;
    b->ep = u->ep;
    b->halfmove = u->halfmove;
    b->hash = u->hash;
}

void ChessMakeNullMove(ChessBoard *b)
{
    ChessUndo *u = &b->undo[b->ply++];
    u->move = CHESS_MOVE_NONE;
    u->castle = (unsigned char)b->castle;
    u->ep = (signed char)b->ep;
    u->halfmove = b->halfmove;
    u->hash = b->hash;
    u->captured = 0;
    if (b->ep >= 0) b->hash ^= s_zEp[CHESS_FILE(b->ep)];
    b->ep = -1;
    b->side ^= 1;
    b->hash ^= s_zSide;
}

void ChessUnmakeNullMove(ChessBoard *b)
{
    ChessUndo *u = &b->undo[--b->ply];
    b->side ^= 1;
    b->ep = u->ep;
    b->hash = u->hash;
}

// ------------------------------------------------------------------ hợp lệ

int ChessGenLegal(ChessBoard *b, ChessMove *out)
{
    ChessMove pseudo[CHESS_MAX_MOVES];
    int count = ChessGenPseudo(b, pseudo, false);
    int n = 0;
    int us = b->side;
    for (int i = 0; i < count; i++) {
        ChessMakeMove(b, pseudo[i]);
        if (!ChessInCheck(b, us)) out[n++] = pseudo[i];
        ChessUnmakeMove(b);
    }
    return n;
}

bool ChessIsLegal(ChessBoard *b, ChessMove m)
{
    ChessMove moves[CHESS_MAX_MOVES];
    int n = ChessGenLegal(b, moves);
    for (int i = 0; i < n; i++) {
        if (moves[i] == m) return true;
    }
    return false;
}

int ChessRepetitionCount(const ChessBoard *b)
{
    int count = 1;
    // Chỉ cần xét lùi tới nước bắt quân / đi tốt gần nhất.
    int limit = b->ply - b->halfmove;
    if (limit < 0) limit = 0;
    for (int i = b->ply - 2; i >= limit; i -= 2) {
        if (b->undo[i].hash == b->hash) count++;
    }
    return count;
}

bool ChessInsufficientMaterial(const ChessBoard *b)
{
    int minors = 0, bishopsColor[2] = {0, 0}, bishops = 0;
    for (int sq = 0; sq < 64; sq++) {
        unsigned char p = b->sq[sq];
        if (!p) continue;
        int t = CHESS_TYPE(p);
        if (t == CHESS_KING) continue;
        if (t == CHESS_PAWN || t == CHESS_ROOK || t == CHESS_QUEEN) return false;
        minors++;
        if (t == CHESS_BISHOP) {
            bishops++;
            bishopsColor[(CHESS_FILE(sq) + CHESS_RANK(sq)) & 1]++;
        }
    }
    if (minors <= 1) return true;                                           // K-K, K+N-K, K+B-K
    if (minors == bishops && (bishopsColor[0] == 0 || bishopsColor[1] == 0)) return true;   // Toàn tượng cùng màu ô
    return false;
}

ChessStatus ChessGetStatus(ChessBoard *b)
{
    ChessMove moves[CHESS_MAX_MOVES];
    int n = ChessGenLegal(b, moves);
    if (n == 0) return ChessInCheck(b, b->side) ? CHESS_CHECKMATE : CHESS_STALEMATE;
    if (b->halfmove >= 100) return CHESS_DRAW_FIFTY;
    if (ChessRepetitionCount(b) >= 3) return CHESS_DRAW_REPETITION;
    if (ChessInsufficientMaterial(b)) return CHESS_DRAW_MATERIAL;
    return CHESS_ONGOING;
}

// ------------------------------------------------------------------ ký hiệu

void ChessSquareName(int sq, char *out)
{
    out[0] = (char)('a' + CHESS_FILE(sq));
    out[1] = (char)('1' + CHESS_RANK(sq));
    out[2] = '\0';
}

void ChessMoveToSAN(ChessBoard *b, ChessMove m, char *out, int outSize)
{
    static const char LETTER[CHESS_PIECE_TYPES] = {'?', 'P', 'N', 'B', 'R', 'Q', 'K'};
    char buf[16];
    int len = 0;
    int from = CHESS_MOVE_FROM(m), to = CHESS_MOVE_TO(m);
    int flags = CHESS_MOVE_FLAGS(m);
    int type = CHESS_TYPE(b->sq[from]);
    char toName[3];
    ChessSquareName(to, toName);

    if (flags & CHESS_FLAG_CASTLE) {
        len = snprintf(buf, sizeof(buf), "%s", to > from ? "O-O" : "O-O-O");
    } else if (type == CHESS_PAWN) {
        if (flags & CHESS_FLAG_CAPTURE) buf[len++] = (char)('a' + CHESS_FILE(from)), buf[len++] = 'x';
        buf[len++] = toName[0];
        buf[len++] = toName[1];
        if (CHESS_MOVE_PROMO(m)) {
            buf[len++] = '=';
            buf[len++] = LETTER[CHESS_MOVE_PROMO(m)];
        }
        buf[len] = '\0';
    } else {
        buf[len++] = LETTER[type];
        // Phân biệt khi có quân cùng loại khác cũng tới được ô đích
        ChessMove moves[CHESS_MAX_MOVES];
        int n = ChessGenLegal(b, moves);
        bool ambiguous = false, sameFile = false, sameRank = false;
        for (int i = 0; i < n; i++) {
            int of = CHESS_MOVE_FROM(moves[i]);
            if (of == from || CHESS_MOVE_TO(moves[i]) != to || CHESS_TYPE(b->sq[of]) != type) continue;
            ambiguous = true;
            if (CHESS_FILE(of) == CHESS_FILE(from)) sameFile = true;
            if (CHESS_RANK(of) == CHESS_RANK(from)) sameRank = true;
        }
        if (ambiguous) {
            if (!sameFile) buf[len++] = (char)('a' + CHESS_FILE(from));
            else if (!sameRank) buf[len++] = (char)('1' + CHESS_RANK(from));
            else {
                buf[len++] = (char)('a' + CHESS_FILE(from));
                buf[len++] = (char)('1' + CHESS_RANK(from));
            }
        }
        if (flags & CHESS_FLAG_CAPTURE) buf[len++] = 'x';
        buf[len++] = toName[0];
        buf[len++] = toName[1];
        buf[len] = '\0';
    }

    // Chiếu / chiếu hết
    ChessMakeMove(b, m);
    if (ChessInCheck(b, b->side)) {
        ChessMove replies[CHESS_MAX_MOVES];
        bool mate = ChessGenLegal(b, replies) == 0;
        len = (int)strlen(buf);
        buf[len++] = mate ? '#' : '+';
        buf[len] = '\0';
    }
    ChessUnmakeMove(b);

    snprintf(out, (size_t)outSize, "%s", buf);
}
