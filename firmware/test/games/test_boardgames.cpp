// Sticky OS's own logic around CrossPlay's Four in a Row and Chess
// (firmware/sticky-os/boardgames.h): the board's levels, the end of a chess
// game, the captured pieces, and the scripted games the on-device test plays
// (a win, a loss and a draw each), which only stay true while the levels do.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "boardgames.h"

static int fails = 0, checks = 0;
#define CHECK(c)                                                    \
  do {                                                              \
    checks++;                                                       \
    if (!(c)) fails++, printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
  } while (0)

// ---- Four in a Row

static void testFourColumnMatchesTheBrain() {
  using namespace connectfour;
  srand(11);
  for (int game = 0; game < 60; game++) {
    Game g;
    start(g);
    while (!over(g)) {
      CHECK(boards::fourColumn(g, kDepth) == chooseColumn(g));
      int c;
      do c = rand() % kColumns;
      while (!canDrop(g, c));
      drop(g, c);
    }
    CHECK(boards::fourColumn(g, 3) == kNoColumn);
  }
}

// You first (Light), the board replying at `level`; returns the outcome.
static connectfour::Outcome fourScript(int level, const char *cols) {
  using namespace connectfour;
  Game g;
  start(g);
  for (const char *c = cols; *c && !over(g); c++) {
    CHECK(canDrop(g, *c - '0'));
    drop(g, *c - '0');
    if (!over(g)) drop(g, boards::fourColumn(g, boards::kFourDepth[level]));
  }
  return g.outcome;
}

static void testFourScripts() {
  using connectfour::Outcome;
  CHECK(fourScript(0, "3325544") == Outcome::LightWins);            // a win on easy
  CHECK(fourScript(2, "00000") == Outcome::DarkWins);              // a loss on hard: always the first column
  CHECK(fourScript(0, "333245504402530066111") == Outcome::Draw);  // a draw on easy
}

// ---- Chess

struct Session {
  chess::Position p;
  uint64_t keys[600];
  int n = 0;
  boards::ChessEnd end = boards::END_NONE;
};
static chess::SearchBuffers buffers;
static chess::MoveList legal;

static void sessionStart(Session &s, const char *fen = nullptr) {
  if (fen) CHECK(chess::parseFen(fen, s.p));
  else chess::setStartPosition(s.p);
  s.n = 0;
  s.keys[s.n++] = chess::positionKey(s.p);
  s.end = boards::chessEnd(s.p, legal, s.keys, s.n);
}
static void sessionApply(Session &s, const chess::Move &m) {
  chess::Undo u;
  chess::makeMove(s.p, m, u);
  s.keys[s.n++] = chess::positionKey(s.p);
  s.end = boards::chessEnd(s.p, legal, s.keys, s.n);
}
static bool sessionMove(Session &s, const char *uci) {
  for (int i = 0; i < legal.count; i++) {
    char b[8];
    chess::moveToString(legal.moves[i], b);
    if (!strcmp(b, uci)) return sessionApply(s, legal.moves[i]), true;
  }
  printf("  %s isn't legal here\n", uci);
  return false;
}
// What the tab does: your move, then (unless it's over) the board's at `level`.
static boards::ChessEnd chessScript(int level, std::initializer_list<const char *> moves, bool *whiteToMove) {
  Session s;
  sessionStart(s);
  for (const char *uci : moves) {
    if (s.end) break;
    if (!sessionMove(s, uci)) return boards::END_NONE;
    if (s.end) break;
    chess::Position copy = s.p;
    chess::SearchResult r = chess::search(copy, boards::kChessDepth[level], buffers, boards::kChessBudget[level]);
    CHECK(r.hasMove);
    sessionApply(s, r.best);
  }
  *whiteToMove = s.p.whiteToMove;
  return s.end;
}

