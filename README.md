# Gomoku

A Gomoku game (19x19, Ninuki-renju rules) with an AI based on Min-Max, played in the terminal (ncurses).

## Build and run

```sh
make        # needs a C++17 compiler and ncurses (installed by default on Linux and macOS)
./Gomoku
```

The Makefile rules are `all`, `Gomoku`, `clean`, `fclean`, `re`. Use a terminal of at least 80x22.

**Modes** (chosen in the start menu):
1. You (Black) against the AI
2. The AI (Black) against you
3. Two players on the same computer. The AI suggests a move (`*`) for the player to move.

**Controls:** arrow keys or mouse to move, `Enter`/`space` to place a stone, `r` for the menu, `q` to quit.

The panel shows the **AI timer** (time of the last search, and the average), the search depth, the number of positions
searched, and the root candidates with their scores (debug view of the AI's reasoning).
The last move is shown in red.

## Rules

- Players take turns. Black starts. Five or more stones in a row wins.
- **Capture:** flanking exactly two enemy stones (`X O O X`) removes them. Capturing 10 stones wins.
  Moving into a flanked position (`X O _ X`, O plays the gap) is safe.
- **Endgame capture:** a five only wins if the opponent cannot break it by capturing a pair from it.
  It also does not win if the opponent has already captured 8 stones and can capture a pair anywhere.
  If the opponent does not break the five on the next move, the five wins.
- **No double-three:** a move that creates two free-threes at once is forbidden, unless the move captures.
  A free-three is three stones that can become an open four (`_XXXX_`) with one more move, for example `_XXX_` or `_X_XX_`.

Code: `src/Board.cpp` (`play`, `captures`, `freeThree`, `legal`, `canBreak`, `winner`).

## The AI (`src/AI.cpp`)

### Min-Max / Negamax with alpha-beta

The AI builds a tree of possible games: its moves, then the opponent's replies, then its replies, and so on, **10 moves
deep** (`DEPTH`). The positions at the bottom of the tree get a score from the heuristic. In Min-Max, the AI picks the
move with the maximum score, and assumes the opponent always picks the move with the minimum score (the best for them).

The code uses **Negamax**, the same algorithm written once. A score for one player is the negative of the score for the
other player, so each side maximizes `-score(child)`.

**Alpha-beta pruning** skips branches that cannot change the result. `alpha` is the score the player to move is sure to
get, and `beta` is the score the opponent is sure to get. If a move scores `>= beta`, the opponent would never allow this
position. We stop looking at the other moves there (a "cutoff"). This gives the same result as plain Min-Max, much
faster, when the best moves are tried first.

**Search space** (`genMoves`): only the empty cells next to a stone, i.e. the union of the 3x3 squares around every
stone, not the whole board or one big rectangle (`Board::near` counts the stones around each cell). Each one is scored
quickly with *what I gain by playing here + what the opponent would gain by playing here* (attack + defense). Moves are
sorted by this score, which makes alpha-beta efficient. Only the best `WIDTH[depth]` legal moves are searched: 8 at the root, down to
3 near the leaves. This limit is what allows 10 levels in well under half a second (about 0.1 s on average).
The width is limited, never the depth: every line that does not end the game is searched down to level 10. The panel
shows the depth the last search really reached.

**Game end inside the tree** (`tryMove`): capturing 10 stones, or a five that cannot be broken, is a win.
A five the opponent left on the board, that was not broken by a capture, is a loss. Wins get `WIN - distance`, so the
AI prefers fast wins and slow losses.

### Heuristic (`Board::update`, `Board::eval`)

`eval(color)` gives the score of a position for `color`, the player to move. It adds these parts.

**1. Alignments with room for five.** Every set of 5 consecutive cells on the board (horizontal, vertical, both
diagonals) is a *window*. A window with stones of only one color is a place where that color can still make five.
It is worth, by number of stones:

| stones in the window | 0 | 1 | 2  | 3   | 4    | 5 (five) |
|----------------------|---|---|----|-----|------|----------|
| value                | 0 | 1 | 10 | 100 | 1000 | 100000   |

A window with both colors is worth 0, because nobody can make five there. This one rule covers several things.
- **Current alignments:** more stones in a window means more points.
- **Room to make five:** an alignment boxed in (by enemy stones or the edge) with less than 5 cells of room is in no
  window, so it is worth 0.
- **Freedom:** a free three `__XXX__` is in 3 windows of 3 stones (300). Half-free, `OXXX__`, it is in 1 window
  (100). Flanked, `OXXXO`, it is in none (0).
- **Split shapes:** shapes like `X_XX` are counted too.

**2. Captured stones.** Pairs already captured are worth 0, 300, 800, 2000, 6000 (0 to 4 pairs).

**3. Potential captures.** A pattern `X O O _` (X can capture the pair with one move) is worth, for X, a third of
what that capture would add. It grows as a player gets close to 10 captures.

**4. Figures.** A player who has a four (a window with 4 stones and an empty cell) and is to move completes five:
big bonus. A player facing two fours can block only one: big penalty.

**Both players:** every part is computed for both colors and subtracted (Black's score = -White's score).

The score is **incremental**. Placing or removing a stone only changes the 20 windows (5 per direction) and 16 capture
patterns (4 per direction) that contain that cell, so `update` recomputes only those. The heuristic of a leaf is then
just a read. `delta` uses the same windows to score candidate moves (attack + defense) for move ordering.
