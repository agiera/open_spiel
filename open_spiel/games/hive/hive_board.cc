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

#include "open_spiel/games/hive/hive_board.h"

#include <vector>
#include <iomanip>

#include "open_spiel/abseil-cpp/absl/random/uniform_int_distribution.h"
#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/games/chess/chess_common.h"
#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace hive {

static inline Hexagon kEmptyHexagon = Hexagon(0, 0, 0);

absl::optional<BugType> BugTypeFromChar(char c) {
  return BugType::kBee;
}

std::string BugTypeToString(BugType t, bool uppercase) {
  return "bug";
}

std::string Bug::ToString() const {
  std::string base = BugTypeToString(type);
  return color == Color::kWhite ? absl::AsciiStrToUpper(base)
                                : absl::AsciiStrToLower(base);
}

// 6 adjacent hexagons
//    0
//   5 1
//
//   4 2
//    3

// A hexagon's ith neighbour is it's (i + 3 mod 5)ths neighbour
const std::vector<int8_t> neighbour_inverse = {3, 4, 5, 0, 1, 2};

const std::array<Offset, 6> evenColumnNeighbors = {
  Offset{0, -1, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {-1, 1, 0}, {-1, 0, 0},
};

const std::array<Offset, 6> oddColumnNeighbors = {
  Offset{0, -1, 0}, {1, -1, 0}, {0, 1, 0}, {1, 0, 0}, {-1, 0, 0}, {-1, -1, 0},
};

std::ostream& operator<<(std::ostream& os, Bug b) {
  return os << BugToString(b);
}

std::ostream& operator<<(std::ostream& os, Hexagon h) {
  return os << HexagonToString(h);
}

std::string HexagonToString(Hexagon h) {
  return absl::StrCat("(", h.loc.x + 1, ", ", h.loc.y + 1, ", ", h.loc.z + 1, ")");
}

Hexagon::Hexagon() {
  loc.x = 0;
  loc.y = 0;
  loc.z = 0;
  bug = absl::nullopt;
  visited = false;
  neighbours.fill(NULL);
}

Hexagon::Hexagon(int8_t x, int8_t y, int8_t z) {
  loc.x = x;
  loc.y = y;
  loc.z = z;
  bug = absl::nullopt;
  visited = false;
  neighbours.fill(NULL);
}

BugCollection::BugCollection(Color c) {
  color_ = c;
  bug_counts_ = bug_counts;
}

bool BugCollection::HasBug(BugType t) const {
  return !bug_counts_[t] == 0;
}

Bug BugCollection::UseBug(BugType t) {
  if (HasBug(t)) {
    bug_counts_[t]--;
    return Bug{color_, t};
  }
}

HiveBoard::HiveBoard(int board_size) :
  board_size_(board_size),
  board_height_(kBoardHeight),
  bug_collections_ ({ BugCollection(Color::kBlack), BugCollection(Color::kWhite) })
{
  Clear();
}

void HiveBoard::Clear() {  
  for (Hexagon* h : hexagons_) {
    bug_collections_[h->bug.value().color].ReturnBug(*h);
  }

  hexagons_.clear();

  zobrist_hash_ = 0;
  std::fill(begin(board_), end(board_), NULL);
  std::fill(begin(observation_), end(observation_), 0);
  std::fill(begin(visited_), end(visited_), false);
}

int HiveBoard::OffsetToIndex(Offset o) const {
  return o.x + kBoardSize*o.y + kBoardSize*kBoardSize*o.z;
}

absl::optional<Color> HiveBoard::HexagonOwner(Hexagon& h) const {
  if (h.bug == absl::nullopt) { return absl::nullopt; }
  absl::optional<Color> c = absl::nullopt;
  for (Hexagon* n : h.neighbours) {
    if (n->bug.has_value()) {
      c = n->bug.value().color;
      if (c.value() != n->bug.value().color) {
        return absl::nullopt;
      }
    }
  }
  return c;
}

void HiveBoard::CacheHexagonOwner(Hexagon& h) {
  absl::optional<Color> c = HexagonOwner(h);
  if (c.has_value()) {
    available_[c.value()].push_back(h.boardIndex);
  } else {
    // TODO: available_.remove(h.boardIndex);
  }
}

Bug HiveBoard::TakeBug(Hexagon& h) {
  if (!h.bug.has_value()) { return; }
  Bug b = h.bug.value();
  CacheHexagonOwner(h);
  for (Hexagon* n : h.neighbours) {
    CacheHexagonOwner(*n);
  }
  return b;
}

void HiveBoard::PlayMove(HiveMove &m) {
  if (m.pass) return;
  Bug b;
  if (m.place) {
    b = bug_collections_[to_play_].UseBug(m.bug);
  } else {
    // TODO: Remove bug from board and set to h
    Hexagon* h = GetHexagon(m.from);
    b = TakeBug(*h);
  }
  Hexagon* h = GetHexagon(m.to);
  h->bug = b;
  CacheHexagonOwner(*h);
  for (Hexagon* n : h->neighbours) {
    CacheHexagonOwner(*n);
  }
}

/*
bool HiveBoard::IsLegalMove(HiveMove m) const {
  if (m.pass) return true;
  if (!m.place && m.from_hex.isInvalid()) return false;
  if (m.to_hex.isInvalid()) return false;

  if (m.place && bug_collections_[to_play_].HasBug(m.bug)) return false;

  // We're better off iterating over legal moves than trying to find them all
  // TODO
  //bool found = false;
  //GenerateLegalMoves([&found, &tested_move](const Move& found_move) {
  //  if (tested_move == found_move) {
  //    found = true;
  //    return false;  // We don't need any more moves.
  //  }
  //  return true;
  //});
  //return found;
}
*/

std::string HiveBoard::ToString() {
  std::ostringstream stream;
  stream << *this;
  return stream.str();
}

std::ostream& operator<<(std::ostream& os, const HiveBoard& board) {
  return os;
}

HiveBoard BoardFromFEN(const std::string& fen) {
  HiveBoard board(kBoardSize);
}

}  // namespace hive
}  // namespace open_spiel
