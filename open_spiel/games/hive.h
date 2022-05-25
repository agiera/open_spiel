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

#include "open_spiel/games/hive/hive_board.h"
#include "open_spiel/spiel.h"

// Simple game of Noughts and Crosses:
// https://en.wikipedia.org/wiki/Tic-tac-toe
//
// Parameters: none

namespace open_spiel {
namespace hive {

using hive::BugType;
using hive::Color;
using hive::Bug;
using hive::Hexagon;

// 1 Bee
// 2 Beetles
// 3 Ants
// 3 Grasshoppers
// 2 Spiders
// 1 Ladybugs
// 1 Mosquito
// 1 Pillbug

// Constants.
inline constexpr int kNumPlayers = 2;

// The maximum length the tiles can span is 28, so the hive can never wrap
// around and complete the connection
inline constexpr int kBoardSize = 29;
// there can be a stack of bugs up to 7 high
inline constexpr int kBoardHeight = 7;
inline constexpr int kNumHexagons = kBoardSize*kBoardSize*kBoardHeight;
inline constexpr int kNumBugTypes = 8;
inline constexpr int kNumCellStates = 2*kNumBugTypes + 1;

// State of an in-play game.
class HiveState : public State {
 public:
  HiveState(std::shared_ptr<const Game> game);

  HiveState(const HiveState&) = default;
  HiveState& operator=(const HiveState&) = default;

  Player CurrentPlayer() const override {
    return IsTerminal() ? kTerminalPlayerId : current_player_;
  }
  std::string ActionToString(Player player, Action action_id) const override;
  std::string ToString() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  std::string InformationStateString(Player player) const override;
  std::string ObservationString(Player player) const override;
  void ObservationTensor(Player player,
                         absl::Span<float> values) const override;
  std::unique_ptr<State> Clone() const override;
  void UndoAction(Player player, Action move) override;
  std::vector<Action> LegalActions() const override;
  Player outcome() const { return outcome_; }

  // Only used by Ultimate Tic-Tac-Toe.
  void SetCurrentPlayer(Player player) { current_player_ = player; }

 protected:
  void DoApplyAction(Action move) override;

 private:
  Player current_player_ = 0;         // Player zero goes first
  Player outcome_ = kInvalidPlayer;
  int num_moves_ = 0;
};

// Game object.
class HiveGame : public Game {
 public:
  explicit HiveGame(const GameParameters& params);
  // An action can consist of moving one piece from one point to another or
  // placing one of the eight pieces on some point.
  int NumDistinctActions() const override {
    return kNumHexagons + kBoardHeight;
  }
  std::unique_ptr<State> NewInitialState() const override {
    return std::unique_ptr<State>(new HiveState(shared_from_this()));
  }
  int NumPlayers() const override { return kNumPlayers; }
  double MinUtility() const override { return -1; }
  double UtilitySum() const override { return 0; }
  double MaxUtility() const override { return 1; }
  std::vector<int> ObservationTensorShape() const override {
    return {kNumCellStates + 1 + 1, kBoardSize, kBoardSize};
  }
};

}  // namespace hive
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_HIVE_H_
