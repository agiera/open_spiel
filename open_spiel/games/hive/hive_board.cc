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
#include <stack>
#include <iomanip>

#include "open_spiel/abseil-cpp/absl/random/uniform_int_distribution.h"
#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/games/chess/chess_common.h"
#include "open_spiel/spiel_globals.h"
#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace hive {

absl::optional<BugType> BugTypeFromChar(char c) {
  return BugType::kBee;
}

std::string BugTypeToString(BugType t, bool uppercase) {
  return "bug";
}

std::string Bug::ToString() const {
  std::string base = BugTypeToString(type);
  return player == kWhite ? absl::AsciiStrToUpper(base)
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

Offset::Offset(int boardIdx) {
  boardIdx = boardIdx % (kBoardSize*kBoardSize*kBoardHeight);
  z = boardIdx / (kBoardSize*kBoardSize);
  boardIdx = boardIdx % (kBoardSize*kBoardSize);
  y = boardIdx / kBoardSize;
  x = boardIdx % kBoardSize;
  index = x + kBoardSize*y + kBoardSize*kBoardSize*z;
}

const std::array<Offset, 6> evenColumnNeighbors = {
  Offset(0, -1, 0), Offset(1, 0, 0), Offset(1, 1, 0),
  Offset(0, 1, 0), Offset(-1, 1, 0), Offset(-1, 0, 0),
};

const std::array<Offset, 6> oddColumnNeighbors = {
  Offset(0, -1, 0), Offset(1, -1, 0), Offset(0, 1, 0),
  Offset(1, 0, 0), Offset(-1, 0, 0), Offset(-1, -1, 0),
};

std::array<Offset, 6> Offset::neighbours() const {
  auto f = [this](Offset p) -> Offset { return p + *this; };
  std::array<Offset, 6> neighbours;
  if (x % 2 == 0) {
    neighbours = evenColumnNeighbors;
  } else {
    neighbours = oddColumnNeighbors;
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
  if (o.x % 2 == 0) {
    return o + evenColumnNeighbors[i];
  }
  return o + oddColumnNeighbors[i];
}


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
  above = NULL;
  below = NULL;
  neighbours.fill(NULL);
}

Hexagon* Hexagon::Bottom() const {
  Hexagon* h = below;
  while (h->loc.z != 0) {
    h = h->below;
  }
  return h;
}

Hexagon* Hexagon::Top() const {
  Hexagon* h = above;
  while (h->loc.z != kBoardHeight-1) {
    h = h->above;
  }
  return h;
}

int Hexagon::FindClockwiseMove(int prev_idx) const {
  int j;
  for (int8_t i=0; i < 6; i++) {
    j = (prev_idx + 1 - i) % 6;
    if (neighbours[j]->bug.has_value() &&
        neighbours[(j+1) % 6]->bug.has_value() &&
        neighbours[(j-1) % 6]->bug.has_value()) {
      return i;
    }
  }
  return -1;
}

int Hexagon::FindCounterClockwiseMove(int prev_idx) const {
  int j;
  for (int8_t i=0; i < 6; i++) {
    j = (prev_idx - 1 + i) % 6;
    if (neighbours[j]->bug.has_value() &&
        neighbours[(j-1) % 6]->bug.has_value() &&
        neighbours[(j+1) % 6]->bug.has_value()) {
      return i;
    }
  }
  return -1;
}

std::vector<Hexagon*> Hexagon::FindJumpMoves() const {
  std::vector<Hexagon*> jumps;
  Hexagon* b = Bottom();
  for (Hexagon* n : b->neighbours) {
    if (n->bug.has_value()) {
      // TODO: filter gated
      jumps.push_back(n->Top());
    }
  }
  return jumps;
}

void Hexagon::GenerateMoves(BugType t, std::vector<HiveMove> &moves) const {
  if (above != NULL) { return; }
  switch (t) {
    case kBee: GenerateBeeMoves(moves);
    case kBeetle: GenerateBeetleMoves(moves);
    case kAnt: GenerateAntMoves(moves);
    case kGrasshopper: GenerateGrasshopperMoves(moves);
    case kSpider: GenerateSpiderMoves(moves);
    case kLadybug: GenerateLadybugMoves(moves);
    case kMosquito: GenerateMosquitoMoves(moves);
    case kPillbug: GeneratePillbugMoves(moves);
    default: break;
  };
}

void Hexagon::GenerateBeeMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<int> beeMoves;
  beeMoves.insert(FindClockwiseMove(0));
  beeMoves.insert(FindClockwiseMove(3));
  beeMoves.insert(FindCounterClockwiseMove(0));
  beeMoves.insert(FindCounterClockwiseMove(3));
  beeMoves.erase(-1);
  for (int i : beeMoves) {
    moves.push_back(HiveMove{false, false, bug.value().type, loc, neighbours[i]->loc});
  }
}

