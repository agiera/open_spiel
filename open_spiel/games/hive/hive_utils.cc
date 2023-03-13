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

Offset::Offset(int boardIdx) {
  boardIdx = mod(boardIdx, kBoardSize*kBoardSize*kBoardHeight);
  z = boardIdx / (kBoardSize*kBoardSize);
  boardIdx = mod(boardIdx, kBoardSize*kBoardSize);
  y = boardIdx / kBoardSize;
  x = mod(boardIdx, kBoardSize);
  index = x + kBoardSize*y + kBoardSize*kBoardSize*z;
}

const std::array<Offset, 6> evenRowNeighbors = {
  Offset(-1, -1, 0), Offset(0, -1, 0), Offset(1, 0, 0),
  Offset(0, 1, 0), Offset(-1, 1, 0), Offset(-1, 0, 0),
};

const std::array<Offset, 6> oddRowNeighbors = {
  Offset(0, -1, 0), Offset(1, -1, 0), Offset(1, 0, 0),
  Offset(1, 1, 0), Offset(0, 1, 0), Offset(-1, 0, 0),
};

std::array<Offset, 6> Offset::neighbours() const {
  auto f = [this](Offset p) -> Offset { return p + *this; };
  std::array<Offset, 6> neighbours;
  if (mod(y, 2) == 0) {
    neighbours = evenRowNeighbors;
  } else {
    neighbours = oddRowNeighbors;
  }
  std::transform(neighbours.begin(), neighbours.end(), neighbours.begin(), f);
  return neighbours;
}

Offset Offset::operator+(const Offset& other) const {
  return Offset(x + other.x, y + other.y, z + other.z);
}

bool Offset::operator==(const Offset& other) const {
  return x == other.x && y == other.y && z == other.z;
}

bool Offset::operator!=(const Offset& other) const {
  return !operator==(other);
}

Offset neighbourOffset(Offset o, int i) {
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
  return absl::StrCat("(", o.x, ", ", o.y, ", ", o.z, ")");
}

std::ostream& operator<<(std::ostream& os, Hexagon h) {
  return os << HexagonToString(h);
}

std::string HexagonToString(Hexagon h) {
  return OffsetToString(h.loc);
}

Hexagon::Hexagon() {
  loc.x = 0;
  loc.y = 0;
  loc.z = 0;
  bug = kEmptyBug;
  above = -1;
  below = -1;
  neighbours.fill(-1);
}

BugCollection::BugCollection(Player p)
  : player_(p), bug_counts_({ 0, 0, 0, 0, 0, 0, 0, 0 }) {}

void BugCollection::Reset() {
  bug_counts_ = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (std::vector<int> h_vec : hexagons_) {
    h_vec.clear();
  }
}

bool BugCollection::HasBug(BugType t) const {
  return !(bug_counts_[t] == bug_counts[t]);
}

void BugCollection::UseBug(Hexagon* h, BugType t) {
  SPIEL_DCHECK_EQ(h->bug.order, -1);
  SPIEL_DCHECK_NE(bug_counts_[t], bug_counts[t]);
  hexagons_[t].push_back(h->loc.index);
  Bug bug = Bug{player_, t, bug_counts_[t]};
  h->bug = bug;
  bug_counts_[t]++;
}

void BugCollection::MoveBug(Hexagon* from, Hexagon* to) {
  SPIEL_DCHECK_NE(from->bug.order, -1);
  SPIEL_DCHECK_EQ(to->bug.order, -1);
  to->bug = from->bug;
  from->bug = kEmptyBug;
  hexagons_[to->bug.type][to->bug.order] = to->loc.index;
}

void BugCollection::ReturnBug(Hexagon* h) {
  SPIEL_DCHECK_NE(h->bug.order, -1);
  bug_counts_[h->bug.type]--;
  hexagons_[h->bug.type].pop_back();
  h->bug = kEmptyBug;
}

absl::optional<Offset> BugCollection::GetBug(Bug b) const {
  if (b.order >= bug_counts_[b.type]) {
    std::cout << "no bug found in collection\n";
    return absl::nullopt;
  }
  return hexagons_[b.type][b.order];
}

int8_t BugCollection::NumBugs(BugType bt) const {
  return bug_counts_[bt];
}

}  // namespace hive
}  // namespace open_spiel
