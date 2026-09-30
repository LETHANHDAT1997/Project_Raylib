#define _POSIX_C_SOURCE 200809L   // clock_gettime
#include "caro_ai.h"
#include "caro_rules.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Đánh giá theo "bộ năm ô" (window): mọi đoạn 5 ô liên tiếp trên bàn. Một
// đoạn chỉ còn giá trị với một bên khi bên kia chưa có quân nào trong đó.
// Đặt một quân chỉ làm thay đổi tối đa 20 đoạn, nên cập nhật tăng dần rất rẻ.
// ---------------------------------------------------------------------------

#define MAX_WINDOWS 1100
#define MAX_AFFECT  28
#define WIN_SCORE   100000000
#define MAX_CANDS   CARO_MAX_CELLS

// Điểm xếp hạng nước đi (bảng kinh điển của AI caro dạng bộ năm):
// OWN[k] = đoạn đã có k quân mình, OPP[k] = đoạn có k quân đối phương.
static const int OWN_TUPLE[5] = {7, 35, 800, 15000, 800000};
static const int OPP_TUPLE[5] = {7, 15, 400, 1800, 100000};

// Điểm thế cờ ở lá của cây tìm kiếm.
static const int EVAL_TUPLE[6] = {0, 1, 12, 150, 2500, 1000000};

typedef struct {
    CaroBoard b;
    int cellCount;

    int windowCount;
    short win[MAX_WINDOWS][5];
    short outA[MAX_WINDOWS], outB[MAX_WINDOWS];    // Ô ngay ngoài hai đầu đoạn (-1 = ngoài bàn)
    unsigned char cnt[MAX_WINDOWS][3];             // cnt[w][CARO_X], cnt[w][CARO_O]

    short cellWin[CARO_MAX_CELLS][20];             // Các đoạn chứa ô
    unsigned char cellWinN[CARO_MAX_CELLS];
    short cellAff[CARO_MAX_CELLS][MAX_AFFECT];     // Các đoạn bị ảnh hưởng khi đặt vào ô
    unsigned char cellAffN[CARO_MAX_CELLS];
    unsigned char nearCount[CARO_MAX_CELLS];       // Số quân trong vùng 5x5 quanh ô

    int evalSum[3];                                // Tổng điểm thế cờ theo từng bên

    double deadline;
    bool timeUp;
    long nodes;
    unsigned int rng;
} AICtx;

typedef struct {
    CaroBoard board;
    CaroDifficulty difficulty;
} AIJob;

static AICtx s_ctx;
static AIJob s_job;
static pthread_t s_thread;
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static bool s_running = false;     // Luồng đang tồn tại (chưa join)
static bool s_done = false;
static int s_result = -1;
static volatile bool s_abort = false;

// ------------------------------------------------------------------ tiện ích

static double NowSeconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static unsigned int NextRand(AICtx *c)
{
    // xorshift32 - không dùng rand() vì luồng chính cũng gọi nó.
    unsigned int x = c->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    c->rng = x ? x : 0x9E3779B9u;
    return c->rng;
}

static bool WindowLive(const AICtx *c, int w, int player)
{
    if (c->b.rule != CARO_RULE_BLOCKED) return true;
    int opp = CaroOpponent((CaroStone)player);
    bool a = c->outA[w] >= 0 && c->b.cells[c->outA[w]] == opp;
    bool b = c->outB[w] >= 0 && c->b.cells[c->outB[w]] == opp;
    return !(a && b);
}

// Đóng góp của một đoạn vào điểm thế cờ của `player`.
static int WindowEval(const AICtx *c, int w, int player)
{
    int own = c->cnt[w][player];
    int opp = c->cnt[w][CaroOpponent((CaroStone)player)];
    if (own == 0 || opp > 0 || !WindowLive(c, w, player)) return 0;
    return EVAL_TUPLE[own];
}

// ------------------------------------------------------------------ dựng ngữ cảnh

static void AddAffect(AICtx *c, int cell, int w)
{
    for (int i = 0; i < c->cellAffN[cell]; i++) {
        if (c->cellAff[cell][i] == w) return;
    }
    if (c->cellAffN[cell] < MAX_AFFECT) c->cellAff[cell][c->cellAffN[cell]++] = (short)w;
}

