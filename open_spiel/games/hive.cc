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

#include "open_spiel/games/hive.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "open_spiel/games/hive/hive_board.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/utils/tensor_view.h"

namespace open_spiel {
namespace hive {
namespace {

// Facts about the game.
const GameType kGameType{
    /*short_name=*/"hive",
    /*long_name=*/"Hive",
    GameType::Dynamics::kSequential,
    GameType::ChanceMode::kDeterministic,
    GameType::Information::kPerfectInformation,
    GameType::Utility::kZeroSum,
    GameType::RewardModel::kTerminal,
    /*max_num_players=*/2,
    /*min_num_players=*/2,
    /*provides_information_state_string=*/true,
    /*provides_information_state_tensor=*/false,
    /*provides_observation_string=*/true,
    /*provides_observation_tensor=*/true,
    /*parameter_specification=*/{}  // no parameters
};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new HiveGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

}  // namespace

HiveMove HiveState::ActionToHiveMove(Action move) const {
  const int hex_idx_mask = (1 << kNumIndexBits) -1;
  bool pass = (move & 1) != 0;
  bool place = ((move >> 1) & 1) != 0;
  return HiveMove{
    pass,
    place,
    (BugType) ((move >> 2) & 7),
    Offset((move >> 2) & hex_idx_mask),
    Offset((move >> (2 + kNumIndexBits)) & hex_idx_mask),
  };
}

Action HiveState::HiveMoveToAction(HiveMove move) const {
  return (move.to.index << kNumIndexBits + 5) | (move.from.index << 5) |
         (move.bug << 2) | (move.place << 1) | move.pass;
}

void HiveState::DoApplyAction(Action move) {
  HiveMove hive_move = ActionToHiveMove(move);
  moves_history_.push(hive_move);
  repetitions_.extract(board_.zobrist_hash);
  board_.PlayMove(hive_move);
  cached_legal_actions_ = absl::nullopt;
}

void HiveState::UndoAction(Player player, Action move) {
  SPIEL_CHECK_GE(moves_history_.size(), 1);
  repetitions_.insert(board_.zobrist_hash);
  HiveMove last_move = moves_history_.top();
  moves_history_.pop();
  history_.pop_back();
  --move_number_;
  board_.UndoMove(last_move);
  cached_legal_actions_ = absl::nullopt;
}

std::vector<Action> HiveState::LegalActions() const {
  if (!cached_legal_actions_.has_value()) {
    cached_legal_actions_ = {};
    for (HiveMove m : board_.LegalMoves()) {
      cached_legal_actions_->push_back(HiveMoveToAction(m));
    }
  }
  absl::c_sort(*cached_legal_actions_);
  return *cached_legal_actions_;
}

std::string HiveState::ActionToString(Player player,
                                      Action action_id) const {
  HiveMove m = ActionToHiveMove(action_id);
  if (m.pass) { return "pass"; }
  // TODO
  if (m.place) {
    return "";
  } else {
    return "";
  }
}

HiveState::HiveState(std::shared_ptr<const Game> game) : State(game) {}

std::string HiveState::ToString() const {
  // TODO
  return "";
}

bool HiveState::IsTerminal() const {
  return board_.is_terminal;
}

std::vector<double> HiveState::Returns() const {
  if (board_.outcome == kWhite) {
    return {1.0, -1.0};
  } else if (board_.outcome == kBlack) {
    return {-1.0, 1.0};
  } else {
    return {0.0, 0.0};
  }
}

std::string HiveState::InformationStateString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
  return HistoryString();
}

std::string HiveState::ObservationString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
  return ToString();
}

void HiveState::ObservationTensor(Player player,
                                       absl::Span<float> values) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
}

std::unique_ptr<State> HiveState::Clone() const {
  return std::unique_ptr<State>(new HiveState(*this));
}

HiveGame::HiveGame(const GameParameters& params)
    : Game(kGameType, params) {}

}  // namespace hive
}  // namespace open_spiel
