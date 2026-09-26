#pragma once

const int N = 19;
const int SIZE = N * N;
enum { EMPTY = 0, BLACK = 1, WHITE = 2, OUT = 3 };

inline int other(int color) { return 3 - color; }

// What is needed to undo a move: the stone played and the stones it captured.
struct Move {
    int cell;
    int ncap;
    int cap[16];
};

struct Board {
    int cells[SIZE];
    int near[SIZE];  // number of stones on each cell and its 8 neighbours
    int caps[3];     // stones captured BY each color
    int score;       // heuristic sum of all 5-cell windows, from BLACK's point of view
    int fours[3];    // windows holding 4 stones of a color and an empty cell
    int threats[3];  // enemy pairs each color could capture with one move

    Board();
    int  at(int x, int y) const;
    void play(int cell, int color, Move &m);  // place a stone and do the captures
    void undo(const Move &m);
    int  captures(int cell, int color) const; // pairs captured if color plays cell
    bool freeThree(int cell, int color, int dir) const;
    bool legal(int cell, int color) const;
    bool fiveAt(int cell) const;
    bool hasFive(int color) const;
    bool canBreak(int color);                 // can the opponent break color's five?
    int  winner(int color);                   // game result after color has played
    void delta(int cell, int &db, int &dw) const;
    int  eval(int color) const;

private:
    void update(int cell, int sign);
    void put(int cell, int color);
    void remove(int cell);
};
