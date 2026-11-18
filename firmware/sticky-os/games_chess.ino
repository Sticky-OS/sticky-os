// ---------------------------------------------------------------- Chess
//
// Chess: the rules are CrossPlay's (chess/ChessCore, ChessEngine, ChessPieces, MIT; src/crossplay/README.md),
// this tab draws them and takes the taps, like the other games (gHeader,
// gButton, gTarget; games.ino). Its state lives in PSRAM (chessS, made by chessEnsure()).
//
// You play White against the board. Tap a piece: its moves show as dots, its
// captures as corners; tap one to move. A pawn on the last row asks what to
// become. The board's move: a partial refresh shows yours and "Thinking...",
// then it searches (boards::kChessDepth / kChessBudget in boardgames.h, the
// one place to tune: about a second at most on the S3) and a second partial
// refresh shows its reply; a new game and the end are full refreshes. The
// loop does nothing else meanwhile: the panel shows the same picture, and a
// tap made while it thinks is read afterwards.
//
// Squares: light ones plain, dark ones hatched with thin diagonal lines, the
// way printed chess diagrams do it. A white piece is only an outline, so each
// piece first clears its own silhouette, a pixel wider (chessS->mask, made
// from the bitmaps once): both colours sit on a clean white cut-out and read
// on either square. The last move's two squares are framed, a king in check
// has a thick frame, the picked piece's square is black.
//
// Targets, in ring order (UP / DOWN), and the ids `game pick <id>` takes:
//   100 + square  a square, 0 = a1 .. 63 = h8 (e2 = 112, e4 = 128): your pieces that can move;
//                 once one is picked, it (again: put it back) and where it can go
//   30..33        promotion: queen, rook, bishop, knight (while it asks)
//   10..12        the level: easy, medium, hard
//   20            Undo (your last move and the board's reply)
//   21            Resign (tap twice) while a game is on; else 901 New game (ID_NEW)
//   900           Back (ID_BACK)
//   1000 + from * 64 + to   (pick only, not drawn) play that move at once; a promotion becomes a queen.
//                 e2e4 = 1000 + 12 * 64 + 28 = 1796.
// OK with no ring: rings your first piece that can move (with one picked: its first move);
// while promoting: a queen; after the end: New game.
// `game peek`: "chess <FEN> | <white|black> to move | <playing|thinking|promoting|checkmate, you win|
//   checkmate, board wins|stalemate|draw by repetition|draw by fifty moves|draw by material|resigned>
//   | you <uci|-> | board <uci|-> (<nodes> nodes, <ms> ms) | level <n> | picked <square|->"

#include "boardgames.h"

static const int CHESS_HIST = 512;  // plies kept for Undo and repetition; past it the oldest half goes

struct ChessState {
  bool made = false;
  chess::Position pos;
  boards::ChessEnd end = boards::END_NONE;
  bool thinking = false, promoting = false, resignArmed = false;
  int8_t sel = -1;                   // the picked square
  uint8_t promoFrom = 0, promoTo = 0;
  int plies = 0;                     // moves in hist; keys holds plies + 1 positions
  chess::Move hist[CHESS_HIST];
  chess::Undo undo[CHESS_HIST];
  uint64_t keys[CHESS_HIST + 1];
  char boardSan[10] = "";            // the board's last move, as it's written ("Nf6")
  uint32_t thinkMs = 0, thinkNodes = 0;
  chess::MoveList legal;             // the side to move's legal moves
  chess::SearchBuffers buf;          // the search's lists, ~5 KB
  uint8_t mask[7][chessart::kPieceSize * chessart::kPieceSize / 8];  // each piece's silhouette, a pixel wider
  uint8_t fill[chessart::kPieceSize * chessart::kPieceSize];        // scratch for making them
};
static ChessState *chessS = nullptr;

enum : int16_t { CH_LEVEL0 = 10, CH_UNDO = 20, CH_RESIGN = 21, CH_PROMO0 = 30, CH_SQ0 = 100 };