static void BuildContext(AICtx *c, const CaroBoard *src)
{
    memset(c, 0, sizeof(*c));
    int n = src->n;
    CaroBoardInit(&c->b, n, src->rule);
    c->cellCount = n * n;

    for (int d = 0; d < 4; d++) {
        int dx = CARO_DIR_X[d], dy = CARO_DIR_Y[d];
        for (int y = 0; y < n; y++) {
            for (int x = 0; x < n; x++) {
                int ex = x + dx * (CARO_WIN_LEN - 1), ey = y + dy * (CARO_WIN_LEN - 1);
                if (!CaroInside(&c->b, ex, ey) || c->windowCount >= MAX_WINDOWS) continue;
                int w = c->windowCount++;
                for (int k = 0; k < CARO_WIN_LEN; k++) {
                    int cell = CaroIndex(&c->b, x + dx * k, y + dy * k);
                    c->win[w][k] = (short)cell;
                    c->cellWin[cell][c->cellWinN[cell]++] = (short)w;
                    AddAffect(c, cell, w);
                }
                c->outA[w] = CaroInside(&c->b, x - dx, y - dy) ? (short)CaroIndex(&c->b, x - dx, y - dy) : -1;
                c->outB[w] = CaroInside(&c->b, ex + dx, ey + dy) ? (short)CaroIndex(&c->b, ex + dx, ey + dy) : -1;
                // Luật chặn hai đầu: quân ở ô ngoài đoạn cũng đổi giá trị đoạn.
                if (src->rule == CARO_RULE_BLOCKED) {
                    if (c->outA[w] >= 0) AddAffect(c, c->outA[w], w);
                    if (c->outB[w] >= 0) AddAffect(c, c->outB[w], w);
                }
            }
        }
    }
}

static void SetCell(AICtx *c, int cell, int player)
{
    int n = c->b.n;
    for (int i = 0; i < c->cellAffN[cell]; i++) {
        int w = c->cellAff[cell][i];
        c->evalSum[CARO_X] -= WindowEval(c, w, CARO_X);
        c->evalSum[CARO_O] -= WindowEval(c, w, CARO_O);
    }
    c->b.cells[cell] = (unsigned char)player;
    for (int i = 0; i < c->cellWinN[cell]; i++) c->cnt[c->cellWin[cell][i]][player]++;
    for (int i = 0; i < c->cellAffN[cell]; i++) {
        int w = c->cellAff[cell][i];
        c->evalSum[CARO_X] += WindowEval(c, w, CARO_X);
        c->evalSum[CARO_O] += WindowEval(c, w, CARO_O);
    }

    int cx = cell % n, cy = cell / n;
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {
            if (CaroInside(&c->b, x, y)) c->nearCount[CaroIndex(&c->b, x, y)]++;
        }
    }
}

static void ClearCell(AICtx *c, int cell)
{
    int n = c->b.n;
    int player = c->b.cells[cell];
    for (int i = 0; i < c->cellAffN[cell]; i++) {
        int w = c->cellAff[cell][i];
        c->evalSum[CARO_X] -= WindowEval(c, w, CARO_X);
        c->evalSum[CARO_O] -= WindowEval(c, w, CARO_O);
    }
    c->b.cells[cell] = CARO_EMPTY;
    for (int i = 0; i < c->cellWinN[cell]; i++) c->cnt[c->cellWin[cell][i]][player]--;
    for (int i = 0; i < c->cellAffN[cell]; i++) {
        int w = c->cellAff[cell][i];
        c->evalSum[CARO_X] += WindowEval(c, w, CARO_X);
        c->evalSum[CARO_O] += WindowEval(c, w, CARO_O);
    }

    int cx = cell % n, cy = cell / n;
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {
            if (CaroInside(&c->b, x, y)) c->nearCount[CaroIndex(&c->b, x, y)]--;
        }
    }
}

static void LoadPosition(AICtx *c, const CaroBoard *src)
{
    BuildContext(c, src);
    for (int i = 0; i < src->moveCount; i++) {
        int cell = src->moves[i];
        SetCell(c, cell, src->cells[cell]);
    }
    c->b.moveCount = src->moveCount;
    memcpy(c->b.moves, src->moves, sizeof(src->moves));
    c->b.toMove = src->toMove;
}

