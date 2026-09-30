#define _POSIX_C_SOURCE 200809L   // clock_gettime
#include "chess_ai.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INF        32000
#define MATE       30000
#define MATE_BOUND 29000
#define MAX_PLY    64

// ------------------------------------------------------------------ đánh giá

static const int PIECE_VALUE[CHESS_PIECE_TYPES] = {0, 100, 320, 330, 500, 900, 0};
static const int PHASE_WEIGHT[CHESS_PIECE_TYPES] = {0, 0, 1, 1, 2, 4, 0};

// Bảng vị trí viết theo hình bàn cờ nhìn từ phía trắng (hàng 8 ở trên cùng).
static const int PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
static const int PST_KNIGHT[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};
static const int PST_BISHOP[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};
static const int PST_ROOK[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};
static const int PST_QUEEN[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};
static const int PST_KING_MG[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};
static const int PST_KING_EG[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};
static const int *PST[CHESS_PIECE_TYPES] = {NULL, PST_PAWN, PST_KNIGHT, PST_BISHOP, PST_ROOK, PST_QUEEN, NULL};

// Thưởng tốt thông theo hàng (tính từ phía quân mình).
static const int PASSED_BONUS[8] = {0, 5, 10, 20, 35, 60, 100, 0};

static int PstIndex(int sq, int color)
{
    // Bảng viết hàng 8 ở trên: quân trắng lật dọc, quân đen đọc thẳng.
    return (color == CHESS_WHITE) ? (7 - CHESS_RANK(sq)) * 8 + CHESS_FILE(sq) : sq;
}

static int Distance(int a, int b)
{
    int df = abs(CHESS_FILE(a) - CHESS_FILE(b)), dr = abs(CHESS_RANK(a) - CHESS_RANK(b));
    return df > dr ? df : dr;
}

static int CenterDistance(int sq)
{
    int f = CHESS_FILE(sq), r = CHESS_RANK(sq);
    int df = f < 4 ? 3 - f : f - 4;
    int dr = r < 4 ? 3 - r : r - 4;
    return df + dr;
}

// Điểm tĩnh theo góc nhìn của bên đang tới lượt.
static int Evaluate(const ChessBoard *b)
{
    int mg[2] = {0, 0}, eg[2] = {0, 0};
    int material[2] = {0, 0};
    int phase = 0;
    int pawnFiles[2][8];
    int bishops[2] = {0, 0};
    memset(pawnFiles, 0, sizeof(pawnFiles));

    for (int sq = 0; sq < 64; sq++) {
        unsigned char p = b->sq[sq];
        if (!p) continue;
        int t = CHESS_TYPE(p), c = CHESS_COLOR(p);
        phase += PHASE_WEIGHT[t];
        if (t == CHESS_PAWN) pawnFiles[c][CHESS_FILE(sq)]++;
        if (t == CHESS_BISHOP) bishops[c]++;
        if (t == CHESS_KING) {
            mg[c] += PST_KING_MG[PstIndex(sq, c)];
            eg[c] += PST_KING_EG[PstIndex(sq, c)];
            continue;
        }
        int v = PIECE_VALUE[t] + PST[t][PstIndex(sq, c)];
        material[c] += PIECE_VALUE[t];
        mg[c] += v;
        eg[c] += v;
    }

    // Cấu trúc tốt, tốt thông, xe cột mở
    for (int sq = 0; sq < 64; sq++) {
        unsigned char p = b->sq[sq];
        if (!p) continue;
        int t = CHESS_TYPE(p), c = CHESS_COLOR(p), f = CHESS_FILE(sq);
        if (t == CHESS_PAWN) {
            bool left = f > 0 && pawnFiles[c][f - 1];
            bool right = f < 7 && pawnFiles[c][f + 1];
            if (!left && !right) { mg[c] -= 12; eg[c] -= 16; }
            if (pawnFiles[c][f] > 1) { mg[c] -= 8; eg[c] -= 14; }

            // Tốt thông: không tốt đối phương nào chắn phía trước trên 3 cột
            bool passed = true;
            int r = CHESS_RANK(sq);
            for (int ff = f - 1; ff <= f + 1 && passed; ff++) {
                if (ff < 0 || ff > 7) continue;
                for (int rr = (c == CHESS_WHITE ? r + 1 : 0); rr < (c == CHESS_WHITE ? 8 : r); rr++) {
                    if (b->sq[CHESS_SQ(ff, rr)] == CHESS_PIECE(c ^ 1, CHESS_PAWN)) { passed = false; break; }
                }
            }
            if (passed) {
                int rel = (c == CHESS_WHITE) ? r : 7 - r;
                mg[c] += PASSED_BONUS[rel] / 2;
                eg[c] += PASSED_BONUS[rel];
            }
        } else if (t == CHESS_ROOK) {
            if (!pawnFiles[c][f]) {
                int bonus = pawnFiles[c ^ 1][f] ? 10 : 20;
                mg[c] += bonus;
                eg[c] += bonus / 2;
            }
        }
    }
    for (int c = 0; c < 2; c++) {
        if (bishops[c] >= 2) { mg[c] += 30; eg[c] += 40; }
    }

    if (phase > 24) phase = 24;
    int white = (mg[0] - mg[1]) * phase / 24 + (eg[0] - eg[1]) * (24 - phase) / 24;

    // Tàn cuộc thắng thế: dồn vua đối phương ra mép và đưa vua mình lại gần
    // để máy biết cách chiếu hết (vd. vua + hậu đấu vua).
    int diff = material[0] - material[1];
    if (phase < 10 && abs(diff) >= 300) {
        int strong = diff > 0 ? CHESS_WHITE : CHESS_BLACK;
        int weakKing = b->king[strong ^ 1];
        int mop = CenterDistance(weakKing) * 10 + (14 - Distance(b->king[0], b->king[1])) * 4;
        white += (strong == CHESS_WHITE) ? mop : -mop;
    }

    return (b->side == CHESS_WHITE) ? white : -white;
}