void Hexagon::GenerateBeetleMoves(std::vector<HiveMove> &moves) const {
  GenerateBeeMoves(moves);
  for (Hexagon* h : FindJumpMoves()) {
    moves.push_back(HiveMove{false, false, bug.value().type, loc, h->loc});
  }
}

void Hexagon::GenerateAntMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<Offset> antMoves;

  int i = FindClockwiseMove(0);
  int init_idx = i;
  Hexagon* h = neighbours[i];
  Hexagon* init_h = h;
  while (i != -1 && (h != init_h || i != init_idx)) {
    antMoves.insert(h->loc);

    i = h->FindClockwiseMove(i);
    h = h->neighbours[i];
  }

  for (Offset o : antMoves) {
    moves.push_back(HiveMove{false, false, bug.value().type, loc, o});
  }
}

void Hexagon::GenerateGrasshopperMoves(std::vector<HiveMove> &moves) const {
  for (int8_t i=0; i < 6; i++) {
    if (neighbours[i]->bug.has_value()) {
      Hexagon* n = neighbours[i];
      while(n->neighbours[i]->bug.has_value()) {
        n = n->neighbours[i];
      }
      moves.push_back(HiveMove{false, false, bug.value().type, loc, neighbourOffset(n->loc, i)});
    }
  }
}

absl::optional<Offset> Hexagon::WalkThree(int i, bool clockwise) const {
  Hexagon* h;
  if (clockwise) {
    int i = FindClockwiseMove(i);
    h = neighbours[i];
    if (i == -1) { return absl::nullopt; }
    i = h->FindClockwiseMove(i);
    h = h->neighbours[i];
    i = FindClockwiseMove(i);
    h = h->neighbours[i];
  } else {
    int i = FindCounterClockwiseMove(i);
    h = neighbours[i];
    if (i == -1) { return absl::nullopt; }
    i = FindCounterClockwiseMove(i);
    h = h->neighbours[i];
    i = FindCounterClockwiseMove(i);
    h = h->neighbours[i];
    return i;
  }
  return h->loc;
}

void Hexagon::GenerateSpiderMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<absl::optional<Offset>> spiderMoves;
  spiderMoves.insert(WalkThree(0, true));
  spiderMoves.insert(WalkThree(3, true));
  spiderMoves.insert(WalkThree(0, false));
  spiderMoves.insert(WalkThree(3, false));
  spiderMoves.erase(absl::nullopt);
  for (absl::optional<Offset> o : spiderMoves) {
    moves.push_back(HiveMove{false, false, bug.value().type, loc, o.value()});
  }
}

void Hexagon::GenerateLadybugMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<Offset> ladybugMoves;
  for (Hexagon* n : FindJumpMoves()) {
    for (Hexagon* o : n->FindJumpMoves()) {
      for (Hexagon* p : o->neighbours) {
        if (!p->bug.has_value()) {
          ladybugMoves.insert(p->loc);
        }
      }
    }
  }
  for (Offset o : ladybugMoves) {
    moves.push_back(HiveMove{false, false, bug.value().type, loc, o});
  }
}

void Hexagon::GenerateMosquitoMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<BugType> mirrors;
  for (Hexagon* h : neighbours) {
    if (h != NULL && h->bug.has_value()) {
      mirrors.insert(h->bug.value().type);
    }
  }
  for (BugType t : mirrors) {
    GenerateMoves(t, moves);
  }
}

void Hexagon::GeneratePillbugMoves(std::vector<HiveMove> &moves) const {
  GenerateBeeMoves(moves);
  std::vector<Offset> emptyNeighbours;
  for (Hexagon* n : neighbours) {
    if (!n->bug.has_value()) {
      emptyNeighbours.push_back(n->loc);
    }
  }
  for (Hexagon * n : neighbours) {
    // TODO: fix
    // if (n->bug.has_value() && n != last_moved_) {
    if (n->bug.has_value()) {
      for (Offset o : emptyNeighbours) {
        moves.push_back(HiveMove{false, false, bug.value().type, n->loc, o});
      }
    }
  }
}