static const uint8_t *chessArt(int type, bool solid, bool small) {
  using namespace chessart;
  switch (type) {
    case chess::Pawn: return solid ? (small ? kPawnSolidSmall : kPawnSolid) : (small ? kPawnLightSmall : kPawnLight);
    case chess::Knight: return solid ? (small ? kKnightSolidSmall : kKnightSolid) : (small ? kKnightLightSmall : kKnightLight);
    case chess::Bishop: return solid ? (small ? kBishopSolidSmall : kBishopSolid) : (small ? kBishopLightSmall : kBishopLight);
    case chess::Rook: return solid ? (small ? kRookSolidSmall : kRookSolid) : (small ? kRookLightSmall : kRookLight);
    case chess::Queen: return solid ? (small ? kQueenSolidSmall : kQueenSolid) : (small ? kQueenLightSmall : kQueenLight);
    default: return solid ? (small ? kKingSolidSmall : kKingSolid) : (small ? kKingLightSmall : kKingLight);
  }
}

// Each piece's mask: everything not reachable from the edge of its 48 x 48 box
// without crossing ink (of either colour's drawing), grown by a pixel.
static void chessMakeMasks() {
  const int N = chessart::kPieceSize, ROW = N / 8;
  uint8_t *f = chessS->fill;
  for (int t = chess::Pawn; t <= chess::King; t++) {
    const uint8_t *a = chessArt(t, false, false), *b = chessArt(t, true, false);
    for (int i = 0; i < N * N; i++) {
      int byte = (i / N) * ROW + (i % N) / 8, bit = 0x80 >> (i % 8);
      f[i] = (a[byte] | b[byte]) & bit ? 1 : 0;  // 1 ink, 0 not yet known, 2 outside
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
          uint8_t &v = f[y * N + x];
          if (v) continue;
          if (x == 0 || y == 0 || x == N - 1 || y == N - 1 || f[y * N + x - 1] == 2 || f[y * N + x + 1] == 2 || f[(y - 1) * N + x] == 2 ||
              f[(y + 1) * N + x] == 2)
            v = 2, changed = true;
        }
    }
    memset(chessS->mask[t], 0, sizeof chessS->mask[t]);
    for (int y = 0; y < N; y++)
      for (int x = 0; x < N; x++) {
        bool in = false;
        for (int dy = -1; dy <= 1 && !in; dy++)
          for (int dx = -1; dx <= 1 && !in; dx++) {
            int xx = x + dx, yy = y + dy;
            in = xx >= 0 && yy >= 0 && xx < N && yy < N && f[yy * N + xx] != 2;
          }
        if (in) chessS->mask[t][y * ROW + x / 8] |= 0x80 >> (x % 8);
      }
  }
}

// Made on first use. No function takes or returns ChessState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void chessEnsure() {
  if (chessS) return;
  void *p = heap_caps_malloc(sizeof(ChessState), MALLOC_CAP_SPIRAM);
  chessS = p ? new (p) ChessState() : new ChessState();
  chessMakeMasks();
}

static bool chessOver() { return chessS->end != boards::END_NONE; }
static bool chessYourTurn() { return !chessOver() && !chessS->thinking && chessS->pos.whiteToMove; }

// After every move (and an undo): the side to move's moves, and whether it's over.
static void chessSettle() {
  chessS->end = boards::chessEnd(chessS->pos, chessS->legal, chessS->keys, chessS->plies + 1);
}

static void chessApply(const chess::Move &m) {
  if (chessS->plies == CHESS_HIST) {  // full: the oldest half goes (an even number, so White still moved at the even plies)
    const int H2 = CHESS_HIST / 2;
    memmove(chessS->hist, chessS->hist + H2, H2 * sizeof(chess::Move));
    memmove(chessS->undo, chessS->undo + H2, H2 * sizeof(chess::Undo));
    memmove(chessS->keys, chessS->keys + H2, (H2 + 1) * sizeof(uint64_t));
    chessS->plies = H2;
  }
  if (!chessS->pos.whiteToMove) chess::moveToSan(chessS->pos, m, chessS->boardSan);
  int i = chessS->plies++;
  chessS->hist[i] = m;
  chess::makeMove(chessS->pos, m, chessS->undo[i]);
  chessS->keys[i + 1] = chess::positionKey(chessS->pos);
  chessSettle();
}