// ------------------------------------------------------------------ bảng chuyển vị

enum { TT_EXACT = 0, TT_LOWER = 1, TT_UPPER = 2 };

typedef struct {
    uint64_t key;
    ChessMove move;
    int16_t score;
    int8_t depth;
    uint8_t flag;
} TTEntry;

#define TT_BITS 19
#define TT_SIZE (1u << TT_BITS)

static TTEntry *s_tt = NULL;

static int ScoreToTT(int score, int ply)
{
    if (score > MATE_BOUND) return score + ply;
    if (score < -MATE_BOUND) return score - ply;
    return score;
}

static int ScoreFromTT(int score, int ply)
{
    if (score > MATE_BOUND) return score - ply;
    if (score < -MATE_BOUND) return score + ply;
    return score;
}

// ------------------------------------------------------------------ ngữ cảnh tìm kiếm

typedef struct {
    ChessBoard b;
    ChessMove killers[MAX_PLY][2];
    int history[2][64][64];
    double deadline;
    bool timeUp;
    long nodes;
    unsigned int rng;
} SearchCtx;

typedef struct {
    ChessBoard board;
    ChessAILevel level;
} AIJob;

static SearchCtx s_ctx;
static AIJob s_job;
static pthread_t s_thread;
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static bool s_running = false;
static bool s_done = false;
static ChessMove s_result = CHESS_MOVE_NONE;
static volatile bool s_abort = false;

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static unsigned int NextRand(SearchCtx *c)
{
    unsigned int x = c->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    c->rng = x ? x : 0x2545F491u;
    return c->rng;
}

static int RandRange(SearchCtx *c, int lo, int hi)
{
    return lo + (int)(NextRand(c) % (unsigned int)(hi - lo + 1));
}

static bool CheckTime(SearchCtx *c)
{
    if ((++c->nodes & 2047) == 0) {
        if (s_abort || NowSeconds() > c->deadline) c->timeUp = true;
    }
    return c->timeUp;
}

// ------------------------------------------------------------------ sắp xếp nước

static int MoveOrderScore(const SearchCtx *c, ChessMove m, ChessMove ttMove, int ply)
{
    if (m == ttMove) return 1000000;
    const ChessBoard *b = &c->b;
    int from = CHESS_MOVE_FROM(m), to = CHESS_MOVE_TO(m);
    int flags = CHESS_MOVE_FLAGS(m);
    if (flags & CHESS_FLAG_CAPTURE) {
        int victim = (flags & CHESS_FLAG_EP) ? CHESS_PAWN : CHESS_TYPE(b->sq[to]);
        int attacker = CHESS_TYPE(b->sq[from]);
        return 100000 + PIECE_VALUE[victim] * 10 - attacker;
    }
    if (CHESS_MOVE_PROMO(m)) return 90000 + PIECE_VALUE[CHESS_MOVE_PROMO(m)];
    if (ply < MAX_PLY) {
        if (c->killers[ply][0] == m) return 80000;
        if (c->killers[ply][1] == m) return 79000;
    }
    int h = c->history[b->side][from][to];
    return h > 70000 ? 70000 : h;
}

