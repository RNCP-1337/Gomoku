#include "AI.hpp"
#include <algorithm>

static const int INF = WIN + 1;

// How many candidate moves are searched, by remaining depth (index 10 = root).
static const int WIDTH[DEPTH + 1] = {0, 3, 3, 3, 3, 3, 4, 4, 5, 6, 8};

// How much `who` gains by playing cell. db/dw come from Board::delta;
// a capture also removes stones, so it is simply played and evaluated.
static int gain(Board &b, int cell, int who, int db, int dw) {
    if (!b.captures(cell, who)) return who == BLACK ? db : -dw;
    Move m;
    int before = b.eval(who);
    b.play(cell, who, m);
    int g = b.eval(who) - before;
    b.undo(m);
    return g;
}

// The best looking legal moves near the stones: what I gain by playing there
// plus what the opponent would gain by playing there (attack + defense).
int AI::genMoves(int color, int *out, int width) {
    std::pair<int, int> cand[SIZE];
    int n = 0;
    for (int c = 0; c < SIZE; c++) {
        if (b.cells[c] != EMPTY || !b.near[c]) continue;
        int db, dw;
        b.delta(c, db, dw);
        cand[n++] = {gain(b, c, color, db, dw) + gain(b, c, other(color), db, dw), c};
    }
    std::sort(cand, cand + n, [](const std::pair<int, int> &x, const std::pair<int, int> &y) {
        return x.first > y.first;
    });
    int k = 0;
    for (int i = 0; i < n && k < width; i++)
        if (b.legal(cand[i].second, color)) out[k++] = cand[i].second;
    return k;
}

// Plays cell for color, returns how good it is for color, and takes it back.
// oppFive: the opponent has a five on the board, which color must break by a capture.
int AI::tryMove(int cell, int color, int depth, int alpha, int beta, bool oppFive) {
    int win = WIN - (DEPTH - depth), v;  // faster wins (and slower losses) are better
    Move m;
    b.play(cell, color, m);
    bool five = b.fiveAt(cell);
    if (b.caps[color] >= 10) v = win;
    else if (oppFive && (!m.ncap || b.hasFive(other(color)))) v = -win;  // not broken: lost
    else if (five && !b.canBreak(color)) v = win;
    else v = -search(depth - 1, -beta, -alpha, other(color), five);
    b.undo(m);
    return v;
}

// Negamax (Min-Max where each side maximizes its own score) with alpha-beta pruning.
int AI::search(int depth, int alpha, int beta, int color, bool oppFive) {
    nodes++;
    if (depth == 0) return b.eval(color);
    int moves[32];
    int n = genMoves(color, moves, WIDTH[depth]);
    if (n == 0) return 0;
    int best = -INF;
    for (int i = 0; i < n; i++) {
        int v = tryMove(moves[i], color, depth, alpha, beta, oppFive);
        if (v > best) best = v;
        if (v > alpha) alpha = v;
        if (alpha >= beta) break;  // the opponent will never allow this line
    }
    return best;
}

int AI::bestMove(int color) {
    nodes = 0;
    nroot = genMoves(color, rootMoves, WIDTH[DEPTH]);
    if (nroot == 0) {  // empty board (or no stone around): center, or any legal cell
        int center = N / 2 * N + N / 2;
        if (b.legal(center, color)) return center;
        for (int c = 0; c < SIZE; c++)
            if (b.legal(c, color)) return c;
        return -1;
    }
    bool oppFive = b.hasFive(other(color));
    int alpha = -INF, best = rootMoves[0];
    for (int i = 0; i < nroot; i++) {
        rootScores[i] = tryMove(rootMoves[i], color, DEPTH, alpha, INF, oppFive);
        if (rootScores[i] > alpha) {
            alpha = rootScores[i];
            best = rootMoves[i];
        }
    }
    return best;
}
