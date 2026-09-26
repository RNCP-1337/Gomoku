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

**Candidate moves** (`genMoves`): only empty cells next to a stone. Each one is scored quickly with
*what I gain by playing here + what the opponent would gain by playing here* (attack + defense). Moves are sorted by this
score, which makes alpha-beta efficient. Only the best `WIDTH[depth]` legal moves are searched: 8 at the root, down to
3 near the leaves. This limit is what allows 10 levels in well under half a second (about 0.1 s on average).

**Game end inside the tree** (`tryMove`): capturing 10 stones, or a five that cannot be broken, is a win.
A five the opponent left on the board, that was not broken by a capture, is a loss. Wins get `WIN - distance`, so the
AI prefers fast wins and slow losses.

### Heuristic (`Board::delta`, `Board::eval`)

Every set of 5 consecutive cells on the board (horizontal, vertical, both diagonals) is a *window*. A window that
contains stones of only one color is a place where that color can still make five. It is worth, by number of stones:

| stones in the window | 0 | 1 | 2  | 3   | 4    | 5 (five) |
|----------------------|---|---|----|-----|------|----------|
| value                | 0 | 1 | 10 | 100 | 1000 | 100000   |

A window with both colors is worth 0, because nobody can make five there. The board score is the sum of all windows,
positive for Black and negative for White. With windows, open and blocked shapes get different values for free.
An open three `__XXX__` is inside 3 windows of 3 stones. A three blocked on one side is inside only 1. Split shapes like
`X_XX` are counted too.

Captured pairs are added: 0, 300, 800, 2000, 6000 for 0 to 4 pairs.

The score is **incremental**. Placing or removing a stone only changes the 20 windows that contain that cell (5 per
direction), so `delta` recomputes only those 20 windows. The heuristic of a leaf is then just a read, and the same
`delta` gives the attack + defense score used to sort the candidate moves.
