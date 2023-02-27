// Copyright 2019 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef OPEN_SPIEL_GAMES_HIVE_HIVE_BOARD_H_
#define OPEN_SPIEL_GAMES_HIVE_HIVE_BOARD_H_

#include <array>
#include <cstdint>
#include <cmath>
#include <ostream>
#include <vector>
#include <unordered_set>
#include <set>

#include "open_spiel/games/chess/chess_common.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/abseil-cpp/absl/types/optional.h"

namespace open_spiel {
namespace hive {

// The maximum length the tiles can span is 28, so the hive can never wrap
// around and complete the connection
inline constexpr int kBoardSize = 29;
// there can be a stack of bugs up to 7 high
inline constexpr int kBoardHeight = 7;
inline constexpr int kNumHexagons = kBoardSize*kBoardSize*kBoardHeight;

inline constexpr Player kWhite = 0;
inline constexpr Player kBlack = 1;
static const std::array<std::string, 2> kPlayerChars = { "w", "b" };

// 1 Bee
// 2 Beetles
// 3 Ants
// 3 Grasshoppers
// 2 Spiders
// 1 Ladybugs
// 1 Mosquito
// 1 Pillbug
// 14 bugs per side
inline constexpr int kNumBugTypes = 8;
// Each bug can be one of two colors and can be placed first, second, or third
static const int kNumBugs = 2*3*kNumBugTypes;
// Num bugs of each type
inline constexpr std::array<int8_t, 8> bug_counts = { 1, 2, 3, 3, 2, 1, 1, 1 };
// bug_series[i] is the number of bugs with type < i
inline constexpr std::array<int8_t, 8> bug_series = { 0, 1, 3, 6, 9, 11, 12, 13 };

static const chess_common::ZobristTable<uint64_t, 2, kNumBugs, kBoardSize, kBoardSize, kBoardHeight> zobristTable(/*seed=*/2346);

static const long mod(long a, long b) { return (a%b+b)%b; }

enum BugType : int8_t {
  kBee = 0,
  kBeetle = 1,
  kAnt = 2,
  kGrasshopper = 3,
  kSpider = 4,
  kLadybug = 5,
  kMosquito = 6,
  kPillbug = 7,
};

static const std::array<std::string, kNumBugTypes> kBugTypeChars = {
  "Q", "B", "A", "G", "S", "L", "M", "P"
};

// In case all the bug types are represented in the same plane, these values are
// used to represent each piece type.
static inline constexpr std::array<float, kNumBugTypes> kBugTypeRepresentation = {
    {1, 0.875, 0.75, 0.625, 0.5, 0.375, 0.25, 0.125}};

struct Bug {
  Player player;
  BugType type;
  int8_t order;

  bool operator==(const Bug& other) const {
    return type == other.type && player == other.player && order == other.order;
  }

  bool operator!=(const Bug& other) const { return !(*this == other); }

  std::string ToUnicode() const;
  std::string ToString() const;
};

inline constexpr Bug kEmptyBug = Bug{kWhite, kBee, -1};

// Tries to parse piece type from char ('K', 'Q', 'R', 'B', 'N', 'P').
absl::optional<Bug> PieceTypeFromString(std::string s);

// Converts Bug to string
std::string BugToString(Bug b);

inline int8_t addBoardCoords(int8_t a, int8_t b) {
  return mod(a + b, kBoardSize);
}

class Offset {
 public:
  Offset() : x(0), y(0), z(0) {}
  Offset(int boardIdx);
  Offset(int x_, int y_, int z_) :
    x(mod(x_, kBoardSize)), y(mod(y_, kBoardSize)), z(mod(z_, kBoardHeight)) {}

  int8_t x;
  int8_t y;
  int8_t z;

  int index = x + kBoardSize*y + kBoardSize*kBoardSize*z;
  std::array<Offset, 6> neighbours() const;

  Offset operator+(const Offset& other) const;
  bool operator==(const Offset& other) const;
  bool operator!=(const Offset& other) const;
};

class Hexagon;

class HiveMove {
 public:
  HiveMove() : pass(true) {}
  HiveMove(BugType bt, Hexagon* t)
          : pass(false), place(true), bug_type(bt), to(t) {}
  HiveMove(Hexagon* f, Hexagon* t)
          : pass(false), place(false), from(f), to(t) {}

  bool pass;
  bool place;
  BugType bug_type;
  Hexagon* from;
  Hexagon* to;

