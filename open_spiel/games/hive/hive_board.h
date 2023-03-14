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
#include <stack>
#include <cstdint>
#include <cmath>
#include <ostream>
#include <vector>
#include <unordered_set>
#include <set>

#include "open_spiel/games/hive/hive_utils.h"
#include "open_spiel/games/chess/chess_common.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/abseil-cpp/absl/types/optional.h"

namespace open_spiel {
namespace hive {

static const chess_common::ZobristTable<int64_t, 2, kNumBugs, kBoardSize, kBoardSize, kBoardHeight> zobristTable(/*seed=*/2346);

// In case all the bug types are represented in the same plane, these values are
// used to represent each piece type.
static inline constexpr std::array<float, kNumBugTypes> kBugTypeRepresentation = {
    {1, 0.875, 0.75, 0.625, 0.5, 0.375, 0.25, 0.125}};

// Simple Hive board.
class HiveBoard {
 public:
  explicit HiveBoard();

  void Clear();
  void InitBoard();

  Hexagon GetHexagon(int idx) const;
  Hexagon GetHexagon(Offset o) const;
  Hexagon GetHexagon(int x, int y, int z) const;
  Hexagon GetHexagon(Bug b) const;

  Hexagon Top(Hexagon h) const;
  Hexagon Bottom(Hexagon h) const;

  bool IsSurrounded(int h_idx) const;

  void GenerateMoves(Hexagon h, BugType t, std::vector<HiveMove> &moves) const;

  int NumBugs() const;
  int NumBugs(Player p, BugType bt) const;

  std::vector<HiveMove> LegalMoves() const;
  void PlayMove(HiveMove& m);
  void UndoMove(HiveMove& m);

  Player to_play;

  // TODO: make these functions and have hive.cc cache them
  Player outcome;
  bool is_terminal;

  std::array<int8_t, kBoardSize*kBoardSize*kBoardHeight> observation;

  // TODO: figure out how to normalize for transations and rotations
  int64_t zobrist_hash;

 private:
  // For generating legal moves
  int FindClockwiseMove(Hexagon h, int prev_idx, Bug original) const;
  int FindCounterClockwiseMove(Hexagon h, int prev_idx, Bug original) const;
  int WalkThree(Hexagon h, int i, bool clockwise) const;
  std::vector<Hexagon> FindJumpMoves(Hexagon h) const;

  void GenerateBeeMoves(Hexagon h, std::vector<HiveMove> &moves) const;
  void GenerateBeetleMoves(Hexagon h, std::vector<HiveMove> &moves) const;
  void GenerateAntMoves(Hexagon h, std::vector<HiveMove> &moves) const;
  void GenerateGrasshopperMoves(Hexagon h, std::vector<HiveMove> &moves) const;
  void GenerateSpiderMoves(Hexagon h, std::vector<HiveMove> &moves) const;
  void GenerateLadybugMoves(Hexagon h, std::vector<HiveMove> &moves) const;
  void GenerateMosquitoMoves(Hexagon h, std::vector<HiveMove> &moves) const;
  void GeneratePillbugMoves(Hexagon h, std::vector<HiveMove> &moves) const;

  bool BugCanMove(Hexagon h) const;
  Player HexagonOwner(Hexagon h) const;
  void CacheHexagonOwner(Hexagon h);
  void CacheHexagonArea(Hexagon h);

  void CachePinnedHexagons();

  // For applying or undoing moves
  void PlaceBug(Hexagon* h, Bug b);
  Bug MoveBug(Hexagon* from, Hexagon* to);
  Bug RemoveBug(Hexagon* h);

  // For calculating outcome
  void CacheOutcome();
  void CacheIsTerminal();

  std::array<BugCollection, 2> bug_collections_;

  std::array<std::unordered_set<int>, 2> available_;
  std::unordered_set<int> pinned_;

  std::unordered_set<int> hexagons_;
  std::array<int, 2> bees_;
  std::stack<int> last_moved_;

  std::array<Hexagon, kBoardSize*kBoardSize*kBoardHeight> board_;
};

}  // namespace go
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_HIVE_HIVE_BOARD_H_
