// Four in a Row and Chess (games_four.ino, games_chess.ino): the parts that
// are plain logic, kept out of the tabs so the host can test them
// (firmware/test/games/test_boardgames.cpp). The rules themselves are
// CrossPlay's (src/crossplay), used as they are: this only wraps them.
#pragma once

#include <cstdint>
#include <cstring>

#include "src/crossplay/connectfour/ConnectFourCore.h"
#include "src/crossplay/connectfour/ConnectFourBrain.h"
#include "src/crossplay/chess/ChessCore.h"
#include "src/crossplay/chess/ChessEngine.h"

namespace boards {

// ======================================================================== how hard the board plays
//
// The one place to tune. Levels 0 easy, 1 medium, 2 hard (GameProgress's fourLevel, chessLevel).
//
// Four in a Row: how many plies ConnectFourBrain looks ahead. CrossPlay plays 7
// (connectfour::kDepth). Host, Apple M3: the worst reply at 7 is ~29 ms (the
// empty board); the ESP32-S3 at 240 MHz is guessed 15-30x slower, so ~0.9 s
// worst. 2 sees your next move (blocks a three, misses a double threat).
constexpr int kFourDepth[3] = {2, 4, connectfour::kDepth};

// Chess: depth in plies (chess::kMaxSearchDepth caps it at 4) and a node
// budget, the search's safety net: it stops there and plays the best move
// found so far. CrossPlay plays {1, 3, 4} with 60000 nodes. Host, Apple M3,
// 700 positions from random games: depth 3 never passed 6100 nodes (~2 ms),
// depth 4 reached 167000 nodes unbudgeted, and its slowest search was ~50 ms
// for ~49000 nodes (~1 us a node). At 15-30x slower, 40000 nodes is about
// 0.6-1.3 s on the board.
constexpr int kChessDepth[3] = {1, 3, 4};
// Measured on the board (ESP32-S3, 240 MHz): about 50-75 us a node at depth 4, so
// 18000 nodes keep the board's worst move to about 1.3 s. A move the budget cuts
// short is still legal, the best found so far.
constexpr uint32_t kChessBudget[3] = {18000, 18000, 18000};

inline int clampLevel(int level) { return level < 0 ? 0 : level > 2 ? 2 : level; }

// ======================================================================== Four in a Row

// ConnectFourBrain's chooseColumn() at any depth (it has kDepth built in): the
// same search, the same tie-breaks (the centre, then the lower column), so at
// kDepth it picks what chooseColumn() picks.
inline int fourColumn(const connectfour::Game &game, int depth) {
  using namespace connectfour;
  if (over(game)) return kNoColumn;
  if (depth < 1) depth = 1;
  const uint8_t side = game.turn;
  int bestColumn = kNoColumn, bestScore = 0;
  for (const int column : kSearchOrder) {
    if (!canDrop(game, column)) continue;
    Game next = game;
    drop(next, column);
    const int score = -negamax(next, other(side), depth - 1, -kWinScore * 2, kWinScore * 2);
    const bool better = bestColumn == kNoColumn || score > bestScore ||
                        (score == bestScore && kLinesThrough[column] > kLinesThrough[bestColumn]) ||
                        (score == bestScore && kLinesThrough[column] == kLinesThrough[bestColumn] && column < bestColumn);
    if (better) bestColumn = column, bestScore = score;
  }
  return bestColumn;
}

// ======================================================================== Chess

enum ChessEnd : uint8_t { END_NONE, END_MATE, END_STALEMATE, END_REPETITION, END_FIFTY, END_MATERIAL, END_RESIGNED };

// Neither side can ever mate: the kings alone, or a king and one bishop or knight against a bare king.
inline bool insufficientMaterial(const chess::Position &p) {
  int minors = 0;
  for (int sq = 0; sq < 64; sq++) {
    uint8_t t = chess::pieceType(p.squares[sq]);
    if (t == chess::Pawn || t == chess::Rook || t == chess::Queen) return false;
    if (t == chess::Knight || t == chess::Bishop) minors++;
  }
  return minors <= 1;
}

// Whether the game is over in `p`, the side to move's position. `legal` gets the
// side to move's legal moves (a scratch list, ~1 KB: the caller's, not the stack's);
// `keys` are the positions so far, the last being `p`'s (chess::positionKey).
inline ChessEnd chessEnd(const chess::Position &p, chess::MoveList &legal, const uint64_t *keys, int nKeys) {
  legal.clear();
  chess::generateLegalMoves(p, legal);
  if (legal.count == 0) return chess::isInCheck(p) ? END_MATE : END_STALEMATE;  // mate first: mate on move 50 is mate
  if (keys && chess::repetitionCount(keys, nKeys, p.halfmoveClock) >= 3) return END_REPETITION;
  if (p.halfmoveClock >= 100) return END_FIFTY;  // 100 plies: fifty moves each
  if (insufficientMaterial(p)) return END_MATERIAL;
  return END_NONE;
}

// How many of each piece type (chess::Pawn .. Queen) one side has lost: the
// starting set less what's on the board. A promoted pawn is neither: not lost, its piece not extra.
inline void chessCaptured(const chess::Position &p, bool white, uint8_t lost[7]) {
  static const uint8_t START[7] = {0, 8, 2, 2, 2, 1, 1};
  int have[7] = {};
  for (int sq = 0; sq < 64; sq++)
    if (chess::isColour(p.squares[sq], white)) have[chess::pieceType(p.squares[sq])]++;
  // Promotions: a piece beyond the starting count was a pawn.
  int promoted = 0;
  for (int t = chess::Knight; t <= chess::Queen; t++)
    if (have[t] > START[t]) promoted += have[t] - START[t];
  for (int t = 0; t < 7; t++) {
    int n = START[t] - have[t] - (t == chess::Pawn ? promoted : 0);
    lost[t] = n > 0 ? n : 0;
  }
  lost[chess::King] = 0;
}

// The id `game pick` takes for a whole move: 1000 + from * 64 + to (a promotion becomes a queen).
constexpr int kMoveIdBase = 1000;
constexpr int moveId(int from, int to) { return kMoveIdBase + from * 64 + to; }

// "e2" for square 12.
inline void squareName(int sq, char out[3]) {
  out[0] = (char)('a' + (sq & 7)), out[1] = (char)('1' + (sq >> 3)), out[2] = 0;
}

}  // namespace boards