  bool operator==(const HiveMove& other) const {
    if (pass != other.pass) { return false; }
    if (pass) { return true; }
    if (place != other.place) { return false; }
    if (place) {
      return bug_type == other.bug_type && to == other.to;
    }
    return from == other.from && to == other.to;
  }
};

// x corresponds to file (column / letter)
// y corresponds to rank (row / number).
// Hexagons are placed with corners up and down; flat to the sides.
// This forces some rows to be shifted left and some right to align with the
// euclidian plane.
// Whether it is shifted left or right relative to neighbour rows will change
// the indices of it's neighbors.
// We assume the first column is shifted up, so a column is shifted up iff
// x % 2 == 0.
class Hexagon {
 public:
  Hexagon();

  Offset loc;

  Bug bug;

  Hexagon* above;
  Hexagon* below;
  std::array<Hexagon*, 6> neighbours;
  bool last_moved;

  Hexagon* parent;
  bool visited;
  int num;
  int low;

  Hexagon* Bottom() const;
  Hexagon* Top() const;

  bool IsSurrounded() const;
  void GenerateMoves(BugType t, std::vector<HiveMove> &moves) const;

 private:
  int FindClockwiseMove(int prev_idx) const;
  int FindCounterClockwiseMove(int prev_idx) const;
  Hexagon* WalkThree(int i, bool clockwise) const;
  std::vector<Hexagon*> FindJumpMoves() const;

  void GenerateBeeMoves(std::vector<HiveMove> &moves) const;
  void GenerateBeetleMoves(std::vector<HiveMove> &moves) const;
  void GenerateAntMoves(std::vector<HiveMove> &moves) const;
  void GenerateGrasshopperMoves(std::vector<HiveMove> &moves) const;
  void GenerateSpiderMoves(std::vector<HiveMove> &moves) const;
  void GenerateLadybugMoves(std::vector<HiveMove> &moves) const;
  void GenerateMosquitoMoves(std::vector<HiveMove> &moves) const;
  void GeneratePillbugMoves(std::vector<HiveMove> &moves) const;
};

std::string BugToString(absl::optional<Bug> b);

std::ostream &operator<<(std::ostream &os, Bug b);
std::ostream &operator<<(std::ostream &os, absl::optional<Bug> b);

class BugCollection {
 public:
  explicit BugCollection(Player p);

  void Reset();

  void ReturnBug(Hexagon* h);

  bool HasBug(BugType t) const;
  bool UseBug(Hexagon* h, BugType t);
  absl::optional<Offset> GetBug(Bug b) const;
  int8_t NumBugs(BugType bt) const;

 private:
  Player player_;
  std::array<int8_t, 8> bug_counts_;
  std::array<std::vector<Offset>, 8> hexagons_;
};

// Simple Hive board.
class HiveBoard {
 public:
  explicit HiveBoard();

  void Clear();
  void InitBoard();

  Hexagon* GetHexagon(Offset o);
  Hexagon* GetHexagon(int x, int y, int z);
  Hexagon* GetHexagon(Bug b) const;
  std::size_t NumBugs() const;
  std::size_t NumBugs(Player p, BugType bt) const;

  std::vector<HiveMove> LegalMoves() const;
  void PlayMove(HiveMove& m);
  void UndoMove(HiveMove& m);

  Hexagon* starting_hexagon;

  Player to_play;

  // TODO: make these functions and have hive.cc cache them
  Player outcome;
  bool is_terminal;

  std::array<int8_t, kBoardSize*kBoardSize*kBoardHeight> observation;

  // TODO: figure out how to normalize for transations and rotations
  uint64_t zobrist_hash;

 private:
  Bug RemoveBug(Hexagon* h);
  void PlaceBug(Hexagon* h, Bug b);

  Player HexagonOwner(Hexagon* h) const;
  void CacheHexagonOwner(Hexagon* h);

  void CacheUnpinnedHexagons();

  void CacheOutcome();
  void CacheIsTerminal();

  std::array<BugCollection, 2> bug_collections_;

  std::array<std::unordered_set<Hexagon*>, 2> available_;
  std::unordered_set<Hexagon*> unpinned_;

  std::unordered_set<Hexagon*> hexagons_;
  std::array<Hexagon*, 2> bees_;
  Hexagon* last_moved_;

  std::array<Hexagon, kBoardSize*kBoardSize*kBoardHeight> board_;
};

inline std::ostream& operator<<(std::ostream& stream, const Bug& pt) {
  return stream << BugToString(pt);
}

std::string BugToString(Bug b);
std::string OffsetToString(Offset o);
std::string HexagonToString(Hexagon* h);

}  // namespace go
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_HIVE_HIVE_BOARD_H_
