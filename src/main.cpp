#include <ncurses.h>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "AI.hpp"

enum { VS_AI_BLACK = 1, VS_AI_WHITE, HOTSEAT };
enum { P_BOARD = 1, P_WHITE, P_LAST_B, P_LAST_W, P_HINT, P_TITLE };

static const char *LETTERS = "ABCDEFGHJKLMNOPQRST";
static const char *NAMES[3] = {"", "Black", "White"};
static const int TOP = 2, LEFT = 4, PANEL = LEFT + 2 * N + 2;
static const char *STONE = "O";

struct Game {
    Board  b;
    AI     ai{b};
    int    mode = HOTSEAT;
    int    turn = BLACK;
    int    cx = N / 2, cy = N / 2;  // cursor
    int    last = -1, hint = -1;
    bool   over = false;
    double aiTime = 0, aiTotal = 0; // AI timer (seconds)
    int    aiCount = 0;
    std::string msg;

    void reset(int m) {
        b = Board();
        mode = m;
        turn = BLACK;
        cx = cy = N / 2;
        last = hint = -1;
        over = false;
        aiTime = aiTotal = 0;
        aiCount = 0;
        ai.nroot = 0;
        ai.nodes = 0;
        ai.reached = 0;
        msg = "Black starts.";
    }
    bool aiTurn() const {
        return (mode == VS_AI_BLACK && turn == WHITE) || (mode == VS_AI_WHITE && turn == BLACK);
    }
};

static std::string cellName(int cell) {
    return std::string(1, LETTERS[cell % N]) + std::to_string(N - cell / N);
}

static void drawBoard(const Game &g) {
    attrset(COLOR_PAIR(P_BOARD));
    mvprintw(TOP - 1, LEFT - 3, "   ");
    for (int x = 0; x < N; x++) printw("%c ", LETTERS[x]);
    printw(" ");
    for (int y = 0; y < N; y++) {
        attrset(COLOR_PAIR(P_BOARD));
        mvprintw(TOP + y, LEFT - 3, "%2d ", N - y);
        for (int x = 0; x < N; x++) {
            int cell = y * N + x, sy = TOP + y, sx = LEFT + 2 * x, stone = g.b.cells[cell];
            attr_t cursor = (x == g.cx && y == g.cy && !g.over) ? A_REVERSE : 0;
            if (stone) {
                int pair = cell == g.last ? (stone == BLACK ? P_LAST_B : P_LAST_W)
                                          : (stone == BLACK ? P_BOARD : P_WHITE);
                attrset(COLOR_PAIR(pair) | cursor | (stone == WHITE ? A_BOLD : 0));
                mvaddstr(sy, sx, STONE);
            } else if (cell == g.hint) {
                attrset(COLOR_PAIR(P_HINT) | A_BOLD | cursor);
                mvaddch(sy, sx, '*');
            } else {
                chtype c = ACS_PLUS;
                if (y == 0) c = x == 0 ? ACS_ULCORNER : x == N - 1 ? ACS_URCORNER : ACS_TTEE;
                else if (y == N - 1) c = x == 0 ? ACS_LLCORNER : x == N - 1 ? ACS_LRCORNER : ACS_BTEE;
                else if (x == 0) c = ACS_LTEE;
                else if (x == N - 1) c = ACS_RTEE;
                attrset(COLOR_PAIR(P_BOARD) | cursor);
                mvaddch(sy, sx, c);
            }
            attrset(COLOR_PAIR(P_BOARD));
            mvaddch(sy, sx + 1, x < N - 1 ? ACS_HLINE : ' ');
        }
        addch(' ');
    }
    attrset(A_NORMAL);
}

static void drawPanel(const Game &g) {
    int y = TOP - 1;
    attron(COLOR_PAIR(P_TITLE) | A_BOLD);
    mvprintw(y++, PANEL, "G O M O K U");
    attroff(COLOR_PAIR(P_TITLE) | A_BOLD);
    y++;
    const char *modes[4] = {"", "You (Black) vs AI", "AI vs You (White)", "Hotseat, 2 players"};
    mvprintw(y++, PANEL, "Mode     : %s", modes[g.mode]);
    if (!g.over) mvprintw(y++, PANEL, "Turn     : %s%s", NAMES[g.turn], g.aiTurn() ? " (AI)" : "");
    else y++;
    mvprintw(y++, PANEL, "Captured : Black %d/10  White %d/10", g.b.caps[BLACK], g.b.caps[WHITE]);
    y++;
    attron(A_BOLD);
    mvprintw(y++, PANEL, "AI timer : %.3f s", g.aiTime);
    attroff(A_BOLD);
    mvprintw(y++, PANEL, "Average  : %.3f s (%d searches)", g.aiCount ? g.aiTotal / g.aiCount : 0.0, g.aiCount);
    mvprintw(y++, PANEL, "Search   : depth %d, %ld nodes", g.ai.reached, g.ai.nodes);
    if (g.hint >= 0) mvprintw(y++, PANEL, "Suggested: %s for %s", cellName(g.hint).c_str(), NAMES[g.turn]);
    else y++;
    // Debug: the root candidates in the order they were searched, with their scores.
    mvprintw(y++, PANEL, "AI candidates (score):");
    for (int i = 0; i < g.ai.nroot && i < 8; i++) {
        int s = g.ai.rootScores[i];
        std::string v = s >= WIN - 100 ? "win" : s <= -WIN + 100 ? "loss" : std::to_string(s);
        mvprintw(y + i / 2, PANEL + 2 + (i % 2) * 18, "%-4s %s", cellName(g.ai.rootMoves[i]).c_str(), v.c_str());
    }
    y += 5;
    attron(A_BOLD);
    mvaddnstr(y++, PANEL, g.msg.c_str(), COLS - PANEL - 1);
    attroff(A_BOLD);
    y++;
    mvprintw(y++, PANEL, "Arrows/mouse: move  Enter: play");
    mvprintw(y++, PANEL, "r: menu   q: quit");
}

