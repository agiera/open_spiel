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
    /*parameter_specification=*/{}  // no parameters
};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new HiveGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

// Adds a uniform scalar plane scaled with min and max.
template <typename T>
void AddScalarPlane(T val, T min, T max,
                    absl::Span<float>::iterator& value_it) {
  double normalized_val = static_cast<double>(val - min) / (max - min);
  for (int i = 0; i < kBoardSize*kBoardSize; ++i) *value_it++ = normalized_val;
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
  //std::cout << "\nHiveActionToHiveMove\n";
  if (action.pass) { return HiveMove(); }
  Hexagon from = board_.GetHexagon(action.from);
  //std::cout << "from=" << BugToString(from.bug) << "\n";
  Hexagon around = board_.GetHexagon(action.around);
  //std::cout << "around=" << BugToString(around.bug) << "\n";
  Hexagon to = around;
  if (to.bug != kEmptyBug) {
    to = board_.GetHexagon(to.neighbours[action.neighbour]);
    to = board_.Top(to);
    //std::cout << "to=" << BugToString(to.bug) << "\n";
    if (to.bug != kEmptyBug && to.bug != from.bug) {
      to = board_.GetHexagon(to.above);
    }
  }
  // Case that player is placing a bug
  if (from.bug == kEmptyBug) {
    if (board_.NumBugs() == 0) {
      return HiveMove(action.from.type, starting_hexagon);
    } else if (board_.NumBugs() == 1) {
      return HiveMove(action.from.type, starting_hexagon + 1);
    }
    return HiveMove(action.from.type, to.loc.index);
  }
  return HiveMove(from.loc.index, to.loc.index);
}

HiveAction HiveState::HiveMoveToHiveAction(HiveMove move) const {
  //std::cout << "\nHiveMoveToHiveAction\n";
  //std::cout << "move.pass=" << move.pass << "\n";
  if (move.pass) { return HiveAction{true}; }
  //std::cout << "move.place=" << move.place << "\n";
  //std::cout << "move.to=" << move.to << "\n";
  //std::cout << "board_.to_play=" << board_.to_play << "\n";

  Bug from;
  if (move.place) {
    int8_t order = board_.NumBugs(board_.to_play, move.bug_type);
    from = Bug{board_.to_play, move.bug_type, order};
  } else {
    from = board_.GetHexagon(move.from).bug;
  }
  //std::cout << "from=" << BugToString(from) << "\n";

  Bug around = Bug{kWhite, kBee, 0};
  int8_t neighbour_idx = 0;
  Hexagon to = board_.GetHexagon(move.to);
  //std::cout << "to=" << HexagonToString(to) << "\n";
  for (int8_t i=0; i < 6; i++) {
    Hexagon neighbour = board_.GetHexagon(to.neighbours[i]);
    neighbour = board_.Bottom(neighbour);
    // Don't use self for reference
    if (neighbour.bug != kEmptyBug && neighbour.bug != from) {
      neighbour = board_.Top(neighbour);
      around = neighbour.bug;
      neighbour_idx = mod(i - 3, 6);
    }
  }
  //std::cout << "around=" << BugToString(around) << "\n";
  //std::cout << "neighbour_idx=" << (int) neighbour_idx << "\n";

  // Add optional attributes for string representation
  bool first = board_.NumBugs() < 2;
  Hexagon top = board_.Top(to);
  //std::cout << "top.bug=" << BugToString(top.bug) << "\n";
  bool jump = top.bug != kEmptyBug;
  Bug on = top.bug;
  if (!jump) {
    on = Bug{kWhite, kBee, 0};
  }

  return HiveAction{false, from, around, neighbour_idx, first, jump, on};
}

HiveAction HiveState::ActionToHiveAction(Action action) const {
  //std::cout << "\nActionToHiveAction\n";
  int min_bits = action >> 8;
  if (action == kNumActions - 1) {
    return HiveAction{true};
  }
  int8_t from_order = min_bits / 18;
  int8_t around_order = (min_bits % 18) / 6;
  int8_t neighbour = min_bits % 6;

  Player from_player = (action >> 0) & 0b1;
  BugType from_type = (BugType) ((action >> 1) & 0b111);
  Bug from = Bug{from_player, from_type, from_order};
  //std::cout << "from=" << BugToString(from) << "\n";

  Player around_player = (action >> 4) & 0b1;
  BugType around_type = (BugType) ((action >> 5) & 0b111);
  Bug around = Bug{around_player, around_type, around_order};

  // This is a first move
  if (board_.NumBugs() < 2) {
    return HiveAction{false, from, around, neighbour, true, false, false};
  }

  // Add optional attributes for string representation
  Hexagon around_hex = board_.GetHexagon(around);
  bool jump = false;
  Bug on = Bug{kWhite, kBee, 0};
  //std::cout << "around_hex.bug=" << BugToString(around_hex.bug) << "\n";
  if (around_hex.bug != kEmptyBug) {
    Hexagon to = board_.GetHexagon(around_hex.neighbours[neighbour]);
    to = board_.Top(to);
    //std::cout << "to.bug=" << BugToString(to.bug) << "\n";
    jump = to.bug != kEmptyBug && to.bug != from;
    //std::cout << "jump=" << jump << "\n";
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
  SPIEL_CHECK_GE((int) action.from.order, 0);
  SPIEL_CHECK_LT((int) action.from.order, 6);
  SPIEL_CHECK_GE((int) action.around.order, 0);
  SPIEL_CHECK_LT((int) action.around.order, 6);
  SPIEL_CHECK_GE((int) action.neighbour, 0);
  SPIEL_CHECK_LT((int) action.neighbour, 6);
  Action res = action.from.order*18 + action.around.order*6 + action.neighbour;
  res <<= 8;

  res |= action.from.player << 0;
  res |= action.from.type << 1;

  res |= action.around.player << 4;
  res |= action.around.type << 5;

  return res;
}

void HiveState::DoApplyAction(Action move) {
  //std::cout << "\nDoApplyAction\n";
  HiveAction hive_action = ActionToHiveAction(move);
  //std::cout << "ToString()=" << ToString() << "\n";
  //std::cout << "hive_action=" << HiveActionToString(hive_action) << "\n";
  actions_history_.push_back(hive_action);
  HiveMove hive_move = HiveActionToHiveMove(hive_action);
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
      HiveAction hive_action = HiveMoveToHiveAction(hive_move);
      //std::cout << "hive_action=" << HiveActionToString(hive_action) << "\n";
      Action action = HiveActionToAction(hive_action);
      //std::cout << "action=" << (int) action << "\n";
      action_set.insert(action);
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

HiveState::HiveState(std::shared_ptr<const Game> game) : State(game) {}

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

void HiveState::ObservationTensor(Player player,
                                  absl::Span<float> values) const {
  SPIEL_DCHECK_GE(player, 0);
  SPIEL_DCHECK_LT(player, num_players_);

  auto value_it = values.begin();

  std::copy(board_.observation.begin(), board_.observation.end(), value_it);

  // Num repetitions for the current board.
  AddScalarPlane((int) repetitions_.count(board_.zobrist_hash), 1, 3, value_it);

  // TODO: Add last moved hex
  // AddBinaryPlane(board_.last_moved_, value_it);

  //SPIEL_DCHECK_EQ(value_it, values.end());
}

std::unique_ptr<State> HiveState::Clone() const {
  return std::unique_ptr<State>(new HiveState(*this));
}

HiveGame::HiveGame(const GameParameters& params)
    : Game(kGameType, params) {}

}  // namespace hive
}  // namespace open_spiel
