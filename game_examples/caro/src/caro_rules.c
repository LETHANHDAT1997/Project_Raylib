#include "caro_rules.h"
#include <stdio.h>
#include <string.h>

const int CARO_DIR_X[4] = {1, 0, 1, 1};
const int CARO_DIR_Y[4] = {0, 1, 1, -1};

void CaroBoardInit(CaroBoard *b, int n, CaroRule rule)
{
    memset(b, 0, sizeof(*b));
    b->n = (n > CARO_MAX_N) ? CARO_MAX_N : n;
    b->rule = rule;
    b->toMove = CARO_X;
}

bool CaroPlace(CaroBoard *b, int cell)
{
    if (cell < 0 || cell >= b->n * b->n || b->cells[cell] != CARO_EMPTY) return false;
    b->cells[cell] = (unsigned char)b->toMove;
    b->moves[b->moveCount++] = (short)cell;
    b->toMove = CaroOpponent(b->toMove);
    return true;
}

void CaroUndo(CaroBoard *b)
{
    if (b->moveCount <= 0) return;
    int cell = b->moves[--b->moveCount];
    b->cells[cell] = CARO_EMPTY;
    b->toMove = CaroOpponent(b->toMove);
}

bool CaroBoardFull(const CaroBoard *b)
{
    return b->moveCount >= b->n * b->n;
}

bool CaroIsWinningMove(const CaroBoard *b, int cell, int *endA, int *endB)
{
    CaroStone s = (CaroStone)b->cells[cell];
    if (s == CARO_EMPTY) return false;
    CaroStone opp = CaroOpponent(s);
    int cx = cell % b->n, cy = cell / b->n;

    for (int d = 0; d < 4; d++) {
        int dx = CARO_DIR_X[d], dy = CARO_DIR_Y[d];

        int fwd = 0;
        while (CaroInside(b, cx + dx * (fwd + 1), cy + dy * (fwd + 1)) &&
               b->cells[CaroIndex(b, cx + dx * (fwd + 1), cy + dy * (fwd + 1))] == s) fwd++;
        int back = 0;
        while (CaroInside(b, cx - dx * (back + 1), cy - dy * (back + 1)) &&
               b->cells[CaroIndex(b, cx - dx * (back + 1), cy - dy * (back + 1))] == s) back++;

        if (1 + fwd + back < CARO_WIN_LEN) continue;

        if (b->rule == CARO_RULE_BLOCKED) {
            // Mép bàn không tính là bị chặn - chỉ quân đối phương mới chặn.
            int fx = cx + dx * (fwd + 1), fy = cy + dy * (fwd + 1);
            int bx = cx - dx * (back + 1), by = cy - dy * (back + 1);
            bool blockedF = CaroInside(b, fx, fy) && b->cells[CaroIndex(b, fx, fy)] == opp;
            bool blockedB = CaroInside(b, bx, by) && b->cells[CaroIndex(b, bx, by)] == opp;
            if (blockedF && blockedB) continue;
        }

        if (endA) *endA = CaroIndex(b, cx - dx * back, cy - dy * back);
        if (endB) *endB = CaroIndex(b, cx + dx * fwd, cy + dy * fwd);
        return true;
    }
    return false;
}

const char *CaroCellName(const CaroBoard *b, int cell)
{
    static char buf[8];
    static const char *COLS = "ABCDEFGHJKLMNOPQRST";
    int x = cell % b->n, y = cell / b->n;
    snprintf(buf, sizeof(buf), "%c%d", COLS[x], b->n - y);
    return buf;
}