BugCollection::BugCollection() :
  bug_counts_({ 1, 2, 3, 3, 2, 1, 1, 1 }) {}

void BugCollection::Reset() {
  bug_counts_ = { 1, 2, 3, 3, 2, 1, 1, 1 };
}

void BugCollection::ReturnBug(BugType b) {
  bug_counts_[b]++;
}

bool BugCollection::HasBug(BugType t) const {
  return !bug_counts_[t] == 0;
}

bool BugCollection::UseBug(BugType t) {
  if (HasBug(t)) {
    bug_counts_[t]--;
    return true;
  }
  return false;
}

HiveBoard::HiveBoard() {
  Clear();

  // Initialize hexagon neighbours
  // TODO: make sure this is done at compile time
  for (int8_t x=0; x < kBoardSize; x++) {
    for (int8_t y=0; y < kBoardSize; y++) {
      for (int8_t z=0; z < kBoardHeight+1; z++) {
        Offset o = Offset(x, y, z);
        Hexagon* h = GetHexagon(o);
        h->loc = o;
        std::array<Offset, 6> neighbours = o.neighbours();
        for (int n=0; n < 6; n++) {
          h->neighbours[n] = &board_[neighbours[n].index];
        }
        h->above = GetHexagon(h->loc.x, h->loc.y, h->loc.z + 1);
        h->below = GetHexagon(h->loc.x, h->loc.y, h->loc.z - 1);
      }
    }
  }
}

void HiveBoard::Clear() {  
  hexagons_.empty();
  bees_ = {NULL, NULL};
  outcome = kInvalidPlayer;
  is_terminal = false;

  bug_collections_[kBlack].Reset();
  bug_collections_[kWhite].Reset();

  zobrist_hash = 0;
  std::fill(begin(observation), end(observation), 0);

  std::fill(begin(visited_), end(visited_), false);
}

Hexagon* HiveBoard::GetHexagon(Offset o) {
  return &board_[o.index];
}

Hexagon* HiveBoard::GetHexagon(int8_t x, int8_t y, int8_t z) {
  return &board_[Offset(x, y, z).index];
}

Player HiveBoard::HexagonOwner(Hexagon* h) const {
  Player p = kInvalidPlayer;
  for (Hexagon* n : h->neighbours) {
    if (n->bug.has_value()) {
      p = n->bug.value().player;
      if (p != n->bug.value().player) {
        return kInvalidPlayer;
      }
    }
  }
  return p;
}

void HiveBoard::CacheHexagonOwner(Hexagon* h) {
  Player p = HexagonOwner(h);
  if (p != kInvalidPlayer) {
    available_[p].insert(h->loc);
  } else {
    available_[kBlack].erase(h->loc);
    available_[kWhite].erase(h->loc);
  }
}

void HiveBoard::CacheUnpinnedHexagons() {
  // TODO: there might be a way to update val and min such that we don't have to redo this
  // Or maybe it can only be avoided in some circumstances
  unpinned_.empty();
  if (hexagons_.empty()) { return; }
  std::stack<Hexagon*> preorderStack;
  std::stack<Hexagon*> postorderStack;
  preorderStack.push(*hexagons_.begin());
  int num = 0;
  // Pre-order traversal
  // Resets num and low for all nodes
  // Adds all nodes to unpinned_ as candidates
  while (!preorderStack.empty()) {
    Hexagon* curNode = preorderStack.top();
    preorderStack.pop();
    postorderStack.push(curNode);
    unpinned_.insert(curNode);
    curNode->visited = true;
    curNode->num = num++;
    curNode->low = curNode->num;
    for (Hexagon* n : curNode->neighbours) {
      if (!n->visited) {
        preorderStack.push(n);
      }
    }
  }
  // Post-order traversal
  // Calculates each node's distance to the root
  // Resets visited attribute
  // Removes articulation nodes from unpinned_
  while(!postorderStack.empty()) {
    Hexagon* curNode = postorderStack.top();
    postorderStack.pop();
    curNode->visited = false;
    for (Hexagon* n : curNode->neighbours) {
      // visited acts as an inverse here because of the fist traversal
      if (n->visited) {
        if (n->low >= curNode->num) {
          unpinned_.erase(curNode);
        }
        n->low = std::min(n->low, curNode->num);
      // Back edge
      } else {
        curNode->low = std::min(curNode->low, n->num);
      }
    }
  }
}