static void testChessEnds() {
  Session s;
  sessionStart(s);
  CHECK(s.end == boards::END_NONE && legal.count == 20);
  sessionStart(s, "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");  // Black has no move, isn't in check
  CHECK(s.end == boards::END_STALEMATE);
  sessionStart(s, "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
  CHECK(s.end == boards::END_MATE);
  sessionStart(s, "8/8/4k3/8/8/2K5/8/8 w - - 0 1");
  CHECK(s.end == boards::END_MATERIAL);
  sessionStart(s, "8/8/4k3/8/8/2KN4/8/8 w - - 0 1");
  CHECK(s.end == boards::END_MATERIAL);
  sessionStart(s, "8/8/4k3/8/8/2KR4/8/8 w - - 0 1");
  CHECK(s.end == boards::END_NONE);
  sessionStart(s, "8/8/4k3/8/8/2KR4/8/8 w - - 100 80");
  CHECK(s.end == boards::END_FIFTY);
  // Knights out and back twice: the start position a third time.
  sessionStart(s);
  for (const char *m : {"g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1"}) CHECK(sessionMove(s, m) && s.end == boards::END_NONE);
  CHECK(sessionMove(s, "f6g8") && s.end == boards::END_REPETITION);
}

static void testCaptured() {
  chess::Position p;
  uint8_t lost[7];
  chess::setStartPosition(p);
  boards::chessCaptured(p, true, lost);
  for (int t = 0; t < 7; t++) CHECK(lost[t] == 0);
  CHECK(chess::parseFen("rnb1kbnr/pppp1ppp/8/4p3/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3", p));  // Black's queen gone, White's e-pawn
  boards::chessCaptured(p, false, lost);
  CHECK(lost[chess::Queen] == 1 && lost[chess::Pawn] == 0);
  boards::chessCaptured(p, true, lost);
  CHECK(lost[chess::Pawn] == 1 && lost[chess::Queen] == 0);
  CHECK(chess::parseFen("4k3/8/8/8/8/8/8/QQ2K3 w - - 0 40", p));  // a promoted queen: its pawn neither lost nor its queen extra
  boards::chessCaptured(p, true, lost);
  CHECK(lost[chess::Queen] == 0 && lost[chess::Pawn] == 7 && lost[chess::Rook] == 2);
}

static void testChessScripts() {
  bool whiteToMove = false;
  // The scholar's mate, for you on easy (White mates: Black to move, mated).
  CHECK(chessScript(0, {"e2e4", "f1c4", "d1f3", "f3f7"}, &whiteToMove) == boards::END_MATE && !whiteToMove);
  // A fool's mate for the board on hard, against a careless White.
  CHECK(chessScript(2, {"f2f3", "g2g4", "h2h3"}, &whiteToMove) == boards::END_MATE && whiteToMove);
  // A stalemate on easy: the board takes everything and leaves White's king no move.
  CHECK(chessScript(0, {"a2a4", "g2g4", "d2d4", "f1g2", "c1g5", "b2b4", "g5c1", "e1d2", "a4a5", "d2e1", "g1h3", "g2c6", "b1a3", "h1g1",
                        "a3b5", "a5a6"},
                    &whiteToMove) == boards::END_STALEMATE);
}

static void testLevels() {
  for (int l = 0; l < 3; l++) {
    CHECK(boards::kChessDepth[l] >= 1 && boards::kChessDepth[l] <= chess::kMaxSearchDepth);
    CHECK(boards::kChessBudget[l] > 0);
    CHECK(boards::kFourDepth[l] >= 1);
    if (l) CHECK(boards::kChessDepth[l] >= boards::kChessDepth[l - 1] && boards::kFourDepth[l] > boards::kFourDepth[l - 1]);
  }
  CHECK(boards::clampLevel(-1) == 0 && boards::clampLevel(7) == 2);
  CHECK(boards::moveId(12, 28) == 1796);  // e2e4
  char n[3];
  boards::squareName(12, n);
  CHECK(!strcmp(n, "e2"));
}

int main() {
  testLevels();
  testFourColumnMatchesTheBrain();
  testFourScripts();
  testChessEnds();
  testCaptured();
  testChessScripts();
  printf("boardgames: %d checks, %d failed\n", checks, fails);
  return fails ? 1 : 0;
}
