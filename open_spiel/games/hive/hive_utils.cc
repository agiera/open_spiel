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

#include "open_spiel/games/hive/hive_utils.h"

namespace open_spiel {
namespace hive {

Bug::Bug(BugIdx b) {
  player = b / (kNumBugs / 2);
  BugIdx d = b - player * (kNumBugs / 2);
  uint8_t t =  0;
  while (t < kNumBugTypes - 1 && d >= bug_series[t + 1]) t++;
  type = (BugType) t;
  order = d - bug_series[t];

  SPIEL_DCHECK_GE(player, 0);
  SPIEL_DCHECK_LE(player, 1);
  SPIEL_DCHECK_GE(type, 0);
  SPIEL_DCHECK_LE(type, 7);
  SPIEL_DCHECK_GE(order, -1);
  SPIEL_DCHECK_LE(order, 2);

  idx = player*kNumBugs/2 + bug_series[type] + order;
}

// TODO
absl::optional<BugType> BugTypeFromChar(char c) {
  return BugType::kBee;
}

std::string Bug::ToString() const {
  SPIEL_DCHECK_GE(player, 0);
  SPIEL_DCHECK_LE(player, 1);
  SPIEL_DCHECK_GE(type, 0);
  SPIEL_DCHECK_LE(type, 7);
  SPIEL_DCHECK_GE(order, -1);
  SPIEL_DCHECK_LE(order, 2);

  if (order == -1) { return "kEmptyBug"; }

  std::string res;
  res += kPlayerChars[player];
  res += kBugTypeChars[type];
  if (bug_counts[type] > 1) {
    res += std::to_string(order + 1);
  }
  return res;
}

// 6 adjacent hexagons
//   0 1
//  5   2
//   4 3

// A hexagon's ith neighbour is the neighbour's (i + 3 mod 5)ths neighbour
const std::vector<int8_t> neighbour_inverse = {3, 4, 5, 0, 1, 2};

const std::array<Offset, 6> evenRowNeighbors = {
  Offset(-1, -1), Offset(0, -1), Offset(1, 0),
  Offset(0, 1), Offset(-1, 1), Offset(-1, 0),
};

const std::array<Offset, 6> oddRowNeighbors = {
  Offset(0, -1), Offset(1, -1), Offset(1, 0),
  Offset(1, 1), Offset(0, 1), Offset(-1, 0),
};

Offset::Offset(uint8_t x, uint8_t y) {
  y = idx / kBoardSize;
  x = mod(idx, kBoardSize);

  if (mod(y, 2) == 0) {
    for (int i=0; i < 6; i++) {
      neighbours[i] =
        mod(idx + evenRowNeighbors[i].idx, kBoardSize*kBoardSize);
    }
  } else {
    for (int i=0; i < 6; i++) {
      neighbours[i] = mod(idx + oddRowNeighbors[i].idx, kBoardSize*kBoardSize);
    }
  }
}

Offset Offset::operator+(const Offset& other) const {
  return Offset(x + other.x, y + other.y);
}

bool Offset::operator==(const Offset& other) const {
  return x == other.x && y == other.y;
}

bool Offset::operator!=(const Offset& other) const {
  return !operator==(other);
}

Offset neighbourOffset(Offset o, uint8_t i) {
  if (mod(o.y, 2) == 0) {
    return o + evenRowNeighbors[i];
  }
  return o + oddRowNeighbors[i];
}

std::ostream& operator<<(std::ostream& os, Bug b) {
  return os << b.ToString();
}

std::string BugToString(Bug b) {
  return b.ToString();
}

std::string OffsetToString(Offset o) {
  return absl::StrCat("(", o.x, ", ", o.y, ")");
}

std::ostream& operator<<(std::ostream& os, Hexagon h) {
  return os << HexagonToString(h);
}

std::string HexagonToString(Hexagon h) {
  return OffsetToString(h.loc);
}

Hexagon::Hexagon() {
  bug = kEmptyBug;
}

BugCollection::BugCollection(Player p)
  : player_(p), bug_counts_({ 0, 0, 0, 0, 0, 0, 0, 0 }) {}

void BugCollection::Reset() {
  bug_counts_ = { 0, 0, 0, 0, 0, 0, 0, 0 };
}

bool BugCollection::HasBug(BugType t) const {
  return !(bug_counts_[t] == bug_counts[t]);
}

Bug BugCollection::UseBug(BugType t) {
  SPIEL_DCHECK_NE(bug_counts_[t], bug_counts[t]);
  Bug bug = Bug{player_, t, bug_counts_[t]};
  bug_counts_[t]++;
  return bug;
}

void BugCollection::ReturnBug(Hexagon* h) {
  SPIEL_DCHECK_NE(h->bug.order, -1);
  bug_counts_[h->bug.type]--;
  h->bug = kEmptyBug;
}

int8_t BugCollection::NumBugs(BugType bt) const {
  return bug_counts_[bt];
}

}  // namespace hive
}  // namespace open_spiel