absl::optional<Bug> HiveBoard::RemoveBug(Hexagon* h) {
  if (!h->bug.has_value()) { return h->bug; }
  Bug b = h->bug.value();
  h->bug = absl::nullopt;
  hexagons_.erase(h);
  if (b.type == kBee) {
    bees_[b.player] = NULL;
  }
  bug_collections_[b.player].ReturnBug(b.type);
  zobrist_hash ^= zobristTable[b.player][b.type][h->loc.x][h->loc.y][h->loc.z];
  CacheHexagonOwner(h);
  for (Hexagon* n : h->neighbours) {
    CacheHexagonOwner(n);
  }
  return b;
}

void HiveBoard::PlaceBug(Offset o, BugType b) {
  if (!bug_collections_[to_play].UseBug(b)) {
    return;
  }
  zobrist_hash ^= zobristTable[to_play][b][o.x][o.y][o.z];
  Hexagon* h = GetHexagon(o);
  h->bug = Bug{to_play, b};
  hexagons_.insert(h);
  if (b == kBee) {
    bees_[to_play] = h;
  }
  CacheHexagonOwner(h);
  for (Hexagon* n : h->neighbours) {
    CacheHexagonOwner(n);
  }
  CacheUnpinnedHexagons();
}

void HiveBoard::CacheLegalMoves() {
  legalMoves_.clear();
  for (Hexagon* h : unpinned_) {
    h->GenerateMoves(h->bug.value().type, legalMoves_);
  }
  for (Offset o : available_[to_play]) {
    for (int8_t bug=0; bug < kNumBugs; bug++) {
      HiveMove m = HiveMove{
        false, true, (BugType) bug,
        Offset{}, o.index
      };
      legalMoves_.push_back(m);
    }
  }
  if (legalMoves_.size() == 0) {
    legalMoves_.push_back(HiveMove{true});
  }
};

void HiveBoard::CacheOutcome() {
  // TODO: check if it's possible to lose whithout an opposing bee
  if (bees_[kWhite] == NULL || bees_[kBlack] == NULL) {
    outcome = kInvalidPlayer;
    return;
  }
  if (bees_[kWhite]->IsSurrounded() ^ bees_[kBlack]->IsSurrounded()) {
    outcome = (Player) bees_[kBlack]->IsSurrounded();
  } else {
    outcome = kInvalidPlayer;
  }
}

void HiveBoard::CacheIsTerminal() {
  is_terminal = bees_[kWhite]->IsSurrounded() || bees_[kBlack]->IsSurrounded();
}

void HiveBoard::PlayMove(HiveMove &m) {
  if (m.pass) {
    to_play = (Player) !to_play;
    return;
  }
  if (m.place) {
    PlaceBug(m.to, m.bug);
  } else {
    // TODO: Remove bug from board and set to h
    absl::optional<Bug> b = RemoveBug(GetHexagon(m.from));
    if (!b.has_value()) {
      return;
    }
    PlaceBug(m.to, b.value().type);
  }
  CacheOutcome();
  CacheIsTerminal();
  to_play = (Player) !to_play;
}

void HiveBoard::UndoMove(HiveMove &m) {
  if (m.pass) {
    to_play = (Player) !to_play;
    return;
  } else if (m.place) {
    Hexagon* h = GetHexagon(m.to);
    RemoveBug(h);
  } else {
    HiveMove m_inverse = HiveMove{ false, false, m.bug, m.to, m.from };
    PlayMove(m_inverse);
  }
  CacheOutcome();
  CacheIsTerminal();
  to_play = (Player) !to_play;
}

/*
bool HiveBoard::IsLegalMove(HiveMove m) const {
  if (m.pass) return true;
  if (!m.place && m.from_hex.isInvalid()) return false;
  if (m.to_hex.isInvalid()) return false;

  if (m.place && bug_collections_[to_play].HasBug(m.bug)) return false;

  return legalMoves_.
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
  HiveBoard board();
}

}  // namespace hive
}  // namespace open_spiel