static void draw(const Game &g) {
    erase();
    if (LINES < TOP + N + 1 || COLS < PANEL + 36)
        mvprintw(0, 0, "Please enlarge the terminal (%dx%d needed). q: quit", PANEL + 36, TOP + N + 1);
    else {
        drawBoard(g);
        drawPanel(g);
    }
    refresh();
}

static int menu() {
    while (true) {
        erase();
        attron(COLOR_PAIR(P_TITLE) | A_BOLD);
        mvprintw(1, 4, "G O M O K U");
        attroff(COLOR_PAIR(P_TITLE) | A_BOLD);
        mvprintw(3, 4, "1  Play against the AI (you are Black and start)");
        mvprintw(4, 4, "2  Play against the AI (you are White)");
        mvprintw(5, 4, "3  Two players on this computer, with move suggestions");
        mvprintw(6, 4, "q  Quit");
        refresh();
        int ch = getch();
        if (ch >= '1' && ch <= '3') return ch - '0';
        if (ch == 'q' || ch == 'Q') return 0;
    }
}

// Runs the AI for the player to move and updates the timer.
static int think(Game &g) {
    auto start = std::chrono::steady_clock::now();
    int cell = g.ai.bestMove(g.turn);
    g.aiTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    g.aiTotal += g.aiTime;
    g.aiCount++;
    return cell;
}

static void play(Game &g, int cell) {
    Move m;
    g.b.play(cell, g.turn, m);
    g.last = cell;
    g.hint = -1;
    g.msg = NAMES[g.turn] + std::string(" played ") + cellName(cell) + ".";
    if (m.ncap) g.msg += " +" + std::to_string(m.ncap) + " captured!";
    int w = g.b.winner(g.turn);
    if (w) {
        g.over = true;
        g.msg = NAMES[w] + std::string(g.b.caps[w] >= 10 ? " wins by capture!" : " wins by alignment!");
        return;
    }
    if (g.b.hasFive(g.turn)) g.msg = "Five, but breakable by a capture!";
    g.turn = other(g.turn);
    for (int c = 0; c < SIZE; c++)
        if (g.b.legal(c, g.turn)) return;
    g.over = true;
    g.msg = "Draw: no legal move left.";
}

static void tryPlay(Game &g) {
    int cell = g.cy * N + g.cx;
    if (g.b.cells[cell] != EMPTY) g.msg = "This intersection is taken.";
    else if (!g.b.legal(cell, g.turn)) g.msg = "Forbidden move: double free-three.";
    else play(g, cell);
}

static void run() {
    Game g;
    int mode = menu();
    if (!mode) return;
    g.reset(mode);
    while (true) {
        if (!g.over && g.aiTurn()) {
            g.msg = "AI is thinking...";
            draw(g);
            int cell = think(g);
            if (cell >= 0) play(g, cell);
            flushinp();  // ignore keys pressed while the AI was thinking
            continue;
        }
        if (!g.over && g.mode == HOTSEAT && g.hint < 0) {
            std::string msg = g.msg;
            g.msg = "Computing a suggestion...";
            draw(g);
            g.hint = think(g);
            g.msg = msg;
            flushinp();
        }
        draw(g);
        int ch = getch();
        MEVENT ev;
        if (ch == 'q' || ch == 'Q') return;
        if (ch == 'r' || ch == 'R') {
            if (!(mode = menu())) return;
            g.reset(mode);
            continue;
        }
        if (g.over) continue;
        if (ch == KEY_UP && g.cy > 0) g.cy--;
        else if (ch == KEY_DOWN && g.cy < N - 1) g.cy++;
        else if (ch == KEY_LEFT && g.cx > 0) g.cx--;
        else if (ch == KEY_RIGHT && g.cx < N - 1) g.cx++;
        else if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) tryPlay(g);
        else if (ch == KEY_MOUSE && getmouse(&ev) == OK && (ev.bstate & BUTTON1_PRESSED)) {
            int x = (ev.x - LEFT + 1) / 2, y = ev.y - TOP;
            if (ev.x >= LEFT - 1 && x >= 0 && x < N && y >= 0 && y < N) {
                g.cx = x;
                g.cy = y;
                tryPlay(g);
            }
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    if (MB_CUR_MAX > 1) STONE = "●";  // a round stone when the terminal speaks UTF-8
    if (!initscr()) return 1;
    try {
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        mousemask(BUTTON1_PRESSED, NULL);
        mouseinterval(0);
        if (has_colors()) {
            start_color();
            init_pair(P_BOARD, COLOR_BLACK, COLOR_YELLOW);
            init_pair(P_WHITE, COLOR_WHITE, COLOR_YELLOW);
            init_pair(P_LAST_B, COLOR_BLACK, COLOR_RED);
            init_pair(P_LAST_W, COLOR_WHITE, COLOR_RED);
            init_pair(P_HINT, COLOR_BLUE, COLOR_YELLOW);
            init_pair(P_TITLE, COLOR_YELLOW, COLOR_BLACK);
        }
        run();
    } catch (...) {
        endwin();
        fprintf(stderr, "Gomoku: unexpected error\n");
        return 1;
    }
    endwin();
    return 0;
}
