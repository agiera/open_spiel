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
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/utils/tensor_view.h"
#include "open_spiel/abseil-cpp/absl/algorithm/container.h"

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
    /*parameter_specification=*/
    {{"all_action_reprs", GameParameter(false)}},
};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new HiveGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

// Adds a uniform scalar plane scaled with min and max.
template <typename T>
void AddScalarPlane(T val, T min, T max,
                    absl::Span<float>::iterator& value_it) {
  if (val == min) {
    value_it += kTensorSize*kTensorSize;
    return;
  }
  double normalized_val = static_cast<double>(val - min) / (max - min);
  for (int i = 0; i < kTensorSize*kTensorSize; ++i) *value_it++ = normalized_val;
}

}  // namespace

std::string HiveActionToString(HiveAction action) {
  if (action.pass) { return "pass"; }

  std::string res = BugToString(action.from);
  if (action.first) { return res; }
  if (action.jump) {
    res += " " + BugToString(action.on);
    return res;
  }

  res += " ";
  if (action.neighbour == 0 || action.neighbour > 3) {
    res += kNeighbourSymbols[action.neighbour];
  }
  res += BugToString(action.around);
  if (action.neighbour != 0 && action.neighbour <= 3) {
    res += kNeighbourSymbols[action.neighbour];
  }

  return res;
}

HiveMove HiveState::HiveActionToHiveMove(HiveAction action) const {
  if (action.pass) { return HiveMove(); }
  Hexagon from = board_.GetHexagonFromBug(action.from);
  Hexagon around = board_.GetHexagonFromBug(action.around);
  Offset to;
  // Handles case where the first two moves don't have adjacent bugs
  if (around.bug != kEmptyBug) {
    to = around.loc.neighbours[action.neighbour];
  }
  // Case that player is placing a bug
  if (from.bug == kEmptyBug) {
    if (actions_history_.size() == 0) {
      return HiveMove(action.from.type, starting_hexagon);
    }
    return HiveMove(action.from.type, to.idx);
  }
  return HiveMove(from.loc.idx, to.idx);
}

std::vector<HiveAction> HiveState::HiveMoveToHiveActions(HiveMove move) const {
  std::vector<HiveAction> hive_actions;
  if (move.pass) {
    hive_actions.push_back(HiveAction{true});
    return hive_actions;
  }

  Bug from;
  if (move.place) {
    int8_t order = board_.NumBugs(board_.to_play, move.bug_type);
    from = Bug{board_.to_play, move.bug_type, order};
  } else {
    from = board_.GetHexagon(move.from).bug;
  }

  Bug around = Bug{kWhite, kBee, 0};
  uint8_t neighbour_idx = 0;

  if (actions_history_.size() < 1) {
    hive_actions.push_back({false, from, around, neighbour_idx, true});
    return hive_actions;
  }

  Hexagon to = board_.GetHexagon(move.to);
  Offset to_off = Offset(move.to);

  // Add optional attributes for string representation
  bool jump = to.bug != kEmptyBug;
  Bug on = to.bug;
  if (!jump) {
    on = Bug{kWhite, kBee, 0};
  }

  for (int8_t i=0; i < 6; i++) {
    Hexagon neighbour = board_.GetHexagon(to_off.neighbours[i]);
    if (neighbour.bug != kEmptyBug) {
      // Include beneath self for reference
      if (neighbour.bug == from) {
        if (from.below != (uint8_t) -1) {
          around = Bug(from.below);
          neighbour_idx = mod(i - 3, 6);
          hive_actions.push_back({false, from, around, neighbour_idx, false, jump, on});
        }
      }
      around = neighbour.bug;
      neighbour_idx = mod(i - 3, 6);
      hive_actions.push_back({false, from, around, neighbour_idx, false, jump, on});
    }
  }

  return hive_actions;
}

HiveAction HiveState::ActionToHiveAction(Action action) const {
  int min_bits = action >> 8;
  if (action == kNumActions - 1) {
    return HiveAction{true};
  }

  uint8_t neighbour = action / (kNumBugs*kNumBugs);
  int remainder = action % (kNumBugs*kNumBugs);
  Bug around = Bug(remainder / kNumBugs);
  remainder = remainder % kNumBugs;
  Bug from = Bug(remainder);

  // This is a first move
  if (actions_history_.size() < 1) {
    return HiveAction{false, from, around, neighbour, true, false};
  }

  // Add optional attributes for string representation
  Hexagon around_hex = board_.GetHexagonFromBug(around);
  bool jump = false;
  Bug on = Bug{kWhite, kBee, 0};
  if (around_hex.bug != kEmptyBug) {
    Hexagon to = board_.GetHexagon(around_hex.loc.neighbours[neighbour]);
    jump = to.bug != kEmptyBug && to.bug != from;
    if (jump) {
      on = to.bug;
    }
  }

  return HiveAction{false, from, around, neighbour, false, jump, on};
}

Action HiveState::HiveActionToAction(HiveAction action) const {
  if (action.pass) { return kNumActions - 1; }

  // A way to encode three numbers ranging from 0 to 5
  // This is put in front so the number of actions is minimized
  SPIEL_CHECK_GE((int) action.from.idx, 0);
  SPIEL_CHECK_LT((int) action.from.idx, kNumBugs);
  SPIEL_CHECK_GE((int) action.around.idx, 0);
  SPIEL_CHECK_LT((int) action.around.idx, kNumBugs);
  SPIEL_CHECK_GE((int) action.neighbour, 0);
  SPIEL_CHECK_LT((int) action.neighbour, 6);

  return action.from.idx + action.around.idx*kNumBugs +
         action.neighbour*kNumBugs*kNumBugs;
}

