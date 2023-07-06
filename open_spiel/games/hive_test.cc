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
  {  // https://entomology.gitlab.io/hive.html?game=-eFCWQfSX3nVMRPJZPDNqA
    "wG1", "bB1 /wG1", "wQ \\wG1", "bG1 /bB1", "wA1 wG1-", "bB2 -bB1",
    "wB1 wQ-", "bQ bG1\\", "wA1 \\bB2", "bS1 bG1-", "wB1 wG1", "bS2 -bG1",
    "wA2 \\wQ", "bG2 bS2\\", "wG2 \\wA1", "bG1 \\wG2", "wA2 bQ\\", "bA1 -bG1",
    "wA2 -bA1", "bA2 bQ\\", "wG3 wB1-", "bG3 bA1/", "wA3 wQ-", "bA2 wA3/",
    "wB1 bB1", "bA3 bG3-", "wB1 bS1", "bA3 \\wQ", "wB1 bQ", "bG3 bS2-",
    "wS1 wG3\\", "bA3 wS1-", "wB2 wB1\\", "bG3 wB2\\", "wB1 wB2", "bS1 wS1\\",
    "wB1 bG3", "bA3 bQ/",
  },
  {  // https://entomology.gitlab.io/hive.html?game=_hnL5ewQyIGHVUAS5hpo8A
    "wL", "bL wL-", "wA1 -wL", "bM bL-", "wQ wA1/", "bP bL/", "wA2 wQ/",
    "bQ bP-", "wA1 bQ/", "bA1 bL\\", "wA2 bA1-", "bA2 \\bP", "wP \\wQ",
    "bA2 wA1-", "wM wP-", "bS1 bA2\\", "wB1 \\wA1", "bS1 wB1-", "wB1 wA1",
    "bA2 wM/", "wP wM-", "bL bM-", "wA2 bA1\\", "bA2 -wQ", "wB1 bQ",
    "bB1 \\bA2", "wA3 wM/", "bB1 bA2", "wA3 wB1-", "bL wL-", "wB1 bM",
    "wP wQ-", "wM wP/", "bL /wL", "wA2 wM-", "bA1 wB1-",
  },
  {  // https://entomology.gitlab.io/hive.html?game=YHn2Nwrf1zUEzs3sjyV9EA
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
  {  // https://entomology.gitlab.io/hive.html?game=7ABVIYDwZMjid93frHxZiw
    "wG1", "bL /wG1", "wQ \\wG1", "bQ bL\\", "wA1 wG1-", "bS1 -bL", "wP \\wQ",
    "bS1 -wP", "wA2 wQ-", "bB1 -bS1", "wA1 bQ\\", "bB1 bS1", "wB1 wA1-",
    "bB1 wP", "wB1 wA1", "bA1 bB1/", "wA2 \\bA1", "bB2 -bQ", "wA3 wB1-",
    "bB2 bL", "wA3 bQ-", "bS2 \\bS1", "wM wB1-", "bS2 bS1\\", "wM -bQ",
    "bA2 -bS2", "wA2 bB2-", "bB2 wG1", "wB1 wM\\",
  },
  {  // https://entomology.gitlab.io/hive.html?game=Dh3KixjI1aenHQN5TQMKrQ
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
  {  // https://entomology.gitlab.io/hive.html?game=bsn:T!HV-mangouste-Marko60-2023-04-01-1501
    "wP", "bM wP-", "wQ -wP", "bQ bM\\", "wA1 \\wQ", "bP bQ\\", "wB1 wA1-",
    "bA1 /bP", "wA1 /bA1", "bA2 bP-", "wB1 wP", "bB1 bM-", "wG1 -wQ", "bB1 bQ",
    "wA2 wG1\\", "bA2 /wA2", "wM wA2-", "bB1 wM-", "wM bB1", "bM wB1",
    "wG1 bM-", "bA3 bQ-", "wG2 -wQ", "bS1 -bA2", "wL /wM", "bS1 \\wG2",
    "wM bQ", "bG1 /bA2", "wB2 wG1/", "bG1 wQ/", "wB2 wG1-", "bA3 bA2-",
    "wG3 wG1/", "bM wA2-", "wA1 bS1/", "bL bG1/", "wG3 wM-", "bL bS1-",
  },
  {  // https://entomology.gitlab.io/hive.html?game=bsn:T!HV-D0D0-Marko60-2023-04-01-1427
    "wL", "bP wL-", "wQ \\wL", "bQ bP\\", "wM /wL", "bM bP-", "wB1 -wL",
    "bA1 bP/", "wM wL", "bM -wB1", "wP \\wQ", "bA2 bQ\\", "wA1 -wP",
    "bA2 -wA1", "wS1 wB1\\", "bA3 bP-", "wS1 -bM", "bA1 wP/", "wM bP",
    "bB1 bQ\\", "wG1 wQ-", "bA3 wG1/", "wL -bQ", "bG1 bA3/", "wG2 wB1-",
    "bB2 \\bA1", "wG2 bG1/", "bB2 bA1", "wB2 wB1-", "bB2 wP", "wB2 wM",
    "bL bA1-", "wB2 wG1-", "bL bM/", "wB1 bL", "bA1 bM-", "wB1 bB2",
    "bG2 bB1\\", "wA2 wG2/", "bG2 wB1-", "wG1 bB1\\", "bL wQ-", "wB2 wM-",
    "bS1 bG2/", "wB2 bQ-", "bA3 wA2/", "wA3 wG1\\", "bA2 -wG1", "wB2 bQ",
    "bG3 bG2-", "wG3 wM-", "bA3 bB1-", "wB2 bB1", "bG3 bA1-", "wA2 bQ-",
    "bL bM/", "wB2 wL\\",
  },
  {  // https://entomology.gitlab.io/hive.html?game=bsn:T!HV-94FL-daallo-2023-04-01-1708
    "wP", "bG1 /wP", "wQ \\wP", "bQ bG1\\", "wS1 wP-", "bA1 -bG1", "wS1 bQ\\",
    "bB1 bA1\\", "wA1 wP-", "bA1 wA1-", "wA2 wQ-", "bM -bG1", "wA2 /bB1",
    "bM bG1", "wB1 -wQ", "bB2 -bM", "wL wQ-", "bM wP", "wB2 wS1-", "bB2 bG1",
    "wB2 bQ-", "bA2 -bB2", "wL bB2-", "bA2 /wA2", "wM wB2-", "bB2 wL",
    "wA3 wS1-", "bG1 wQ-", "wM bB1/", "bA2 wA2\\", "wA3 -wM", "bB2 wM",
    "wG1 wS1-", "bA1 wG1-", "wG2 -wA2", "bA2 /wG2", "wA1 wA2-",
  },
  {  // https://entomology.gitlab.io/hive.html?game=bsn:T!HV-Marko60-D0D0-2023-04-01-1357
    "wP", "bM wP\\", "wQ \\wP", "bQ bM-", "wA1 wQ/", "bP bQ-", "wM wQ-",
    "bA1 bP/", "wA2 -wA1", "bA2 bP\\", "wA2 bA1/", "bS1 bQ/", "wA1 bA2\\",
    "bA3 /bM", "wA3 wQ/", "bA3 wA3/", "wL wA1\\", "bS2 bA3-", "wG1 -wA3",
    "bS2 wG1/", "wL bP-", "bS1 wA2/", "wM bQ\\", "bG1 bS1/", "wA3 bA3-",
    "bB1 bQ/", "wB1 wQ-", "bB1 wP-", "wB1 bB1", "bM wB1", "wB2 /wP", "wM bM-",
    "wL /bQ", "bL bP-", "wM bL-", "bB2 bM-", "wB2 wP", "bM wB2", "wB1 bB2",
    "bG2 /bM", "wS1 wA1\\", "bG1 wL-", "wG2 -wG1", "bG1 bG2\\", "wS1 wL-",
    "bB1 wQ-", "wB1 bM-", "bB2 wB1", "wG2 wG1-", "bQ bB2-", "wM bQ/",
    "bL wA3\\", "wL bP-", "bG1 -bM", "wG2 bB2\\", "bS1 wL-", "wA3 bA1-",
    "bG2 -wQ", "wL bB1-",
  },
  // Edge cases from https://github.com/edre/nokamute/blob/master/data/uhp_tests.txt
  {  // Queen moves
    "wG1", "bG1 wG1-", "wQ \\wG1", "bQ bG1-", "wG2 /wG1", "bA1 bQ/",
    "wG3 /wG2", "bA1 bQ-", "wB1 -wG3", "bA1 bQ/", "wB2 \\wB1", "bA1 bQ-",
    "wS1 \\wB2", "bA1 bQ/", "wS2 wS1/", "bA1 bQ-", "wA1 \\wS2", "bA1 bQ/",
    "wA2 wA1/", "bA1 bQ-", "wA3 wA2/", "bA1 wA3-", "wQ \\bG1", "wQ -wG1",
  }, // wQ -wG1
  {  // Grasshopper moves
    "wG1", "bG1 wG1-", "wQ /wG1", "bQ bG1-", "wS1 wQ\\", "bA1 bQ-", "wB1 /wS1",
    "bA1 -wQ", "wB1 wS1\\", "bA2 bQ-", "wB1 /wS1", "bA2 wG1\\", "wB1 wS1\\",
    "bA3 bQ-", "wB1 /wS1", "bS1 bQ\\", "wB1 wS1", "bS1 wB1\\", "wB1 /wB1",
    "bA3 -wB1", "wG1 bQ-", "wG1 bA2\\", "wG1 bA1\\", "wG2 \\wG1", "wS2 \\wG1",
    "wA1 \\wG1", "wB2 \\wG1",
  }, // wB1 /wB1
  {  // Ant moves
    "wS1", "bB1 wS1-", "wQ -wS1", "bQ bB1-", "wB1 \\wQ", "bG1 bQ/", "wB2 \\wB1",
    "bG2 bG1/", "wS2 \\wB2", "bS1 bG2/", "wA1 \\wS1", "bB2 bS1/", "wA2 \\wS2",
    "bG3 \\bB2", "wA1 -bG1", "bA1 \\bG3", "wG1 wA2/", "bS2 -bA1", "wG2 wG1/",
    "bA2 -bS2", "wA3 wG2-", "bA3 bS2\\", "wG3 wA3\\", "bA3 wG3\\", "wA1 -bG2",
    "wA1 -bS1", "wA1 /bG3", "wA1 bS2\\", "wA1 bA2\\", "wA1 /bA2", "wA1 bA3-",
    "wA1 bA3\\", "wA1 /bA3", "wA1 /wG3", "wA1 wG2\\", "wA1 wG1\\", "wA1 wB2/",
    "wA1 wB1/", "wA1 \\wS1", "wA1 \\bB1",
  }, // wA1 -bS1
  {  // Spider moves
    "wG1", "bA1 wG1-", "wS1 \\wG1", "bQ bA1-", "wQ /wG1", "bG1 bQ\\",
    "wG2 wQ\\", "bB1 /bG1", "wB1 /wG2", "bG2 bG1\\", "wG3 /wB1", "bG2 -bB1",
    "wB2 wG3\\", "bA1 bG1\\", "wA1 wB2-", "bA1 bB1\\", "wA2 wA1/", "bA1 bG1-",
    "wS2 wA2-", "bA1 bG1\\", "wA3 wS2\\", "bA1 wA3-", "wS1 \\bQ", "wS1 /bQ",
    "wS1 wG1\\", "wS1 /wQ",
  },
  {  // Spider moves 2
    "wG1", "bA1 wG1/", "wB1 /wG1", "bA2 bA1-", "wQ wB1\\", "bQ bA2\\",
    "wB2 /wQ", "bG1 bQ\\", "wS1 wG1\\", "bB1 /bG1", "wG2 /wB2", "bG2 bG1\\",
    "wG3 wG2\\", "bG2 wS1\\", "wA1 wG3-", "bA1 -wB1", "wS2 wA1/", "bA3 bG1\\",
    "wA2 wS2-", "bA2 \\wG1", "wA3 wA2\\", "bA3 wA3-", "wS1 bA2/", "wS1 bQ/",
    "wS1 wG1/", "wS1 \\bQ",
  },
  {  // Beetle moves
    "wB1", "bB1 wB1-", "wQ \\wB1", "bQ bB1/", "wG1 /wB1", "bB2 bB1\\",
    "wA1 /wG1", "bA1 bQ\\", "wG2 -wA1", "bQ \\bB1", "wB2 /wG2", "bA2 \\bA1",
    "wG3 wB2\\", "bA2 \\wQ", "wA2 wG3-", "bB2 wB1\\", "wS1 wA2\\", "bA1 bB1\\",
    "wS2 wS1-", "bA1 bB1-", "wA3 wS2/", "bA1 \\wA3", "wB1 wQ", "wB1 bQ",
    "wB1 bB1", "wB1 bB2", "wB1 wG1",
  },
  {  // Mosquito moves
    "wM", "bG1 wM-", "wS1 /wM", "bQ bG1-", "wQ /wS1", "bB1 bG1\\", "wB1 /wQ",
    "bB1 wM\\", "wS2 /wB1", "bA1 bQ-", "wB2 wS2\\", "bA1 bQ\\", "wG1 wB2-",
    "bA1 bQ-", "wG2 wG1/", "bA1 bQ\\", "wG3 wG2/", "bA1 bQ-", "wA1 wG3-",
    "bA1 bQ/", "wA2 wA1-", "bA1 bQ-", "wA3 wA2\\", "bA1 /wA3", "wM bQ-",\
    "wM bB1\\", "wM /wS2", "wM \\bG1", "wM bG1", "wM bB1", "wM wS1",
    "wM \\wS1", "wM bQ/", "wM -wQ",
  },
  {  // Ladybug moves
    "wL", "bL wL/", "wQ -wL", "bQ bL/", "wQ -bL", "bA1 bQ/", "wB1 \\wQ",
    "bA1 bQ-", "wS1 \\wB1", "bA1 bQ/", "wB2 \\wS1", "bA1 bQ-", "wS2 wB2/",
    "bA1 bQ/", "wA1 wS2-", "bA1 bQ-", "wG1 wA1/", "bA1 bQ/", "wG2 wG1-",
    "bA1 bQ-", "wA2 wG2\\", "bA1 bQ/", "wA3 wA2-", "bA1 bQ-", "wG3 wA3/",
    "bA1 \\wG3", "wL wB1/", "wL -bQ", "wL /wB1", "wL /wS1", "wL bQ\\",
    "wL bL\\", "wL \\bQ", "wL bQ/", "wL bQ-", "wL /wQ",
  },
  {  // Pillbug can't throw last move.
    "wP", "bS1 wP-", "wQ /wP", "bQ bS1-", "wB1 -wQ", "bB1 bS1\\", "wG1 wB1\\",
    "bB1 wP\\", "wS1 wG1\\", "bQ bS1/", "wB1 -wP", "bB1 wQ", "wG2 wS1\\",
    "bB1 wB1", "wG3 wG2\\", "bA1 bQ\\", "wS2 wG3-", "bA1 bS1\\", "wA1 wS2/",
    "bA1 bQ\\", "wA2 wA1/", "bA1 bS1\\", "wA3 wA2/", "bA1 bQ\\", "wB2 wA3/",
    "bA1 wB2/", "pass", "bQ \\bS1", "bS1 -bQ", "bS1 wP\\",
  },
  {  // White wins
    "wS1", "bS1 wS1-", "wQ -wS1", "bQ bS1/", "wG1 -wQ", "bG1 \\bQ", "wG1 bQ\\",
    "bG2 bQ/", "wA1 wQ\\", "bA1 bG2/", "wA1 bG2\\", "bA1 \\bG2", "wQ \\wS1",
    "bA1 bG2/", "wQ /bG1",
  },
  {  // Black suicide
    "wS1", "bS1 wS1-", "wQ -wS1", "bQ bS1/", "wG1 -wQ", "bG1 \\bQ", "wG1 bQ\\",
    "bG2 bQ/", "wA1 wQ\\", "bA1 bG2/", "wA1 bG2\\", "bA1 /bG1",
  },
  {  // Draw
    "wS1", "bS1 wS1-", "wQ -wS1", "bQ bS1/", "wG1 -wQ", "bG1 \\bQ", "wG1 bQ\\",
    "bG2 bQ/", "wA1 wQ\\", "bA1 bG2/", "wA1 bG2\\", "bA1 \\bG2", "wQ \\wS1",
    "bG1 wA1/", "wQ -bQ", "bA1 \\wQ", "wB1 -wS1", "bG3 bG1-", "wB1 /bA1",
    "bG3 -bG2",
  },
  {  // Pass
    "wA1", "bS1 wA1-", "wQ -wA1", "bQ bS1/", "wQ \\wA1", "bA1 bS1\\",
    "wQ -wA1", "bA2 bQ\\", "wQ \\wA1", "bA1 \\wQ", "wG1 /wQ", "bA2 /wG1", "pass",
  },
  // Weird stuff:
  {  // Super long hive axis-1
    "wG1", "bG1 wG1-", "wQ -wG1", "bQ bG1-", "wS1 -wQ", "bS1 bQ-", "wS2 -wS1",
    "bS2 bS1-", "wB1 -wS2", "bB1 bS2-", "wB2 -wB1", "bB2 bB1-", "wG2 -wB2",
    "bG2 bB2-", "wG3 -wG2", "bG3 bG2-", "wA1 -wG3", "bA1 bG3-", "wA2 -wA1",
    "bA2 bA1-", "wA3 -wA2", "bA3 bA2-", "wA3 \\wA2", "wA3 \\wA1", "wA3 \\wG3",
    "wA3 \\wG2", "wA3 \\wB2", "wA3 \\wB1", "wA3 \\wS2", "wA3 \\wS1",
    "wA3 \\wQ", "wA3 \\wG1", "wA3 \\bG1", "wA3 \\bQ", "wA3 \\bS1", "wA3 \\bS2",
    "wA3 \\bB1", "wA3 \\bB2", "wA3 \\bG2", "wA3 \\bG3", "wA3 \\bA1",
    "wA3 \\bA2", "wA3 \\bA3", "wA3 bA3/", "wA3 bA3-", "wA3 bA3\\", "wA3 bA2\\",
    "wA3 bA1\\", "wA3 bG3\\", "wA3 bG2\\", "wA3 bB2\\", "wA3 bB1\\",
    "wA3 bS2\\", "wA3 bS1\\", "wA3 bQ\\", "wA3 bG1\\", "wA3 wG1\\", "wA3 wQ\\",
    "wA3 wS1\\", "wA3 wS2\\", "wA3 wB1\\", "wA3 wB2\\", "wA3 wG2\\",
    "wA3 wG3\\", "wA3 wA1\\", "wA3 wA2\\", "wA3 /wA2",
  },
  {  // Super long hive axis-2
    "wM", "bM wM\\", "wL \\wM", "bL bM\\", "wP \\wL", "bP bL\\", "wQ \\wP",
    "bQ bP\\", "wS1 \\wQ", "bS1 bQ\\", "wS2 \\wS1", "bS2 bS1\\", "wB1 \\wS2",
    "bB1 bS2\\", "wB2 \\wB1", "bB2 bB1\\", "wG1 \\wB2", "bG1 bB2\\",
    "wG2 \\wG1", "bG2 bG1\\", "wG3 \\wG2", "bG3 bG2\\", "wA1 \\wG3",
    "bA1 bG3\\", "wA2 \\wA1", "bA2 bA1\\", "wA3 \\wA2", "bA3 bA2\\",
    "wA3 wA2/", "wA3 wA1/", "wA3 wG3/", "wA3 wG2/", "wA3 wG1/", "wA3 wB2/",
    "wA3 wB1/", "wA3 wS2/", "wA3 wS1/", "wA3 wQ/", "wA3 wP/", "wA3 wL/",
    "wA3 wM/", "wA3 bM/", "wA3 bL/", "wA3 bP/", "wA3 bQ/", "wA3 bS1/",
    "wA3 bS2/", "wA3 bB1/", "wA3 bB2/", "wA3 bG1/", "wA3 bG2/", "wA3 bG3/",
    "wA3 bA1/", "wA3 bA2/", "wA3 bA3/", "wA3 bA3-", "wA3 bA3\\", "wA3 /bA3",
    "wA3 /bA2", "wA3 /bA1", "wA3 /bG3", "wA3 /bG2", "wA3 /bG1", "wA3 /bB2",
    "wA3 /bB1", "wA3 /bS2", "wA3 /bS1", "wA3 /bQ", "wA3 /bP", "wA3 /bL",
    "wA3 /bM", "wA3 /wM", "wA3 /wL", "wA3 /wP", "wA3 /wQ", "wA3 /wS1",
    "wA3 /wS2", "wA3 /wB1", "wA3 /wB2", "wA3 /wG1", "wA3 /wG2", "wA3 /wG3",
    "wA3 /wA1", "wA3 /wA2", "wA3 -wA2",
  },
  {  // Super long hive axis-3
    "wM", "bM wM/", "wL /wM", "bL bM/", "wP /wL", "bP bL/", "wQ /wP", "bQ bP/",
    "wS1 /wQ", "bS1 bQ/", "wS2 /wS1", "bS2 bS1/", "wB1 /wS2", "bB1 bS2/",
    "wB2 /wB1", "bB2 bB1/", "wG1 /wB2", "bG1 bB2/", "wG2 /wG1", "bG2 bG1/",
    "wG3 /wG2", "bG3 bG2/", "wA1 /wG3", "bA1 bG3/", "wA2 /wA1", "bA2 bA1/",
    "wA3 /wA2", "bA3 bA2/", "wA3 -wA2", "wA3 -wA1", "wA3 -wG3", "wA3 -wG2",
    "wA3 -wG1", "wA3 -wB2", "wA3 -wB1", "wA3 -wS2", "wA3 -wS1", "wA3 -wQ",
    "wA3 -wP", "wA3 -wL", "wA3 -wM", "wA3 -bM", "wA3 -bL", "wA3 -bP",
    "wA3 -bQ", "wA3 -bS1", "wA3 -bS2", "wA3 -bB1", "wA3 -bB2", "wA3 -bG1",
    "wA3 -bG2", "wA3 -bG3", "wA3 -bA1", "wA3 -bA2", "wA3 -bA3", "wA3 \\bA3",
    "wA3 bA3/", "wA3 bA3-", "wA3 bA3\\", "wA3 bA2\\", "wA3 bA1\\", "wA3 bG3\\",
    "wA3 bG2\\", "wA3 bG1\\", "wA3 bB2\\", "wA3 bB1\\", "wA3 bS2\\",
    "wA3 bS1\\", "wA3 bQ\\", "wA3 bP\\", "wA3 bL\\", "wA3 bM\\", "wA3 wM\\",
    "wA3 wL\\", "wA3 wP\\", "wA3 wQ\\", "wA3 wS1\\", "wA3 wS2\\", "wA3 wB1\\",
    "wA3 wB2\\", "wA3 wG1\\", "wA3 wG2\\", "wA3 wG3\\", "wA3 wA1\\",
    "wA3 wA2\\",
  },
  {  // Draw by threefold repetition
    "wS1", "bB1 wS1-", "wQ \\wS1", "bQ bB1\\", "wQ -wS1", "bQ bB1-",
    "wQ \\wS1", "bQ bB1\\", "wQ -wS1", "bQ bB1-", "wQ \\wS1",
    "bQ bB1\\",
  },
  {  // Beetle gate
    "wB1", "bS1 wB1-", "wQ \\wB1", "bQ bS1/", "wB2 -wQ", "bB1 bQ\\", "wS1 /wB2",
    "bB1 bS1", "wG1 /wS1", "bQ \\bB1", "wG2 wG1\\", "bB2 bQ/", "wG3 wG2\\",
    "bB2 \\bQ", "wA1 wG3-", "bB2 wQ", "wA2 wA1-", "bA1 bQ/", "wS2 wA2-",
    "bA1 bB1/", "wA3 wS2/", "bA1 wA3/", "wB1 bB2", "wB1 bB1", "wB1 /bB1",
    "wB1 wB2\\",
  },
  {  // Ladybug gate
    "wL", "bG1 wL/", "wQ -wL", "bQ bG1/", "wQ -bG1", "bG2 bQ-", "wB1 \\wQ",
    "bB1 bG2-", "wS1 \\wB1", "bB1 bG2", "wS2 \\wS1", "bG3 \\bQ", "wG1 wS2/",
    "bB2 bG3/", "wB2 wG1/", "bB2 bG3", "wA1 wB2-", "bA1 bB1-", "wA2 wA1-",
    "bA1 bB1\\", "wG2 wA2-", "bA1 bB1-", "wG3 wG2\\", "bA1 bB1\\", "wA3 wG3\\",
    "bA1 wA3\\", "wL -bB2", "wL /bB2", "wL /wB1", "wL /wS1", "wL bQ\\",
    "wL bG1\\", "wL /wQ",
  },
  {  // Pillbug gate
    "wP", "bB1 wP-", "wQ /wP", "bQ bB1/", "wQ wP\\", "bQ \\bB1", "wQ /wP",
    "bA1 bQ/", "wQ wP\\", "bA1 -bQ", "wQ /wP", "bB2 \\bQ", "wQ wP\\",
    "bB2 bQ", "bA1 -wP", "bB1 wQ", "bA1 -bB2", "bA1 /wP",
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
    // Human games can use up to six strings to refer to the same move
    // we must allow this in our validation
    auto game = LoadGame("hive(all_action_reprs=true)");
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
  open_spiel::hive::HumanGamesTests();
  open_spiel::hive::BasicHiveTests();
}