// Đặt thử quân `player` ở ô trống `cell` có thành hàng thắng không.
static bool MakesFive(AICtx *c, int cell, int player)
{
    // Lọc nhanh: phải có ít nhất một đoạn chứa ô này đã đủ 4 quân của player.
    bool maybe = false;
    for (int i = 0; i < c->cellWinN[cell] && !maybe; i++) {
        int w = c->cellWin[cell][i];
        if (c->cnt[w][player] == 4 && c->cnt[w][CaroOpponent((CaroStone)player)] == 0) maybe = true;
    }
    if (!maybe) return false;
    c->b.cells[cell] = (unsigned char)player;
    bool win = CaroIsWinningMove(&c->b, cell, NULL, NULL);
    c->b.cells[cell] = CARO_EMPTY;
    return win;
}

// Điểm xếp hạng khi `player` đặt vào ô trống `cell`. defenseWeight < 1 làm
// máy "quên" phòng thủ (dùng cho mức Dễ).
static int MoveScore(const AICtx *c, int cell, int player, float defenseWeight)
{
    int opp = CaroOpponent((CaroStone)player);
    float score = 0.0f;
    for (int i = 0; i < c->cellWinN[cell]; i++) {
        int w = c->cellWin[cell][i];
        int own = c->cnt[w][player], other = c->cnt[w][opp];
        if (own > 0 && other > 0) continue;
        if (own == 0 && other == 0) { score += OWN_TUPLE[0]; continue; }
        if (other == 0) {
            int v = OWN_TUPLE[own];
            score += WindowLive(c, w, player) ? v : v / 4;
        } else {
            int v = OPP_TUPLE[other];
            score += (WindowLive(c, w, opp) ? v : v / 4) * defenseWeight;
        }
    }
    // Ưu tiên nhẹ vùng trung tâm để khai cuộc tự nhiên hơn.
    int n = c->b.n, x = cell % n, y = cell / n;
    int dc = abs(x - n / 2) + abs(y - n / 2);
    return (int)score + (n - dc);
}

typedef struct {
    short cell;
    int score;
} Cand;

static int CompareCand(const void *a, const void *b)
{
    return ((const Cand *)b)->score - ((const Cand *)a)->score;
}

// Các ô trống gần quân đã có, kèm điểm xếp hạng, sắp giảm dần.
static int GenerateCandidates(AICtx *c, int player, Cand *out, float defenseWeight)
{
    int count = 0;
    if (c->b.moveCount == 0) {
        int mid = c->b.n / 2;
        out[0].cell = (short)CaroIndex(&c->b, mid, mid);
        out[0].score = 1;
        return 1;
    }
    for (int cell = 0; cell < c->cellCount; cell++) {
        if (c->b.cells[cell] != CARO_EMPTY || c->nearCount[cell] == 0) continue;
        out[count].cell = (short)cell;
        out[count].score = MoveScore(c, cell, player, defenseWeight);
        count++;
    }
    qsort(out, (size_t)count, sizeof(Cand), CompareCand);
    return count;
}

// Tìm các ô mà `player` đặt vào là thắng ngay. Trả về số ô (tối đa maxOut).
static int FindWinningCells(AICtx *c, int player, int *out, int maxOut)
{
    int count = 0;
    for (int cell = 0; cell < c->cellCount && count < maxOut; cell++) {
        if (c->b.cells[cell] != CARO_EMPTY || c->nearCount[cell] == 0) continue;
        if (MakesFive(c, cell, player)) out[count++] = cell;
    }
    return count;
}

static bool CheckTime(AICtx *c)
{
    if ((++c->nodes & 511) == 0) {
        if (s_abort || NowSeconds() > c->deadline) c->timeUp = true;
    }
    return c->timeUp;
}

// ------------------------------------------------------------------ VCF

