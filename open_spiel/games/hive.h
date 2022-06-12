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

#ifndef OPEN_SPIEL_GAMES_HIVE_H_
#define OPEN_SPIEL_GAMES_HIVE_H_

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <math.h>

#include "open_spiel/spiel.h"
#include "open_spiel/games/hive/hive_board.h"

// Simple game of Noughts and Crosses:
// https://en.wikipedia.org/wiki/Tic-tac-toe
//
// Parameters: none

namespace open_spiel {
namespace hive {

// Constants.
inline constexpr int kNumPlayers = 2;

const int kNumCellStates = 2*2*kNumBugTypes + 1;
const int kNumberStates = 3 * (1 + kNumBugs * 2) * kBoardSize * kBoardSize * kBoardHeight;
// TODO: lower number of actions by implementing better packing in encoding and decoding
//       Does this actually help?
//const int kNumActions = 1 + kNumBugs * kNumHexagons + kNumHexagons * kNumHexagons;
const int kNumActions = 2 * 2 * kNumBugs * kNumHexagons * kNumHexagons;
const int kNumIndexBits = ceil(log2(kNumHexagons));

// State of an in-play game.
class HiveState : public State {
 public:
  HiveState(std::shared_ptr<const Game> game);

  HiveState(const HiveState&) = default;
  HiveState& operator=(const HiveState&) = default;

  Player CurrentPlayer() const override {
    return IsTerminal() ? kTerminalPlayerId : board_.to_play;
  }

  HiveMove ActionToHiveMove(Action move) const;
  Action HiveMoveToAction(HiveMove move) const;
  void UndoAction(Player player, Action move) override;

  std::string ActionToString(Player player, Action action_id) const override;
  std::string ToString() const override;

  Player outcome() const { return board_.outcome; }
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;

  std::string InformationStateString(Player player) const override;
  std::string ObservationString(Player player) const override;
  void ObservationTensor(Player player,
                         absl::Span<float> values) const override;

  std::unique_ptr<State> Clone() const override;
  std::vector<Action> LegalActions() const override;

 protected:
  void DoApplyAction(Action move) override;

 private:
  int num_moves_ = 0;
  std::stack<HiveMove> moves_history_;
  std::multiset<uint64_t> repetitions_;

  mutable absl::optional<std::vector<Action>> cached_legal_actions_;

  HiveBoard board_;
};

// Game object.
class HiveGame : public Game {
 public:
  explicit HiveGame(const GameParameters& params);
  int NumDistinctActions() const override { return kNumActions; }
  std::unique_ptr<State> NewInitialState() const override {
    return std::unique_ptr<State>(new HiveState(shared_from_this()));
  }
  int NumPlayers() const override { return kNumPlayers; }
  double MinUtility() const override { return -1; }
  double UtilitySum() const override { return 0; }
  double MaxUtility() const override { return 1; }
  std::vector<int> ObservationTensorShape() const override {
    static std::vector<int> shape{
      13 /* piece types * colours + empty */ + 1 /* repetition count */ +
          1 /* last moved piece */ + 1 /* side to move */,
      kBoardSize, kBoardSize, kBoardHeight};
    return shape;
  }
  // TODO: figure out resonable upper bound
  int MaxGameLength() const override { return 10000000000; }
};

}  // namespace hive
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_HIVE_H_