static void chessNew() {
  chessS->made = true;
  chess::setStartPosition(chessS->pos);
  chessS->plies = 0;
  chessS->keys[0] = chess::positionKey(chessS->pos);
  chessS->thinking = chessS->promoting = chessS->resignArmed = false;
  chessS->sel = -1;
  chessS->boardSan[0] = 0;
  chessS->thinkMs = chessS->thinkNodes = 0;
  if (demoMode) {  // screenshots: an Italian game, White's knight picked with its moves showing
    for (const char *uci : {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5", "b1c3", "g8f6"}) {
      chessSettle();
      for (int i = 0; i < chessS->legal.count; i++) {
        char s[6];
        chess::moveToString(chessS->legal.moves[i], s);
        if (!strcmp(s, uci)) {
          chessApply(chessS->legal.moves[i]);
          break;
        }
      }
    }
    chessS->sel = 21;  // f3
  }
  chessSettle();
}

static void chessFinish() {
  GameProgress &p = gameProgress();
  bool won = chessS->end == boards::END_MATE && !chessS->pos.whiteToMove;
  bool lost = chessS->end == boards::END_RESIGNED || (chessS->end == boards::END_MATE && chessS->pos.whiteToMove);
  if (won) p.chessWins++, beep(2400, 120);
  else if (lost) p.chessLosses++, beep(800, 200);
  else p.chessDraws++;
  gamesChanged();
  chessS->sel = -1, chessS->promoting = chessS->resignArmed = false;
  gs->gFocusId = -1;
}

// The board's reply: searched on a copy (the search makes and unmakes moves on what it's given).
static void chessReply() {
  int lv = boards::clampLevel(gameProgress().chessLevel);
  chess::Position copy = chessS->pos;
  uint32_t t = millis();
  chess::SearchResult r = chess::search(copy, boards::kChessDepth[lv], chessS->buf, boards::kChessBudget[lv]);
  chessS->thinkMs = millis() - t, chessS->thinkNodes = r.nodes;
  Serial.printf("chess: the board thought %lu ms, %lu nodes at depth %d%s\n", (unsigned long)chessS->thinkMs, (unsigned long)r.nodes,
                boards::kChessDepth[lv], r.budgetExhausted ? " (budget reached)" : "");
  if (r.hasMove) chessApply(r.best);
}

// Your move, then the board's: two partial refreshes (yours and "Thinking...", then its reply), a full one at the end.
static void chessPlay(const chess::Move &m) {
  chessS->sel = -1, chessS->promoting = chessS->resignArmed = false;
  chessApply(m);
  if (chessOver()) return chessFinish(), render(true);
  chessS->thinking = true;
  render(false);
  chessReply();
  chessS->thinking = false;
  if (chessOver()) return chessFinish(), render(true);
  beep(2600, 15);
  render(false);
}

static int chessFindMove(int from, int to, int promo) {
  for (int i = 0; i < chessS->legal.count; i++) {
    const chess::Move &m = chessS->legal.moves[i];
    if (m.from == from && m.to == to && (!(m.flags & chess::FlagPromotion) || m.promotion == promo)) return i;
  }
  return -1;
}
static bool chessCanMove(int sq) {
  for (int i = 0; i < chessS->legal.count; i++)
    if (chessS->legal.moves[i].from == sq) return true;
  return false;
}
static bool chessCanUndo() { return chessYourTurn() && !chessS->promoting && chessS->plies >= 2; }

static void chessTapSquare(int sq) {
  if (!chessYourTurn()) return;
  if (chessS->promoting) {
    if (sq == chessS->promoFrom) chessS->promoting = false, chessS->sel = -1, render(false);  // put back
    return;
  }
  int i = chessS->sel >= 0 ? chessFindMove(chessS->sel, sq, chess::Queen) : -1;
  if (i >= 0) {
    if (chessS->legal.moves[i].flags & chess::FlagPromotion) {  // what it becomes, first
      chessS->promoting = true, chessS->promoFrom = chessS->sel, chessS->promoTo = sq;
      gs->gFocusId = -1;
      return render(false);
    }
    return chessPlay(chessS->legal.moves[i]);
  }
  chessS->sel = sq != chessS->sel && chessCanMove(sq) ? sq : -1;
  chessS->resignArmed = false;
  render(false);
}

// ---- drawing

static void chessGeometry(int &x0, int &y0, int &s) {
  if (TALL) s = 56, x0 = (W - 8 * s) / 2, y0 = BODY_Y + 96;
  else s = 52, x0 = 16, y0 = BODY_Y + (BODY_H - 8 * s) / 2;
}

static void chessPiece(int x, int y, uint8_t piece) {
  int t = chess::pieceType(piece);
  canvas->drawBitmap(x, y, chessS->mask[t], chessart::kPieceSize, chessart::kPieceSize, PAPER);
  canvas->drawBitmap(x, y, chessArt(t, chess::isBlack(piece), false), chessart::kPieceSize, chessart::kPieceSize, INK);
}

static void chessFrame(int x, int y, int s, int t) {
  for (int k = 0; k < t; k++) canvas->drawRect(x + k, y + k, s - 2 * k, s - 2 * k, INK);
}

// The pieces one side has lost, small, from x along; returns the x after them.
static int chessCapturedRow(int x, int y, int maxX, bool white) {
  uint8_t lost[7];
  boards::chessCaptured(chessS->pos, white, lost);
  for (int t : {chess::Queen, chess::Rook, chess::Bishop, chess::Knight, chess::Pawn})
    for (int k = 0; k < lost[t]; k++) {
      const chessart::SmallInk &ink = chessart::kSmallInk[t];
      if (x + ink.width > maxX) return x;
      canvas->drawBitmap(x - ink.left, y, chessArt(t, !white, true), chessart::kSmallPieceSize, chessart::kSmallPieceSize, INK);
      x += ink.width + 3;
    }
  return x;
}

static String chessSay() {
  using namespace boards;
  if (chessS->promoting) return T(S_CHESS_PROMOTE);
  if (chessS->thinking) return T(S_CHESS_THINKING);
  switch (chessS->end) {
    case END_MATE: return T(chessS->pos.whiteToMove ? S_CHESS_MATE_LOSE : S_CHESS_MATE_WIN);
    case END_STALEMATE: return T(S_CHESS_STALEMATE);
    case END_REPETITION: return T(S_CHESS_REPETITION);
    case END_FIFTY: return T(S_CHESS_FIFTY);
    case END_MATERIAL: return T(S_CHESS_MATERIAL);
    case END_RESIGNED: return T(S_CHESS_RESIGNED);
    default: break;
  }
  if (chessS->resignArmed) return T(S_CHESS_RESIGN_SURE);
  if (chessS->sel >= 0) return T(S_CHESS_WHERE);
  bool check = chess::isInCheck(chessS->pos);
  if (chessS->boardSan[0]) return TF(S_CHESS_BOARD_PLAYED, chessS->boardSan) + " " + T(check ? S_CHESS_CHECK : S_CHESS_YOUR_MOVE);
  return T(check ? S_CHESS_CHECK : S_CHESS_START);
}

static void drawChess() {
  chessEnsure();
  if (!chessS->made) chessNew();
  GameProgress &p = gameProgress();
  int x0, y0, s;
  chessGeometry(x0, y0, s);
  const chess::Position &pos = chessS->pos;
  bool yours = chessYourTurn() && !chessS->promoting;
  int lastFrom = -1, lastTo = -1, king = -1;
  if (chessS->plies) lastFrom = chessS->hist[chessS->plies - 1].from, lastTo = chessS->hist[chessS->plies - 1].to;
  if (chess::isInCheck(pos))
    for (int sq = 0; sq < 64; sq++)
      if (pos.squares[sq] == (chess::King | (pos.whiteToMove ? 0 : chess::BlackFlag))) king = sq;

  // The squares: White at the bottom.
  for (int sq = 0; sq < 64; sq++) {
    int x = x0 + (sq & 7) * s, y = y0 + (7 - (sq >> 3)) * s;
    bool dark = ((sq & 7) + (sq >> 3)) % 2 == 0;  // a1 is dark
    if (sq == chessS->sel) canvas->fillRect(x, y, s, s, INK);
    else if (dark)
      for (int yy = 0; yy < s; yy++)
        for (int xx = (4 - (x + y + yy) % 4) % 4; xx < s; xx += 4) canvas->drawPixel(x + xx, y + yy, INK);
    if (pos.squares[sq] != chess::Empty) chessPiece(x + (s - chessart::kPieceSize) / 2, y + (s - chessart::kPieceSize) / 2, pos.squares[sq]);
    if (sq == lastFrom || sq == lastTo) chessFrame(x, y, s, 3);  // after the piece: its white cut-out mustn't break the frame
    if (sq == king) chessFrame(x, y, s, 5);
  }
  chessFrame(x0 - 2, y0 - 2, 8 * s + 4, 2);

  // What can be tapped on the board, in ring order: the picked piece, where it can go, then your other pieces that can move.
  auto squareTarget = [&](int sq) { gTarget(x0 + (sq & 7) * s + 6, y0 + (7 - (sq >> 3)) * s + 6, s - 12, s - 12, CH_SQ0 + sq); };
  if (yours && chessS->sel >= 0) {
    squareTarget(chessS->sel);
    for (int i = 0; i < chessS->legal.count; i++) {
      const chess::Move &m = chessS->legal.moves[i];
      if (m.from != chessS->sel || ((m.flags & chess::FlagPromotion) && m.promotion != chess::Queen)) continue;
      int x = x0 + (m.to & 7) * s, y = y0 + (7 - (m.to >> 3)) * s, c = s / 2, k = s / 4;
      if (pos.squares[m.to] != chess::Empty || (m.flags & chess::FlagEnPassant)) {  // a capture: the square's corners
        canvas->fillTriangle(x, y, x + k, y, x, y + k, INK);
        canvas->fillTriangle(x + s - 1, y, x + s - 1 - k, y, x + s - 1, y + k, INK);
        canvas->fillTriangle(x, y + s - 1, x + k, y + s - 1, x, y + s - 1 - k, INK);
        canvas->fillTriangle(x + s - 1, y + s - 1, x + s - 1 - k, y + s - 1, x + s - 1, y + s - 1 - k, INK);
      } else {
        canvas->fillCircle(x + c, y + c, 11, PAPER);
        canvas->fillCircle(x + c, y + c, 8, INK);
      }
      squareTarget(m.to);
    }
  }
  if (yours)
    for (int sq = 0; sq < 64; sq++)
      if (sq != chessS->sel && chessCanMove(sq)) squareTarget(sq);
  if (chessS->promoting) {  // the four it can become, in a box over the board
    int bw = 4 * 64 + 24, bh = 64 + 64, bx = x0 + (8 * s - bw) / 2, by = y0 + (8 * s - bh) / 2;
    canvas->fillRoundRect(bx - 4, by - 4, bw + 8, bh + 8, 14, PAPER);
    for (int k = 0; k < 3; k++) canvas->drawRoundRect(bx + k, by + k, bw - 2 * k, bh - 2 * k, 12, INK);
    textCentre(bx + bw / 2, by + 34, &FreeSansBold12pt7b, T(S_CHESS_PROMOTE));
    static const uint8_t TYPES[4] = {chess::Queen, chess::Rook, chess::Bishop, chess::Knight};
    for (int i = 0; i < 4; i++) {
      int cx = bx + 12 + i * 64, cy = by + 52;
      chessPiece(cx + 8, cy + 4, TYPES[i]);
      gTarget(cx + 4, cy, 56, 56, CH_PROMO0 + i);
    }
    int sq = chessS->promoFrom;
    gTarget(x0 + (sq & 7) * s + 6, y0 + (7 - (sq >> 3)) * s + 6, s - 12, s - 12, CH_SQ0 + sq);  // put it back
  }

  // The rest: what's happening, the pieces taken, the level, the tally, Undo and Resign (or New game), Back.
  String say = chessSay();
  int px, pw, py;
  if (TALL) {
    px = x0, pw = 8 * s;
    int tx = 16 + 104 + 16, tw = W - 16 - tx;
    auto lines = wrapped(&FreeSansBold12pt7b, say, tw);
    int ly = lines.size() > 1 ? BODY_Y + 29 : BODY_Y + 42;
    for (int i = 0; i < (int)lines.size() && i < 2; i++, ly += 26) textCentre(tx + tw / 2, ly, &FreeSansBold12pt7b, lines[i].c_str());
    chessCapturedRow(x0, y0 - 30, x0 + pw, true);  // the board's catch (your pieces) over the board, yours under it
    chessCapturedRow(x0, y0 + 8 * s + 6, x0 + pw, false);
    py = y0 + 8 * s + 42;
  } else {
    px = x0 + 8 * s + 24, pw = W - 16 - px;
    gCentred(px + pw / 2, BODY_Y + 96, &FreeSansBold12pt7b, say, pw, 28, 3);
    chessCapturedRow(px, BODY_Y + 172, px + pw, true);
    chessCapturedRow(px, BODY_Y + 200, px + pw, false);
    py = BODY_Y + 236;
  }
  static const StrId LEVELS[3] = {S_CHESS_EASY, S_CHESS_MEDIUM, S_CHESS_HARD};
  int lw = (pw - 20) / 3;
  for (int l = 0; l < 3; l++) {
    int lx = px + l * (lw + 10);
    pillButton(lx, py, lw, 48, fitted(&FreeSansBold9pt7b, T(LEVELS[l]), lw - 12).c_str(), l == p.chessLevel, &FreeSansBold9pt7b);
    gTarget(lx, py, lw, 48, CH_LEVEL0 + l);
  }
  String tally = TF(S_CHESS_TALLY, p.chessWins, p.chessDraws, p.chessLosses);
  const GFXfont *tf = textW(&FreeSans12pt7b, tally.c_str()) <= pw ? &FreeSans12pt7b : &FreeSans9pt7b;
  textCentre(px + pw / 2, py + 78, tf, fitted(tf, tally, pw).c_str());
  int by = H - 76, uw = (pw - 12) * 2 / 5;
  if (chessCanUndo()) gButton(px, by, uw, 56, T(S_CHESS_UNDO), false, CH_UNDO);
  else pillButton(px, by, uw, 56, "", false, &FreeSansBold12pt7b), textCentre(px + uw / 2, by + 36, &FreeSans12pt7b, T(S_CHESS_UNDO));  // not now: outline, light type
  if (!chessOver() && chessS->plies > 0) gButton(px + uw + 12, by, pw - uw - 12, 56, T(S_CHESS_RESIGN), chessS->resignArmed, CH_RESIGN);
  else gButton(px + uw + 12, by, pw - uw - 12, 56, T(S_G_NEW_GAME), chessOver(), ID_NEW);
  if (TALL) gButton(16, BODY_Y + 10, 104, 48, T(S_G_BACK), false, ID_BACK);
  else gButton(px, BODY_Y + 10, 116, 48, T(S_G_BACK), false, ID_BACK);
}

// ---- the hooks

static void actChess(int id) {
  chessEnsure();
  if (!chessS->made) chessNew();
  if (id != CH_RESIGN) chessS->resignArmed = false;
  if (id == ID_BACK) return gs->gGame = -1, gs->gFocusId = -1, render(false);
  if (id == ID_NEW) return chessNew(), gs->gFocusId = -1, render(true);
  if (id >= CH_LEVEL0 && id < CH_LEVEL0 + 3) {
    gameProgress().chessLevel = id - CH_LEVEL0;
    gamesChanged();
    return render(false);  // the board's next move plays at it
  }
  if (id == CH_UNDO) {
    if (!chessCanUndo()) return;
    for (int k = 0; k < 2; k++) chessS->plies--, chess::unmakeMove(chessS->pos, chessS->hist[chessS->plies], chessS->undo[chessS->plies]);
    chessS->sel = -1, chessS->boardSan[0] = 0;
    chessSettle();
    return render(false);
  }
  if (id == CH_RESIGN) {
    if (chessOver() || chessS->thinking) return;
    if (!chessS->resignArmed) return chessS->resignArmed = true, render(false);
    chessS->end = boards::END_RESIGNED;
    chessFinish();
    return render(true);
  }
  if (id >= CH_PROMO0 && id < CH_PROMO0 + 4 && chessS->promoting) {
    static const uint8_t TYPES[4] = {chess::Queen, chess::Rook, chess::Bishop, chess::Knight};
    int i = chessFindMove(chessS->promoFrom, chessS->promoTo, TYPES[id - CH_PROMO0]);
    chessS->promoting = false;
    if (i >= 0) return chessPlay(chessS->legal.moves[i]);
    return render(false);
  }
  if (id >= CH_SQ0 && id < CH_SQ0 + 64) return chessTapSquare(id - CH_SQ0);
  if (id >= boards::kMoveIdBase && id < boards::kMoveIdBase + 64 * 64 && chessYourTurn()) {  // `game pick`: a whole move
    int from = (id - boards::kMoveIdBase) / 64, to = (id - boards::kMoveIdBase) % 64, i = chessFindMove(from, to, chess::Queen);
    if (i >= 0) chessPlay(chessS->legal.moves[i]);
  }
}

static void chessOpen() {
  chessEnsure();
  if (!chessS->made || chessOver() || demoMode) chessNew();  // else the game where it was left
  chessS->promoting = chessS->resignArmed = false;
}

// A knight.
static void chessIcon(int cx, int cy, int s, int t) {
  int n = s >= chessart::kPieceSize ? chessart::kPieceSize : chessart::kSmallPieceSize;
  canvas->drawBitmap(cx - n / 2, cy - n / 2, chessArt(chess::Knight, true, n == chessart::kSmallPieceSize), n, n, INK);
}

static String chessLine() {
  GameProgress &p = gameProgress();
  if (chessS && chessS->made && !chessOver() && chessS->plies > 0) return TF(S_CHESS_L_MOVE, (int)chessS->pos.fullmoveNumber);
  if (p.chessWins + p.chessDraws + p.chessLosses) return TF(S_CHESS_L_TALLY, p.chessWins, p.chessDraws);
  return T(S_CHESS_L_HINT);
}

static void chessPeek(String &s) {
  using namespace boards;
  chessEnsure();
  if (!chessS->made) chessNew();
  char fen[96], u[8];
  chess::positionToFen(chessS->pos, fen);
  s += String("chess ") + fen + " | " + (chessS->pos.whiteToMove ? "white" : "black") + " to move | ";
  static const char *const ENDS[] = {"playing", "checkmate, ", "stalemate", "draw by repetition", "draw by fifty moves", "draw by material", "resigned"};
  if (chessS->end == END_NONE) s += chessS->thinking ? "thinking" : chessS->promoting ? "promoting" : "playing";
  else s += ENDS[chessS->end];
  if (chessS->end == END_MATE) s += chessS->pos.whiteToMove ? "board wins" : "you win";
  // Your last move and the board's: White's are the even plies.
  int last = chessS->plies - 1, you = last % 2 ? last - 1 : last, board = last % 2 ? last : last - 1;
  s += " | you ";
  if (you >= 0) chess::moveToString(chessS->hist[you], u), s += u;
  else s += "-";
  s += " | board ";
  if (board >= 0) chess::moveToString(chessS->hist[board], u), s += u;
  else s += "-";
  s += " (" + String(chessS->thinkNodes) + " nodes, " + String(chessS->thinkMs) + " ms) | level " + String(gameProgress().chessLevel) + " | picked ";
  if (chessS->sel >= 0) squareName(chessS->sel, u), s += u;
  else s += "-";
}

// OK with nothing ringed: your first piece that can move, or the picked one's first move; a queen when promoting.
static bool chessOk() {
  chessEnsure();
  if (chessS->promoting) return actChess(CH_PROMO0), true;
  if (!chessYourTurn()) return false;  // the end: New game
  for (const GTarget &t : gs->gHits)
    if (t.id >= CH_SQ0 && t.id < CH_SQ0 + 64 && t.id != CH_SQ0 + chessS->sel) {
      gs->gFocusId = t.id;
      render(false);
      return true;
    }
  return false;
}