static void PickNext(ChessMove *moves, int *scores, int count, int start)
{
    int best = start;
    for (int i = start + 1; i < count; i++) {
        if (scores[i] > scores[best]) best = i;
    }
    if (best != start) {
        ChessMove tm = moves[start]; moves[start] = moves[best]; moves[best] = tm;
        int ts = scores[start]; scores[start] = scores[best]; scores[best] = ts;
    }
}

// ------------------------------------------------------------------ tìm kiếm

static int Quiesce(SearchCtx *c, int alpha, int beta, int ply)
{
    if (CheckTime(c)) return 0;
    int standPat = Evaluate(&c->b);
    if (ply >= MAX_PLY - 1) return standPat;
    if (standPat >= beta) return standPat;
    if (standPat > alpha) alpha = standPat;

    ChessMove moves[CHESS_MAX_MOVES];
    int scores[CHESS_MAX_MOVES];
    int n = ChessGenPseudo(&c->b, moves, true);
    for (int i = 0; i < n; i++) scores[i] = MoveOrderScore(c, moves[i], CHESS_MOVE_NONE, MAX_PLY);

    int us = c->b.side;
    for (int i = 0; i < n; i++) {
        PickNext(moves, scores, n, i);
        ChessMakeMove(&c->b, moves[i]);
        if (ChessInCheck(&c->b, us)) {
            ChessUnmakeMove(&c->b);
            continue;
        }
        int score = -Quiesce(c, -beta, -alpha, ply + 1);
        ChessUnmakeMove(&c->b);
        if (c->timeUp) return 0;
        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

static bool HasNonPawnMaterial(const ChessBoard *b, int color)
{
    for (int sq = 0; sq < 64; sq++) {
        unsigned char p = b->sq[sq];
        if (p && CHESS_COLOR(p) == color && CHESS_TYPE(p) != CHESS_PAWN && CHESS_TYPE(p) != CHESS_KING) return true;
    }
    return false;
}

static int Negamax(SearchCtx *c, int depth, int alpha, int beta, int ply, bool allowNull)
{
    ChessBoard *b = &c->b;
    if (CheckTime(c)) return 0;
    if (ply > 0 && (b->halfmove >= 100 || ChessRepetitionCount(b) >= 2)) return 0;
    if (ply >= MAX_PLY - 1) return Evaluate(b);

    int us = b->side;
    bool inCheck = ChessInCheck(b, us);
    if (inCheck) depth++;                  // Gia hạn khi bị chiếu
    if (depth <= 0) return Quiesce(c, alpha, beta, ply);

    // Tra bảng chuyển vị
    TTEntry *e = &s_tt[b->hash & (TT_SIZE - 1)];
    ChessMove ttMove = CHESS_MOVE_NONE;
    if (e->key == b->hash) {
        ttMove = e->move;
        if (ply > 0 && e->depth >= depth) {
            int s = ScoreFromTT(e->score, ply);
            if (e->flag == TT_EXACT) return s;
            if (e->flag == TT_LOWER && s >= beta) return s;
            if (e->flag == TT_UPPER && s <= alpha) return s;
        }
    }

    bool pvNode = (beta - alpha) > 1;

    // Nước rỗng: nhường lượt mà vẫn vượt beta thì cắt luôn
    if (allowNull && !pvNode && !inCheck && depth >= 3 && ply > 0 && HasNonPawnMaterial(b, us) &&
        Evaluate(b) >= beta) {
        ChessMakeNullMove(b);
        int score = -Negamax(c, depth - 3, -beta, -beta + 1, ply + 1, false);
        ChessUnmakeNullMove(b);
        if (c->timeUp) return 0;
        if (score >= beta) return beta;
    }

    ChessMove moves[CHESS_MAX_MOVES];
    int scores[CHESS_MAX_MOVES];
    int n = ChessGenPseudo(b, moves, false);
    for (int i = 0; i < n; i++) scores[i] = MoveOrderScore(c, moves[i], ttMove, ply);

    int best = -INF;
    ChessMove bestMove = CHESS_MOVE_NONE;
    int origAlpha = alpha;
    int legal = 0;

    for (int i = 0; i < n; i++) {
        PickNext(moves, scores, n, i);
        ChessMove m = moves[i];
        ChessMakeMove(b, m);
        if (ChessInCheck(b, us)) {
            ChessUnmakeMove(b);
            continue;
        }
        legal++;
        bool quiet = !(CHESS_MOVE_FLAGS(m) & CHESS_FLAG_CAPTURE) && !CHESS_MOVE_PROMO(m);
        bool givesCheck = ChessInCheck(b, us ^ 1);

        int score;
        if (legal == 1) {
            score = -Negamax(c, depth - 1, -beta, -alpha, ply + 1, true);
        } else {
            // Giảm độ sâu cho nước yên tĩnh xếp muộn, sai thì tìm lại đủ sâu
            int reduction = (depth >= 3 && legal > 4 && quiet && !inCheck && !givesCheck) ? 1 : 0;
            score = -Negamax(c, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, true);
            if (score > alpha && reduction) score = -Negamax(c, depth - 1, -alpha - 1, -alpha, ply + 1, true);
            if (score > alpha && score < beta) score = -Negamax(c, depth - 1, -beta, -alpha, ply + 1, true);
        }
        ChessUnmakeMove(b);
        if (c->timeUp) return 0;

        if (score > best) {
            best = score;
            bestMove = m;
        }
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            if (quiet && ply < MAX_PLY) {
                if (c->killers[ply][0] != m) {
                    c->killers[ply][1] = c->killers[ply][0];
                    c->killers[ply][0] = m;
                }
                c->history[us][CHESS_MOVE_FROM(m)][CHESS_MOVE_TO(m)] += depth * depth;
            }
            break;
        }
    }

    if (legal == 0) return inCheck ? -MATE + ply : 0;

    e->key = b->hash;
    e->move = bestMove;
    e->score = (int16_t)ScoreToTT(best, ply);
    e->depth = (int8_t)depth;
    e->flag = (best >= beta) ? TT_LOWER : (best <= origAlpha ? TT_UPPER : TT_EXACT);
    return best;
}

// ------------------------------------------------------------------ các mức độ

typedef struct {
    ChessMove move;
    int score;
} RootMove;

// Chấm điểm từng nước ở gốc bằng cửa sổ đầy đủ (đắt hơn PVS nhưng có điểm
// thật của mọi nước để cộng nhiễu). Dùng cho mức Dễ và Thường.
static int ScoreRootMoves(SearchCtx *c, int depth, RootMove *out)
{
    ChessMove moves[CHESS_MAX_MOVES];
    int n = ChessGenLegal(&c->b, moves);
    for (int i = 0; i < n; i++) {
        ChessMakeMove(&c->b, moves[i]);
        int s = (depth <= 1) ? -Quiesce(c, -INF, INF, 1) : -Negamax(c, depth - 1, -INF, INF, 1, true);
        ChessUnmakeMove(&c->b);
        out[i].move = moves[i];
        out[i].score = s;
        if (c->timeUp) return i + 1;
    }
    return n;
}

static ChessMove PickNoisy(SearchCtx *c, int depth, int noise, int randomPercent)
{
    RootMove roots[CHESS_MAX_MOVES];
    int n = ScoreRootMoves(c, depth, roots);
    if (n == 0) return CHESS_MOVE_NONE;

    // Luôn tận dụng nước chiếu hết ngay nếu thấy.
    for (int i = 0; i < n; i++) {
        if (roots[i].score > MATE_BOUND) return roots[i].move;
    }
    if (randomPercent > 0 && RandRange(c, 1, 100) <= randomPercent) {
        // Nước "ngẫu hứng" nhưng tránh thí quân vô lý (mất hơn 3 tốt).
        int bestScore = -INF;
        for (int i = 0; i < n; i++) if (roots[i].score > bestScore) bestScore = roots[i].score;
        RootMove ok[CHESS_MAX_MOVES];
        int k = 0;
        for (int i = 0; i < n; i++) if (roots[i].score >= bestScore - 300) ok[k++] = roots[i];
        if (k > 0) return ok[RandRange(c, 0, k - 1)].move;
    }
    int best = -INF * 2;
    ChessMove bestMove = CHESS_MOVE_NONE;
    for (int i = 0; i < n; i++) {
        int s = roots[i].score + (noise > 0 ? RandRange(c, -noise, noise) : 0);
        if (s > best) {
            best = s;
            bestMove = roots[i].move;
        }
    }
    return bestMove;
}

static ChessMove SearchIterative(SearchCtx *c, double seconds, int maxDepth)
{
    double start = NowSeconds();
    c->deadline = start + seconds;

    ChessMove moves[CHESS_MAX_MOVES];
    int n = ChessGenLegal(&c->b, moves);
    if (n == 0) return CHESS_MOVE_NONE;
    if (n == 1) return moves[0];

    ChessMove best = moves[0];
    int us = c->b.side;
    for (int depth = 1; depth <= maxDepth; depth++) {
        // Gốc tự quản lý để giữ được nước tốt nhất của vòng dở dang.
        int alpha = -INF, beta = INF;
        ChessMove iterBest = CHESS_MOVE_NONE;
        int iterScore = -INF;
        int scores[CHESS_MAX_MOVES];
        for (int i = 0; i < n; i++) scores[i] = (moves[i] == best) ? 2000000 : MoveOrderScore(c, moves[i], CHESS_MOVE_NONE, 0);

        for (int i = 0; i < n; i++) {
            PickNext(moves, scores, n, i);
            ChessMove m = moves[i];
            ChessMakeMove(&c->b, m);
            int score;
            if (i == 0) {
                score = -Negamax(c, depth - 1, -beta, -alpha, 1, true);
            } else {
                score = -Negamax(c, depth - 1, -alpha - 1, -alpha, 1, true);
                if (score > alpha && !c->timeUp) score = -Negamax(c, depth - 1, -beta, -alpha, 1, true);
            }
            ChessUnmakeMove(&c->b);
            if (c->timeUp) break;
            if (score > iterScore) {
                iterScore = score;
                iterBest = m;
            }
            if (score > alpha) alpha = score;
        }
        (void)us;

        if (iterBest != CHESS_MOVE_NONE) best = iterBest;
        if (c->timeUp) break;
        if (iterScore > MATE_BOUND || iterScore < -MATE_BOUND) break;   // Đã thấy chiếu hết
        // Không đủ thời gian cho vòng sau (thường tốn gấp vài lần vòng trước)
        if (NowSeconds() - start > seconds * 0.55) break;
    }
    return best;
}

// ------------------------------------------------------------------ luồng

static void *AIThreadMain(void *arg)
{
    (void)arg;
    SearchCtx *c = &s_ctx;
    c->b = s_job.board;
    memset(c->killers, 0, sizeof(c->killers));
    memset(c->history, 0, sizeof(c->history));
    c->timeUp = false;
    c->nodes = 0;
    c->rng = (unsigned int)(NowSeconds() * 1000003.0) | 1u;
    if (s_tt) memset(s_tt, 0, sizeof(TTEntry) * TT_SIZE);

    ChessMove move = CHESS_MOVE_NONE;
    switch (s_job.level) {
        case CHESS_AI_EASY:
            c->deadline = NowSeconds() + 1.0;
            move = PickNoisy(c, 1, 140, 12);
            break;
        case CHESS_AI_NORMAL:
            c->deadline = NowSeconds() + 2.5;
            move = PickNoisy(c, 3, 18, 0);
            if (c->timeUp) {           // Máy chậm: lùi về độ sâu 2
                c->timeUp = false;
                c->deadline = NowSeconds() + 1.0;
                move = PickNoisy(c, 2, 18, 0);
            }
            break;
        case CHESS_AI_HARD:
            move = SearchIterative(c, 2.5, 30);
            break;
        case CHESS_AI_HINT:
        default:
            move = SearchIterative(c, 1.2, 30);
            break;
    }
    if (move == CHESS_MOVE_NONE) {
        ChessMove moves[CHESS_MAX_MOVES];
        c->b = s_job.board;
        if (ChessGenLegal(&c->b, moves) > 0) move = moves[0];
    }

    pthread_mutex_lock(&s_lock);
    s_result = move;
    s_done = true;
    pthread_mutex_unlock(&s_lock);
    return NULL;
}

void ChessAIRequest(const ChessBoard *board, ChessAILevel level)
{
    if (s_running) return;
    if (!s_tt) s_tt = (TTEntry *)calloc(TT_SIZE, sizeof(TTEntry));
    s_job.board = *board;
    s_job.level = level;
    s_done = false;
    s_result = CHESS_MOVE_NONE;
    s_abort = false;
    if (pthread_create(&s_thread, NULL, AIThreadMain, NULL) == 0) {
        s_running = true;
    } else {
        AIThreadMain(NULL);
    }
}

bool ChessAIPoll(ChessMove *outMove)
{
    pthread_mutex_lock(&s_lock);
    bool done = s_done;
    ChessMove result = s_result;
    if (done) s_done = false;
    pthread_mutex_unlock(&s_lock);
    if (!done) return false;
    if (s_running) {
        pthread_join(s_thread, NULL);
        s_running = false;
    }
    if (outMove) *outMove = result;
    return true;
}

bool ChessAIBusy(void)
{
    return s_running;
}

void ChessAICancel(void)
{
    if (s_running) {
        s_abort = true;
        pthread_join(s_thread, NULL);
        s_running = false;
    }
    s_done = false;
    s_abort = false;
}
