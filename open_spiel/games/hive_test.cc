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

#include "open_spiel/spiel.h"
#include "open_spiel/tests/basic_tests.h"

namespace open_spiel {
namespace hive {
namespace {

namespace testing = open_spiel::testing;

const std::vector<std::vector<std::string>> human_games = {
  {
    "wG1", "bB1 /wG1", "wQ \\wG1", "bG1 /bB1", "wA1 wG1-", "bB2 -bB1",
    "wB1 wQ-", "bQ bG1\\", "wA1 \\bB2", "bS1 bG1-", "wB1 wG1", "bS2 -bG1",
    "wA2 \\wQ", "bG2 bS2\\", "wG2 \\wA1", "bG1 \\wG2", "wA2 bQ\\", "bA1 -bG1",
    "wA2 -bA1", "bA2 bQ\\", "wG3 wB1-", "bG3 bA1/", "wA3 wQ-", "bA2 wA3/",
    "wB1 bB1", "bA3 bG3-", "wB1 bS1", "bA3 \\wQ", "wB1 bQ", "bG3 bS2-",
    "wS1 wG3\\", "bA3 wS1-", "wB2 wB1\\", "bG3 wB2\\", "wB1 wB2", "bS1 wS1\\",
    "wB1 bG3", "bA3 bQ/",
  },
  {
    "wL", "bL wL-", "wA1 -wL", "bM bL-", "wQ wA1/", "bP bL/", "wA2 wQ/",
    "bQ bP-", "wA1 bQ/", "bA1 bL\\", "wA2 bA1-", "bA2 \\bP", "wP \\wQ",
    "bA2 wA1-", "wM wP-", "bS1 bA2\\", "wB1 \\wA1", "bS1 wB1-", "wB1 wA1",
    "bA2 wM/", "wP wM-", "bL bM-", "wA2 bA1\\", "bA2 -wQ", "wB1 bQ",
    "bB1 \\bA2", "wA3 wM/", "bB1 bA2", "wA3 wB1-", "bL wL-", "wB1 bM",
    "wP wQ-", "wM wP/", "bL /wL", "wA2 wM-", "bA1 wB1-",
  },
  {
    "wL", "bL /wL", "wA1 wL/", "bM /bL", "wQ -wA1", "bA1 -bL", "wA1 bM\\",
    "bQ -bM", "wA2 /wA1", "bA1 wA2\\", "wQ -wL", "bP \\bQ", "wM wA1-",
    "bG1 -bQ", "wP \\wQ", "bA2 -bP", "wM -bA2", "bA3 bA2/", "wL bQ\\",
    "bP bQ/", "wA1 bG1\\", "bQ bM-", "wS1 wP-", "bB1 bL-", "wS1 bB1-",
    "bP wL-", "wB1 wP-", "bA3 wB1/", "wB2 wS1\\", "bQ bP-", "wB2 wS1",
    "bB2 bM-", "wA3 -wP", "bA3 \\wA3", "wB2 bB1/", "bS1 bA3/", "wS2 wP/",
    "bS1 wS2/", "wB2 bB1", "bB2 wB2", "wM wQ-", "bB2 wM", "wB1 bB2",
    "bA2 wS1-", "wB2 bL", "bG2 bQ\\", "wB2 bM", "bG2 wB2-", "wG1 wA1\\",
    "wL wA2-", "wB1 bB1", "bQ wL-", "wB1 bG2", "bG3 bA2-", "wG2 wA3\\",
    "bA3 -wA3", "wG2 wP-", "bB2 wQ", "wM bB2", "bG3 -bL", "wG2 -bA3",
    "bS1 wM-", "wG3 wS2-", "bA1 -wA1", "wB1 bB1", "bG1 wG1\\", "wG3 wA3/",
    "bG1 bS1-", "wS2 bG1/", "bA2 \\wG3", "wS1 bQ/", "bA2 wS1-", "wG3 bA3\\",
    "bA2 /wG1", "wG3 wA3/", "bG2 \\wG3", "wB2 bG3", "bA2 wP-", "wB2 -bG3",
    "bG2 bM-", "bA2 wG3-", "bA1 wP-", "wB2 wA3\\",
  },
  {
    "wG1", "bL /wG1", "wQ \\wG1", "bQ bL\\", "wA1 wG1-", "bS1 -bL", "wP \\wQ",
    "bS1 -wP", "wA2 wQ-", "bB1 -bS1", "wA1 bQ\\", "bB1 bS1", "wB1 wA1-",
    "bB1 wP", "wB1 wA1", "bA1 bB1/", "wA2 \\bA1", "bB2 -bQ", "wA3 wB1-",
    "bB2 bL", "wA3 bQ-", "bS2 \\bS1", "wM wB1-", "bS2 bS1\\", "wM -bQ",
    "bA2 -bS2", "wA2 bB2-", "bB2 wG1", "wB1 wM\\",
  },
  {
    "wL", "bL /wL", "wM wL/", "bM /bL", "wA1 wM/", "bQ -bL", "wQ -wM",
    "bP -bM", "wP wQ/", "bA1 bP\\", "wM -bQ", "bM wA1\\", "wB1 \\wM",
    "bM -wB1", "wB2 wQ-", "bM wB1", "wA1 bA1-", "bB1 -bA1", "wB2 wL",
    "bB1 bP-", "bP -wM", "bA1 bB1-", "wA1 /bB1", "bA1 wM\\", "wA2 wP/",
    "bQ bM-", "wB2 bL", "bQ bM/", "wA2 \\bQ", "bM -bQ", "bP -bA1", "bQ wA2-",
    "wA3 \\wA2", "bP -wM", "wG1 wB1-", "bP -wB1", "wA3 bQ/", "bP -wA2",
    "wB2 bB1", "bP \\wA2", "wA1 \\bP", "bA2 /bA1", "wA1 /bA2", "bP wA2/",
    "wB2 bA1", "wA3 -bP", "wB2 wM", "bP wA3/", "wB2 wB1", "bP \\wA3",
    "wB2 bM", "bQ wA3-", "wA1 bQ-", "bB2 -bP", "wB2 wA2", "bB2 -wA3",
    "wB2 bQ", "bB2 wA3", "wB2 bB2", "bA3 bA2-", "wG1 -wB1", "bA3 /wG1",
    "wS1 wB1-", "bS1 bA3\\", "wA1 bQ/", "bA2 wA2-", "wS1 -wL", "bA2 bM-",
    "wS1 bQ-", "bM bP-", "wB2 bM", "bB2 wB2", "wG2 wA1-", "wA3 bP/", "wG2 wA2-",
    "bS2 /bP", "wA3 /bS1", "bG1 bB1\\", "wP wQ-", "bG1 wM-", "wG3 wS1-",
    "bA1 wG3/", "wA3 wA1-", "bA1 wG3-", "wA2 bS2-", "bA1 wA3/", "wA2 -wG2",
    "bA1 wG3-", "wS2 wA1/", "bS1 -wG1", "wA2 bS2-", "bA3 wS2-", "wA2 -wG2",
    "bB1 bL", "wP wQ/", "bA2 wB1-", "bA1 wG2-", "bS2 bP\\",
  },
};

void BasicHiveTests() {
  testing::LoadGameTest("hive");
  testing::NoChanceOutcomesTest(*LoadGame("hive"));
  testing::RandomSimTest(*LoadGame("hive"), 3);
  testing::RandomSimTestWithUndo(*LoadGame("hive"), 3);
}

void HumanGamesTests() {
  for (std::vector<std::string> game_moves : human_games) {
    std::cout << "starting test game" << std::endl;
    auto game = LoadGame("hive");
    auto state = game->NewInitialState();
    for (std::string move : game_moves) {
      SPIEL_CHECK_FALSE(state->IsTerminal());
      std::unordered_map<std::string, Action> legal_actions_map;
      for (Action a : state->LegalActions()) {
        legal_actions_map[state->ActionToString(a)] = a;
      }
      SPIEL_CHECK_EQ(legal_actions_map.count(move), true);
      state->ApplyAction(legal_actions_map[move]);
    }
  }
}

}  // namespace
}  // namespace hive
}  // namespace open_spiel

int main(int argc, char** argv) {
  //open_spiel::hive::BasicHiveTests();
  open_spiel::hive::HumanGamesTests();
}