// Tìm chuỗi "tứ liên tiếp": mỗi nước của bên tấn công đều tạo một hàng 4
// buộc đối phương phải chặn, cho tới khi có tứ kép hoặc thành 5.
static bool SearchVCF(AICtx *c, int attacker, int depth, int *firstMove)
{
    if (depth <= 0 || CheckTime(c)) return false;
    int defender = CaroOpponent((CaroStone)attacker);

    int win[2];
    if (FindWinningCells(c, attacker, win, 1) > 0) {
        if (firstMove) *firstMove = win[0];
        return true;
    }
    // Đối phương đang có nước thắng: tấn công bằng tứ không còn kịp.
    if (FindWinningCells(c, defender, win, 1) > 0) return false;

    for (int cell = 0; cell < c->cellCount; cell++) {
        if (c->b.cells[cell] != CARO_EMPTY || c->nearCount[cell] == 0) continue;

        // Chỉ xét ô tạo được ít nhất một đoạn 4 quân còn sống.
        bool makesFour = false;
        for (int i = 0; i < c->cellWinN[cell] && !makesFour; i++) {
            int w = c->cellWin[cell][i];
            if (c->cnt[w][attacker] == 3 && c->cnt[w][defender] == 0) makesFour = true;
        }
        if (!makesFour) continue;

        SetCell(c, cell, attacker);
        int threats[2];
        int threatCount = FindWinningCells(c, attacker, threats, 2);
        bool success = false;
        if (threatCount >= 2) {
            success = true;                       // Tứ kép / tứ mở: không chặn nổi
        } else if (threatCount == 1) {
            SetCell(c, threats[0], defender);     // Nước chặn bắt buộc
            if (!CaroIsWinningMove(&c->b, threats[0], NULL, NULL)) {
                success = SearchVCF(c, attacker, depth - 1, NULL);
            }
            ClearCell(c, threats[0]);
        }
        ClearCell(c, cell);

        if (success) {
            if (firstMove) *firstMove = cell;
            return true;
        }
        if (c->timeUp) return false;
    }
    return false;
}

// ------------------------------------------------------------------ alpha-beta

static int Evaluate(const AICtx *c, int player)
{
    int opp = CaroOpponent((CaroStone)player);
    // Bên tới lượt được lợi một nhịp nên thế công của họ đáng giá hơn.
    return (c->evalSum[player] * 6) / 5 - c->evalSum[opp];
}

static int Negamax(AICtx *c, int depth, int alpha, int beta, int ply, int *bestOut)
{
    int player = c->b.toMove;
    int opp = CaroOpponent((CaroStone)player);
    if (CheckTime(c)) return 0;

    int wins[2];
    if (FindWinningCells(c, player, wins, 1) > 0) {
        if (bestOut) *bestOut = wins[0];
        return WIN_SCORE - ply;
    }

    int oppWins[2];
    int oppWinCount = FindWinningCells(c, opp, oppWins, 2);
    if (oppWinCount >= 2 && !bestOut) return -(WIN_SCORE - ply - 1);

    if (depth <= 0) return Evaluate(c, player);

    static Cand candBuf[12][MAX_CANDS];
    Cand *cands = candBuf[ply < 12 ? ply : 11];
    int count;
    if (oppWinCount >= 1) {
        // Bắt buộc chặn (nếu có hai ô thì ở gốc vẫn chọn một ô để còn đi).
        cands[0].cell = (short)oppWins[0];
        cands[0].score = 0;
        count = 1;
    } else {
        count = GenerateCandidates(c, player, cands, 1.0f);
        int beam = (ply == 0) ? 14 : (ply < 3 ? 10 : 7);
        if (count > beam) count = beam;
    }
    if (count == 0) return 0;

    int best = -WIN_SCORE - 1;
    int bestCell = cands[0].cell;
    for (int i = 0; i < count; i++) {
        int cell = cands[i].cell;
        SetCell(c, cell, player);
        c->b.toMove = (CaroStone)opp;
        c->b.moveCount++;
        int v = -Negamax(c, depth - 1, -beta, -alpha, ply + 1, NULL);
        c->b.moveCount--;
        c->b.toMove = (CaroStone)player;
        ClearCell(c, cell);

        if (c->timeUp) break;
        if (v > best) {
            best = v;
            bestCell = cell;
        }
        if (v > alpha) alpha = v;
        if (alpha >= beta) break;
    }
    if (bestOut) *bestOut = bestCell;
    return best;
}

// ------------------------------------------------------------------ các mức độ

