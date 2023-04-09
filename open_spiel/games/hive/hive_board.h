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
#include "open_spiel/spiel_utils.h"
#include "open_spiel/abseil-cpp/absl/types/optional.h"

namespace open_spiel {
namespace hive {

// Simple Hive board.
class HiveBoard {
 public:
  explicit HiveBoard();

  void Clear();
  void InitBoard();

  Hexagon GetHexagon(OffsetIdx idx) const;
  Hexagon GetHexagon(Offset o) const;
  Hexagon GetHexagon(uint8_t x, uint8_t y, uint8_t z) const;
  Hexagon GetHexagonFromBug(BugIdx bug_idx) const;
  Hexagon GetHexagonFromBug(Bug b) const;

  Hexagon Top(Hexagon h) const;
  Hexagon Bottom(Hexagon h) const;
  int Height(Hexagon h) const;

  bool IsSurrounded(BugIdx h_idx) const;

  void GenerateMoves(Hexagon h, BugType t, std::vector<HiveMove> &moves) const;

  int NumBugs(Player p, BugType bt) const;

  std::vector<HiveMove> LegalMoves() const;
  void PlayMove(HiveMove& m);
  void UndoMove(HiveMove& m);

  Player to_play;

  std::vector<BugIdx> last_moved_;
  Player outcome;
  bool is_terminal;

  int64_t zobrist_hash;

 private:
  // For generating legal moves
  int FindClockwiseMove(Offset o, int prev_idx, Bug original) const;
  int FindCounterClockwiseMove(Offset o, int prev_idx, Bug original) const;
  OffsetIdx WalkThree(Hexagon h, int i, bool clockwise) const;
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
  Player HexagonOwner(OffsetIdx idx) const;
  void CacheHexagonOwner(OffsetIdx idx);
  void CacheHexagonArea(OffsetIdx idx);

  void CachePinnedHexagons();

  // For applying or undoing moves
  void PlaceBug(OffsetIdx h_idx, Bug b);
  Bug MoveBug(OffsetIdx from_idx, OffsetIdx to_idx);
  Bug RemoveBug(OffsetIdx h_idx);

  void CacheOutcome();

  std::array<BugCollection, 2> bug_collections_;

  std::array<std::unordered_set<OffsetIdx>, 2> available_;
  std::unordered_set<BugIdx> pinned_;

  std::array<BugIdx, 2> bees_;

  std::array<Hexagon, kNumBugs> hexagons_;
  std::array<Offset, kBoardSize*kBoardSize> board_;
  std::unordered_set<BugIdx> top_hexagons_;
};

}  // namespace go
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_HIVE_HIVE_BOARD_H_
