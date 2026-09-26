#pragma once
#include "Board.hpp"

const int DEPTH = 10;
const int WIN = 1000000000;

struct AI {
    Board &b;
    long nodes;               // positions visited during the last search
    int  reached;             // deepest level actually reached by the last search
    int  rootMoves[32];       // root candidates in search order, for the debug panel
    int  rootScores[32];
    int  nroot;

    AI(Board &board) : b(board), nodes(0), reached(0), nroot(0) {}
    int bestMove(int color);  // -1 if there is no legal move

private:
    int genMoves(int color, int *out, int width);
    int search(int depth, int alpha, int beta, int color, bool oppFive);
    int tryMove(int cell, int color, int depth, int alpha, int beta, bool oppFive);
};
