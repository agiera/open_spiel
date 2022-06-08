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

#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace hive {

// Constants.
inline constexpr int kNumPlayers = 2;

// The maximum length the tiles can span is 28, so the hive can never wrap
// around and complete the connection
inline constexpr int kBoardSize = 29;
// there can be a stack of bugs up to 7 high
inline constexpr int kBoardHeight = 8;
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

class Offset {
 public:
  Offset() : x(0), y(0), z(0) {}
  Offset(int boardIdx);
  Offset(int8_t x_, int8_t y_, int8_t z_) :
    x(x_ % kBoardSize), y(y_ % kBoardSize), z(z_ % kBoardSize) {}

  int8_t x;
  int8_t y;
  int8_t z;

  int index = x + kBoardSize*y + kBoardSize*kBoardSize*z;
  std::array<Offset, 6> neighbours() const;

  Offset operator+(const Offset& other) const;
  bool operator==(const Offset& other) const;
  bool operator!=(const Offset& other) const;
};

// x corresponds to file (column / letter)
// y corresponds to rank (row / number).
// Hexagons are placed with corners left and right.
// This forces some rows to be shifted up and some down when tiling a surface.
// Whether it is shifted up or down relative to neighbore columns will change
// the indices of it's neighbors.
// We assume the first column is shifted up, so a column is shifted up iff
// x % 2 == 0.
class Hexagon {
 private:
  Hexagon* Bottom() const;
  Hexagon* Top() const;

  int Hexagon::FindClockwiseMove(int prev_idx) const;
  int Hexagon::FindCounterClockwiseMove(int prev_idx) const;
  absl::optional<Offset> Hexagon::WalkThree(int i, bool clockwise) const;
  std::vector<Hexagon*> Hexagon::FindJumpMoves() const;

  void Hexagon::GenerateBeeMoves(std::vector<HiveMove> &moves) const;
  void Hexagon::GenerateBeetleMoves(std::vector<HiveMove> &moves) const;
  void Hexagon::GenerateAntMoves(std::vector<HiveMove> &moves) const;
  void Hexagon::GenerateGrasshopperMoves(std::vector<HiveMove> &moves) const;
  void Hexagon::GenerateSpiderMoves(std::vector<HiveMove> &moves) const;
  void Hexagon::GenerateLadybugMoves(std::vector<HiveMove> &moves) const;
  void Hexagon::GenerateMosquitoMoves(std::vector<HiveMove> &moves) const;
  void Hexagon::GeneratePillbugMoves(std::vector<HiveMove> &moves) const;

 public:
  Hexagon();

  Offset loc;

  absl::optional<Bug> bug;

  Hexagon* above;
  Hexagon* below;
  std::array<Hexagon*, 6> neighbours;

  bool visited;

  void Hexagon::GenerateMoves(BugType t, std::vector<HiveMove> &moves) const;
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
  explicit BugCollection();

  void ReturnBug(BugType t);

  bool HasBug(BugType t) const;
  bool UseBug(BugType t);

 private:
  std::array<int8_t, 8> bug_counts_;
};

// Simple Hive board.
class HiveBoard {
 public:
  explicit HiveBoard(int board_size);

  void Clear();

  bool IsLegalMove(HiveMove& m) const;

  void PlayMove(HiveMove& m);

  std::string ToString();

  std::array<int8_t, kBoardSize*kBoardSize*kBoardHeight> observation;

 private:
  Hexagon* GetHexagon(Offset o);
  Hexagon* GetHexagon(int8_t x, int8_t y, int8_t z);
  absl::optional<Bug> RemoveBug(Hexagon* h);
  void PlaceBug(Offset o, BugType b);

  absl::optional<Color> HexagonOwner(Hexagon* h) const;
  void CacheHexagonOwner(Hexagon* h);

  void GenerateLegalMoves();

  int8_t board_size_;
  int8_t board_height_;

  uint64_t zobrist_hash_;

  Color to_play_;
  std::array<BugCollection, 2> bug_collections_;

  std::array<std::unordered_set<Offset>, 2> available_;
  std::vector<Hexagon*> hexagons_;
  std::array<Hexagon, kBoardSize*kBoardSize*(kBoardHeight+1)> board_;
  std::array<bool, kBoardSize*kBoardSize> visited_;

  std::vector<HiveMove> legalMoves_;
};

std::ostream &operator<<(std::ostream &os, const HiveBoard &board);

HiveBoard BoardFromFEN(std::string fen);

}  // namespace go
}  // namespace open_spiel

template<> struct std::hash<open_spiel::hive::Offset> {
    std::size_t operator()(open_spiel::hive::Offset const& o) const noexcept {
        return std::hash<int>{}(o.index);
    }
};

#endif  // OPEN_SPIEL_GAMES_HIVE_HIVE_BOARD_H_
