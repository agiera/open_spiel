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
#include <ostream>
#include <vector>
#include <unordered_set>

#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace hive {

// Constants.
inline constexpr int kNumPlayers = 2;

// The maximum length the tiles can span is 28, so the hive can never wrap
// around and complete the connection
inline constexpr int kBoardSize = 29;
// there can be a stack of bugs up to 7 high
inline constexpr int kBoardHeight = 7;
inline constexpr int kNumHexagons = kBoardSize*kBoardSize*kBoardHeight;

const chess_common::ZobristTable<uint64_t, 2*kNumBugs, kBoardSize, kBoardSize, kBoardHeight> zobristTable(/*seed=*/2346);

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
inline constexpr int kNumCellStates = 2*kNumBugTypes + 1;
inline constexpr int kNumBugs = 14;
inline constexpr std::array<int8_t, 8> bug_counts = {1, 2, 3, 3, 2, 1, 1, 1};
// bug_series[i] is the number of bugs with type < i
inline constexpr std::array<int8_t, 8> bug_series = {0, 1, 3, 6, 9, 11, 12, 13};

enum Color : int8_t { kBlack = 0, kWhite = 1 };

inline Color OppColor(Color color) {
  return color == Color::kWhite ? Color::kBlack : Color::kWhite;
}

std::string ColorToString(Color c);

inline std::ostream& operator<<(std::ostream& stream, Color c) {
  return stream << ColorToString(c);
}

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

// Tries to parse piece type from char
// ("B", "T", "A", "G", "S", "L", "M", "P").
// Case-insensitive.
absl::optional<BugType> BugTypeFromChar(char c);

// Converts bug type to one character strings -
// "B", "T", "A", "G", "S", "L", "M", "P".
// b must be one of the enumerator values of PieceType.
std::string BugTypeToString(BugType b, bool uppercase = true);

struct Bug {
  bool operator==(const Bug& other) const {
    return type == other.type && color == other.color;
  }

  bool operator!=(const Bug& other) const { return !(*this == other); }

  std::string ToUnicode() const;
  std::string ToString() const;

  Color color;
  BugType type;
};

inline int8_t addBoardCoords(int8_t a, int8_t b) {
  return (a + b) % kBoardSize;
}

struct Offset {
  int8_t x;
  int8_t y;
  int8_t z;

  Offset operator+(const Offset& other) const {
    return {addBoardCoords(x, other.x), addBoardCoords(y, other.y), addBoardCoords(z, other.z)};
  }

  bool operator==(const Offset& other) const {
    return x == other.x && y == other.y && z == other.z;
  }
  bool operator!=(const Offset& other) const {
    return !operator==(other);
  }
};
std::array<Offset, 6> neighbours(Offset o);

// x corresponds to file (column / letter)
// y corresponds to rank (row / number).
// Hexagons are placed with corners left and right.
// This forces some rows to be shifted up and some down when tiling a surface.
// Whether it is shifted up or down relative to neighbore columns will change
// the indices of it's neighbors.
// We assume the first column is shifted up, so a column is shifted up iff
// x % 2 == 0.
class Hexagon {
 public:
  explicit Hexagon();

  Offset loc;
  Bug bug;

  Hexagon* above;
  Hexagon* below;
  std::array<Hexagon*, 6> neighbours;

  int boardIndex;
  bool visited;
};

std::string BugToString(absl::optional<Bug> b);

std::ostream &operator<<(std::ostream &os, Bug b);
std::ostream &operator<<(std::ostream &os, absl::optional<Bug> b);

std::string HiveActionToString(Action action);

struct HiveMove {
  bool pass;
  bool place;
  BugType bug;
  Offset from;
  Offset to;

  bool operator==(const HiveMove& other) const {
    if (pass != other.pass) { return false; }
    if (pass) { return true; }
    if (place != other.place) { return false; }
    if (!place && from != other.from) { return false; }
    return to == other.to;
  }
};

class BugCollection {
 public:
  explicit BugCollection(Color c);

  void ReturnBug(Hexagon* h);

  bool HasBug(BugType t) const;
  Hexagon* UseBug(BugType t);

 private:
  std::array<Hexagon, kNumBugs> hexagons_;
  std::array<int8_t, 8> bug_counts_;
};

// Simple Hive board.
class HiveBoard {
 public:
  explicit HiveBoard(int board_size);

  void Clear();

  void GenerateLegalMoves() const;

  bool IsLegalMove(HiveMove& m) const;

  void PlayMove(HiveMove& m);

  std::string ToString();

  std::array<int8_t, kBoardSize*kBoardSize*kBoardHeight> observation_;

 private:
  int OffsetToIndex(Offset o) const;
  Hexagon* GetHexagon(Offset o) const;
  absl::optional<Color> OffsetOwner(Offset o) const;
  void CacheOffsetOwner(Offset o);
  Hexagon* TakeHexagon(Offset o);
  void PlaceHexagon(Offset o, Hexagon* h);

  int8_t board_size_;
  int8_t board_height_;

  uint64_t zobrist_hash_;

  Color to_play_;
  std::array<BugCollection, 2> bug_collections_;

  std::array<std::unordered_set<int>, 2> available_;
  std::vector<Hexagon*> hexagons_;
  std::array<Hexagon*, kBoardSize*kBoardSize*kBoardHeight> board_;
  std::array<bool, kBoardSize*kBoardSize> visited_;

  std::vector<HiveMove> legalMoves_;
};

std::ostream &operator<<(std::ostream &os, const HiveBoard &board);

HiveBoard BoardFromFEN(std::string fen);

}  // namespace go
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_HIVE_HIVE_BOARD_H_