void HiveState::DoApplyAction(Action move) {
  HiveAction hive_action = ActionToHiveAction(move);
  HiveMove hive_move = HiveActionToHiveMove(hive_action);

  actions_history_.push_back(hive_action);
  moves_history_.push_back(hive_move);

  repetitions_.insert(board_.zobrist_hash);

  board_.PlayMove(hive_move);

  cached_legal_actions_.clear();
}

void HiveState::UndoAction(Player player, Action move) {
  SPIEL_DCHECK_GE(moves_history_.size(), 1);
  repetitions_.insert(board_.zobrist_hash);
  actions_history_.pop_back();
  HiveMove last_move = moves_history_.back();
  moves_history_.pop_back();
  history_.pop_back();
  --move_number_;
  board_.UndoMove(last_move);
  cached_legal_actions_.clear();
}

std::vector<Action> HiveState::LegalActions() const {
  std::unordered_set<Action> action_set;
  if (cached_legal_actions_.empty()) {
    for (HiveMove hive_move : board_.LegalMoves()) {
      for (HiveAction hive_action : HiveMoveToHiveActions(hive_move)) {
        Action action = HiveActionToAction(hive_action);
        action_set.insert(action);
        if (!all_action_reprs_) break;
      }
    }
    cached_legal_actions_.assign(action_set.begin(), action_set.end());
    absl::c_sort(cached_legal_actions_);
  }
  return cached_legal_actions_;
}

std::string HiveState::ActionToString(Player player,
                                      Action action_id) const {
  HiveAction m = ActionToHiveAction(action_id);
  return HiveActionToString(m);
}

HiveState::HiveState(std::shared_ptr<const Game> game, bool all_action_reprs)
  : State(game),
    all_action_reprs_(all_action_reprs) {}

std::string HiveState::ToString() const {
  std::string res = "Base+MLP;";
  if (actions_history_.empty()) { return res + "NotStarted;White[1]"; }

  const std::array<std::string, 2> colors = { "White", "Black" };
  res += "InProgress;";
  res += colors[board_.to_play];
  res += "[" + std::to_string(actions_history_.size()/2 + 1) + "];";

  for (HiveAction action : actions_history_) {
    res += HiveActionToString(action) + ";";
  }
  res.pop_back();
  return res;
}

bool HiveState::IsTerminal() const {
  if (repetitions_.count(board_.zobrist_hash) == kNumRepetitionsToDraw) {
    return true;
  }
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
  SPIEL_DCHECK_GE(player, 0);
  SPIEL_DCHECK_LT(player, num_players_);
  return HistoryString();
}

std::string HiveState::ObservationString(Player player) const {
  SPIEL_DCHECK_GE(player, 0);
  SPIEL_DCHECK_LT(player, num_players_);
  return ToString();
}

// Representation is encoded as an adjacency graph.
// However that wouldn't encode the structure of the hexagons properly.
// The way to fix it is to introduce 6 new bugs. A bug's will be adjecent to
// the ith new bug if it's ith side isn't adjacent to a different bug.
void HiveState::ObservationTensor(Player player,
                                  absl::Span<float> values) const {
  SPIEL_DCHECK_GE(player, 0);
  SPIEL_DCHECK_LT(player, num_players_);

  auto value_it = values.begin();

  int adj_mat_size = kNumBugs*(kNumBugs + 6);

  for (int i=0; i < kNumBugs; i++) {
    Hexagon h = board_.GetHexagonFromBug(i);
    if (h.bug == kEmptyBug) {
      value_it += kNumBugs + 6;
      continue;
    }
    if (h.bug.above != (uint8_t) -1) {
      Hexagon above = board_.GetHexagonFromBug(h.bug.above);
      int bug_val = above.bug.type == kBeetle;
      values[adj_mat_size + kNumBugs*bug_val + h.bug.idx] = 1.0;
      value_it += kNumBugs + 6;
      continue;
    }
    for (int n_idx=0; n_idx < 6; n_idx++) {
      int n = h.bug.neighbours[n_idx];
      if (n == (uint8_t) -1) {
        value_it[kNumBugs + n_idx] = 1.0;
      } else {
        value_it[n] = 1.0;
      }
    }
    value_it += kNumBugs + 6;
  }

  value_it += (15 + 1)*8*8 - adj_mat_size;
 
  // Num repetitions for the current board.
  AddScalarPlane((int) repetitions_.count(board_.zobrist_hash), 1, 3, value_it);
 
  int last_moved = board_.last_moved_.back();
  AddScalarPlane(last_moved, -1, kNumBugs-1, value_it);
 
  AddScalarPlane(board_.to_play, 0, 1, value_it);

  SPIEL_DCHECK_EQ(value_it, values.end());
}

std::unique_ptr<State> HiveState::Clone() const {
  return std::unique_ptr<State>(new HiveState(*this));
}

HiveGame::HiveGame(const GameParameters& params)
    : Game(kGameType, params),
      all_action_reprs_(ParameterValue<bool>("all_action_reprs")) {}

}  // namespace hive
}  // namespace open_spiel