static int PickGreedy(AICtx *c, CaroDifficulty diff)
{
    int player = c->b.toMove;
    int opp = CaroOpponent((CaroStone)player);
    int wins[2];

    bool sloppy = (diff == CARO_DIFF_EASY);
    if (FindWinningCells(c, player, wins, 1) > 0 && (!sloppy || NextRand(c) % 100 < 75)) return wins[0];
    if (FindWinningCells(c, opp, wins, 1) > 0 && (!sloppy || NextRand(c) % 100 < 70)) return wins[0];

    static Cand cands[MAX_CANDS];
    int count = GenerateCandidates(c, player, cands, sloppy ? 0.55f : 1.0f);
    if (count == 0) return -1;

    if (sloppy) {
        // Chọn ngẫu nhiên trong vài nước tốt nhất, nghiêng về nước đứng đầu.
        static const int weights[5] = {38, 26, 16, 12, 8};
        int top = count < 5 ? count : 5;
        int total = 0;
        for (int i = 0; i < top; i++) total += weights[i];
        int r = (int)(NextRand(c) % (unsigned int)total);
        for (int i = 0; i < top; i++) {
            if (r < weights[i]) return cands[i].cell;
            r -= weights[i];
        }
        return cands[0].cell;
    }

    // Thường: nước tốt nhất, hoà điểm thì bốc ngẫu nhiên cho đỡ lặp lại.
    int ties = 1;
    while (ties < count && cands[ties].score >= cands[0].score - cands[0].score / 40) ties++;
    return cands[NextRand(c) % (unsigned int)ties].cell;
}

static int PickHard(AICtx *c)
{
    int player = c->b.toMove;
    int opp = CaroOpponent((CaroStone)player);
    int wins[2];
    if (FindWinningCells(c, player, wins, 1) > 0) return wins[0];
    if (FindWinningCells(c, opp, wins, 1) > 0) return wins[0];

    double start = NowSeconds();
    c->deadline = start + 0.6;
    int vcfMove = -1;
    if (SearchVCF(c, player, 12, &vcfMove) && vcfMove >= 0) return vcfMove;

    // Đào sâu dần: độ sâu chẵn để lá luôn kết thúc ở lượt của mình.
    c->timeUp = false;
    c->deadline = start + 1.6;
    int best = PickGreedy(c, CARO_DIFF_NORMAL);
    for (int depth = 2; depth <= 10; depth += 2) {
        int cell = -1;
        int score = Negamax(c, depth, -WIN_SCORE - 1, WIN_SCORE + 1, 0, &cell);
        if (c->timeUp) break;
        if (cell >= 0) best = cell;
        if (score >= WIN_SCORE - 64 || score <= -(WIN_SCORE - 64)) break;   // Đã thấy kết cục
    }
    return best;
}

// ------------------------------------------------------------------ luồng

static void *AIThreadMain(void *arg)
{
    (void)arg;
    AICtx *c = &s_ctx;
    LoadPosition(c, &s_job.board);
    c->rng = (unsigned int)(NowSeconds() * 1000003.0) | 1u;
    c->timeUp = false;
    c->nodes = 0;
    c->deadline = NowSeconds() + 5.0;

    int cell = (s_job.difficulty == CARO_DIFF_HARD) ? PickHard(c) : PickGreedy(c, s_job.difficulty);
    if (cell < 0) {
        // Phòng hờ: ô trống đầu tiên
        for (int i = 0; i < c->cellCount; i++) {
            if (s_job.board.cells[i] == CARO_EMPTY) { cell = i; break; }
        }
    }

    pthread_mutex_lock(&s_lock);
    s_result = cell;
    s_done = true;
    pthread_mutex_unlock(&s_lock);
    return NULL;
}

void CaroAIRequest(const CaroBoard *board, CaroDifficulty difficulty)
{
    if (s_running) return;
    s_job.board = *board;
    s_job.difficulty = difficulty;
    s_done = false;
    s_result = -1;
    s_abort = false;
    if (pthread_create(&s_thread, NULL, AIThreadMain, NULL) == 0) {
        s_running = true;
    } else {
        // Không tạo được luồng: tính luôn trên luồng chính.
        AIThreadMain(NULL);
    }
}

bool CaroAIPoll(int *outCell)
{
    pthread_mutex_lock(&s_lock);
    bool done = s_done;
    int result = s_result;
    if (done) s_done = false;
    pthread_mutex_unlock(&s_lock);
    if (!done) return false;

    if (s_running) {
        pthread_join(s_thread, NULL);
        s_running = false;
    }
    if (outCell) *outCell = result;
    return true;
}

bool CaroAIBusy(void)
{
    return s_running;
}

void CaroAICancel(void)
{
    if (s_running) {
        s_abort = true;
        pthread_join(s_thread, NULL);
        s_running = false;
    }
    s_done = false;
    s_abort = false;
}
