#include "Board.hpp"

// The 4 line directions, and the 8 directions used for captures.
static const int DX[4] = {1, 0, 1, 1};
static const int DY[4] = {0, 1, 1, -1};
static const int DX8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static const int DY8[8] = {0, 1, 1, 1, 0, -1, -1, -1};

// Value of a 5-cell window holding n stones of a single color.
static const int WINDOW[6] = {0, 1, 10, 100, 1000, 100000};
// Value of the number of pairs captured.
static const int CAPTURE[6] = {0, 300, 800, 2000, 6000, 100000};

static int windowValue(int nb, int nw) {
    if (nw == 0) return WINDOW[nb];
    if (nb == 0) return -WINDOW[nw];
    return 0;  // both colors: nobody can make five here
}

Board::Board() : caps{0, 0, 0}, score(0) {
    for (int i = 0; i < SIZE; i++) cells[i] = near[i] = 0;
}

int Board::at(int x, int y) const {
    if (x < 0 || y < 0 || x >= N || y >= N) return OUT;
    return cells[y * N + x];
}

// Change of `score` if BLACK (db) or WHITE (dw) puts a stone on cell,
// cell being considered empty. Only the 20 windows containing cell change.
void Board::delta(int cell, int &db, int &dw) const {
    int x = cell % N, y = cell / N;
    db = dw = 0;
    for (int d = 0; d < 4; d++) {
        int line[9], count[4] = {0, 0, 0, 0};  // stones of each kind in the window
        for (int k = -4; k <= 4; k++)
            line[k + 4] = k == 0 ? EMPTY : at(x + k * DX[d], y + k * DY[d]);
        for (int i = 0; i < 4; i++) count[line[i]]++;
        for (int s = 0; s < 5; s++) {  // slide the window line[s..s+4]
            count[line[s + 4]]++;
            if (!count[OUT]) {
                int nb = count[BLACK], nw = count[WHITE], v = windowValue(nb, nw);
                db += windowValue(nb + 1, nw) - v;
                dw += windowValue(nb, nw + 1) - v;
            }
            count[line[s]]--;
        }
    }
}

void Board::put(int cell, int color) {
    int db, dw;
    delta(cell, db, dw);
    score += color == BLACK ? db : dw;
    cells[cell] = color;
    int x = cell % N, y = cell / N;
    for (int j = y - 1; j <= y + 1; j++)
        for (int i = x - 1; i <= x + 1; i++)
            if (at(i, j) != OUT) near[j * N + i]++;
}

void Board::remove(int cell) {
    int color = cells[cell];
    cells[cell] = EMPTY;
    int db, dw;
    delta(cell, db, dw);
    score -= color == BLACK ? db : dw;
    int x = cell % N, y = cell / N;
    for (int j = y - 1; j <= y + 1; j++)
        for (int i = x - 1; i <= x + 1; i++)
            if (at(i, j) != OUT) near[j * N + i]--;
}

int Board::captures(int cell, int color) const {
    int x = cell % N, y = cell / N, opp = other(color), n = 0;
    for (int d = 0; d < 8; d++)
        if (at(x + DX8[d], y + DY8[d]) == opp && at(x + 2 * DX8[d], y + 2 * DY8[d]) == opp
            && at(x + 3 * DX8[d], y + 3 * DY8[d]) == color)
            n++;
    return n;
}

void Board::play(int cell, int color, Move &m) {
    int x = cell % N, y = cell / N, opp = other(color);
    m.cell = cell;
    m.ncap = 0;
    put(cell, color);
    for (int d = 0; d < 8; d++) {
        if (at(x + DX8[d], y + DY8[d]) == opp && at(x + 2 * DX8[d], y + 2 * DY8[d]) == opp
            && at(x + 3 * DX8[d], y + 3 * DY8[d]) == color) {
            int c1 = (y + DY8[d]) * N + x + DX8[d];
            int c2 = (y + 2 * DY8[d]) * N + x + 2 * DX8[d];
            remove(c1);
            remove(c2);
            m.cap[m.ncap++] = c1;
            m.cap[m.ncap++] = c2;
        }
    }
    caps[color] += m.ncap;
}

void Board::undo(const Move &m) {
    int color = cells[m.cell];
    caps[color] -= m.ncap;
    for (int i = 0; i < m.ncap; i++) put(m.cap[i], other(color));
    remove(m.cell);
}

// A free three is 3 stones that can become an open four (_XXXX_) in one move:
// there is a 4-cell window holding the new stone, 3 own stones, 1 empty cell,
// and both cells around that window are empty.
bool Board::freeThree(int cell, int color, int d) const {
    int x = cell % N, y = cell / N;
    int line[9];
    for (int k = -4; k <= 4; k++)
        line[k + 4] = k == 0 ? color : at(x + k * DX[d], y + k * DY[d]);
    for (int s = 1; s <= 4; s++) {
        int own = 0, empty = 0;
        for (int i = s; i < s + 4; i++) {
            if (line[i] == color) own++;
            else if (line[i] == EMPTY) empty++;
        }
        if (own == 3 && empty == 1 && line[s - 1] == EMPTY && line[s + 4] == EMPTY)
            return true;
    }
    return false;
}

// Empty, and not a double three (unless the move captures).
bool Board::legal(int cell, int color) const {
    if (cells[cell] != EMPTY) return false;
    if (captures(cell, color)) return true;
    int threes = 0;
    for (int d = 0; d < 4; d++) threes += freeThree(cell, color, d);
    return threes < 2;
}

bool Board::fiveAt(int cell) const {
    int x = cell % N, y = cell / N, color = cells[cell];
    for (int d = 0; d < 4; d++) {
        int n = 1;
        for (int k = 1; at(x + k * DX[d], y + k * DY[d]) == color; k++) n++;
        for (int k = 1; at(x - k * DX[d], y - k * DY[d]) == color; k++) n++;
        if (n >= 5) return true;
    }
    return false;
}

bool Board::hasFive(int color) const {
    for (int c = 0; c < SIZE; c++)
        if (cells[c] == color && fiveAt(c)) return true;
    return false;
}

// Endgame capture: the opponent can break every five of color by capturing a pair,
// or it already has 4 pairs and can capture a fifth one (and win by capture).
bool Board::canBreak(int color) {
    int opp = other(color);
    for (int c = 0; c < SIZE; c++) {
        if (cells[c] != EMPTY || !captures(c, opp)) continue;
        if (caps[opp] >= 8) return true;
        Move m;
        play(c, opp, m);
        bool still = hasFive(color);
        undo(m);
        if (!still) return true;
    }
    return false;
}

// Full game rules, checked after color has played. Returns the winner or EMPTY.
int Board::winner(int color) {
    int opp = other(color);
    if (caps[color] >= 10) return color;
    if (hasFive(opp)) return opp;  // color could have broken that five, and did not
    if (hasFive(color) && !canBreak(color)) return color;
    return EMPTY;
}

// Heuristic from color's point of view: alignments + captures.
int Board::eval(int color) const {
    int pb = caps[BLACK] / 2, pw = caps[WHITE] / 2;
    int s = score + CAPTURE[pb > 5 ? 5 : pb] - CAPTURE[pw > 5 ? 5 : pw];
    return color == BLACK ? s : -s;
}
