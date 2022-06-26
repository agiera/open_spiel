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
#include <string>

#include "open_spiel/abseil-cpp/absl/random/uniform_int_distribution.h"
#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/games/chess/chess_common.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_globals.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/abseil-cpp/absl/types/optional.h"

namespace open_spiel {
namespace hive {

// TODO
absl::optional<BugType> BugTypeFromChar(char c) {
  return BugType::kBee;
}

std::string Bug::ToString() const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LE(player, 1);
  SPIEL_CHECK_GE(type, 0);
  SPIEL_CHECK_LE(type, 7);
  SPIEL_CHECK_GE(order, 0);
  SPIEL_CHECK_LE(order, 2);

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
  if (y % 2 == 0) {
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
  if (o.y % 2 == 0) {
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

std::ostream& operator<<(std::ostream& os, Hexagon* h) {
  return os << HexagonToString(h);
}

std::string HexagonToString(Hexagon* h) {
  return absl::StrCat("(", h->loc.x + 1, ", ", h->loc.y + 1, ", ", h->loc.z + 1, ")");
}

Hexagon::Hexagon() {
  loc.x = 0;
  loc.y = 0;
  loc.z = 0;
  bug = kEmptyBug;
  visited = false;
  above = NULL;
  below = NULL;
  neighbours.fill(NULL);
}

Hexagon* Hexagon::Bottom() const {
  Hexagon* h = above;
  while (h->loc.z != 0) {
    h = h->below;
  }
  return h;
}

Hexagon* Hexagon::Top() const {
  Hexagon* h = Bottom();
  while (h->loc.z != kBoardHeight-1 && h->above->bug != kEmptyBug) {
    h = h->above;
  }
  return h;
}

int Hexagon::FindClockwiseMove(int prev_idx) const {
  int j;
  for (int8_t i=0; i < 6; i++) {
    j = (prev_idx + 1 - i) % 6;
    if (neighbours[j]->bug != kEmptyBug &&
        neighbours[(j+1) % 6]->bug != kEmptyBug &&
        neighbours[(j-1) % 6]->bug != kEmptyBug) {
      return i;
    }
  }
  return -1;
}

int Hexagon::FindCounterClockwiseMove(int prev_idx) const {
  int j;
  for (int8_t i=0; i < 6; i++) {
    j = (prev_idx - 1 + i) % 6;
    if (neighbours[j]->bug != kEmptyBug &&
        neighbours[(j-1) % 6]->bug != kEmptyBug &&
        neighbours[(j+1) % 6]->bug != kEmptyBug) {
      return i;
    }
  }
  return -1;
}

std::vector<Hexagon*> Hexagon::FindJumpMoves() const {
  std::vector<Hexagon*> jumps;
  Hexagon* b = Bottom();
  for (Hexagon* n : b->neighbours) {
    if (n->bug != kEmptyBug) {
      // TODO: filter gated
      jumps.push_back(n->Top());
    }
  }
  return jumps;
}

bool Hexagon::IsSurrounded() const {
  bool res = true;
  for (Hexagon* h : neighbours) {
    if (h->bug == kEmptyBug) {
      res = false;
    }
  }
  return res;
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
    moves.push_back(HiveMove(above->below, neighbours[i]));
  }
}

void Hexagon::GenerateBeetleMoves(std::vector<HiveMove> &moves) const {
  GenerateBeeMoves(moves);
  for (Hexagon* h : FindJumpMoves()) {
    moves.push_back(HiveMove(above->below, h));
  }
}

void Hexagon::GenerateAntMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<Hexagon*> antMoves;

  int i = FindClockwiseMove(0);
  int init_idx = i;
  Hexagon* h = neighbours[i];
  Hexagon* init_h = h;
  while (i != -1 && (h != init_h || i != init_idx)) {
    antMoves.insert(h);

    i = h->FindClockwiseMove(i);
    h = h->neighbours[i];
  }

  for (Hexagon* h : antMoves) {
    moves.push_back(HiveMove(above->below, h));
  }
}

void Hexagon::GenerateGrasshopperMoves(std::vector<HiveMove> &moves) const {
  for (int8_t i=0; i < 6; i++) {
    if (neighbours[i]->bug != kEmptyBug) {
      Hexagon* n = neighbours[i];
      // There aren't enough hexagons to wrap around the board
      // So no infinite loop
      while(n->bug != kEmptyBug) {
        n = n->neighbours[i];
      }
      moves.push_back(HiveMove(above->below, n));
    }
  }
}

Hexagon* Hexagon::WalkThree(int i, bool clockwise) const {
  Hexagon* h;
  if (clockwise) {
    int i = FindClockwiseMove(i);
    h = neighbours[i];
    if (i == -1) { return NULL; }
    i = h->FindClockwiseMove(i);
    h = h->neighbours[i];
    i = FindClockwiseMove(i);
    h = h->neighbours[i];
  } else {
    int i = FindCounterClockwiseMove(i);
    h = neighbours[i];
    if (i == -1) { return NULL; }
    i = FindCounterClockwiseMove(i);
    h = h->neighbours[i];
    i = FindCounterClockwiseMove(i);
    h = h->neighbours[i];
  }
  return h;
}

void Hexagon::GenerateSpiderMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<Hexagon*> spiderMoves;
  spiderMoves.insert(WalkThree(0, true));
  spiderMoves.insert(WalkThree(3, true));
  spiderMoves.insert(WalkThree(0, false));
  spiderMoves.insert(WalkThree(3, false));
  spiderMoves.erase(NULL);
  for (Hexagon* h : spiderMoves) {
    moves.push_back(HiveMove(above->below, h));
  }
}

void Hexagon::GenerateLadybugMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<Hexagon*> ladybugMoves;
  for (Hexagon* n : FindJumpMoves()) {
    for (Hexagon* o : n->FindJumpMoves()) {
      for (Hexagon* p : o->neighbours) {
        if (p->bug == kEmptyBug) {
          ladybugMoves.insert(p);
        }
      }
    }
  }
  for (Hexagon* h : ladybugMoves) {
    moves.push_back(HiveMove(above->below, h));
  }
}

void Hexagon::GenerateMosquitoMoves(std::vector<HiveMove> &moves) const {
  std::unordered_set<BugType> mirrors;
  for (Hexagon* h : neighbours) {
    if (h->bug != kEmptyBug) {
      mirrors.insert(h->bug.type);
    }
  }
  for (BugType t : mirrors) {
    GenerateMoves(t, moves);
  }
}

void Hexagon::GeneratePillbugMoves(std::vector<HiveMove> &moves) const {
  GenerateBeeMoves(moves);
  std::vector<Hexagon*> emptyNeighbours;
  for (Hexagon* n : neighbours) {
    if (n->bug == kEmptyBug) {
      emptyNeighbours.push_back(n);
    }
  }
  for (Hexagon * n : neighbours) {
    if (n->bug != kEmptyBug && !n->last_moved) {
      for (Hexagon* empty : emptyNeighbours) {
        moves.push_back(HiveMove(n, empty));
      }
    }
  }
}

BugCollection::BugCollection(Player p) :
  player_(p), bug_counts_({ 0, 0, 0, 0, 0, 0, 0, 0 }) {}

void BugCollection::Reset() {
  bug_counts_ = { 0, 0, 0, 0, 0, 0, 0, 0 };
  for (std::vector<Hexagon*> h_vec : hexagons_) {
    h_vec.empty();
  }
}

void BugCollection::ReturnBug(Hexagon* h) {
  bug_counts_[h->bug.type]--;
  hexagons_[h->bug.type].pop_back();
  h->bug = kEmptyBug;
}

bool BugCollection::HasBug(BugType t) const {
  return !(bug_counts_[t] == bug_counts[t]);
}

bool BugCollection::UseBug(Hexagon* h, BugType t) {
  if (HasBug(t)) {
    hexagons_[(int) t].push_back(h);
    Bug bug = Bug{player_, t, bug_counts_[t]};
    h->bug = bug;
    bug_counts_[t]++;
    return true;
  }
  return false;
}

Hexagon* BugCollection::GetBug(Bug b) const {
  if (b.order >= bug_counts_[b.type]) {
    return NULL;
  }
  return hexagons_[b.type][b.order];
}

int8_t BugCollection::NumBugs(BugType bt) const {
  return bug_counts_[bt];
}

HiveBoard::HiveBoard()
  : bug_collections_({ BugCollection(kWhite), BugCollection(kBlack) }) {
  starting_hexagon = GetHexagon(13, 13, 0);
  Clear();
  InitBoard();
}

void HiveBoard::Clear() {  
  hexagons_.empty();
  bees_ = {NULL, NULL};
  outcome = kInvalidPlayer;
  is_terminal = false;

  bug_collections_[kBlack].Reset();
  bug_collections_[kWhite].Reset();
  
  for (Hexagon &h : board_) {
    h.bug = kEmptyBug;
  }

  zobrist_hash = 0;
  std::fill(begin(observation), end(observation), 0);

  available_[kWhite].clear();
  available_[kBlack].clear();
  available_[kWhite].insert(GetHexagon(13, 13, 0));
}

void HiveBoard::InitBoard() {
  // Initialize hexagon neighbours
  // TODO: make sure this is done at compile time
  for (int8_t x=0; x < kBoardSize; x++) {
    for (int8_t y=0; y < kBoardSize; y++) {
      for (int8_t z=0; z < kBoardHeight; z++) {
        Offset o = Offset(x, y, z);
        Hexagon* h = GetHexagon(o);
        h->loc = o;
        std::array<Offset, 6> neighbours = o.neighbours();
        for (int n=0; n < 6; n++) {
          h->neighbours[n] = GetHexagon(neighbours[n]);
        }
        h->above = GetHexagon(x, y, z + 1);
        h->below = GetHexagon(x, y, z - 1);
      }
    }
  }
}

Hexagon* HiveBoard::GetHexagon(Offset o) {
  return &board_[o.index];
}

Hexagon* HiveBoard::GetHexagon(int x, int y, int z) {
  return GetHexagon(Offset(x, y, z));
}

Hexagon* HiveBoard::GetHexagon(Bug b) const {
  return bug_collections_[b.player].GetBug(b);
}

std::size_t HiveBoard::NumBugs() const {
  return hexagons_.size();
}

std::size_t HiveBoard::NumBugs(Player p, BugType bt) const {
  return bug_collections_[p].NumBugs(bt);
}

Player HiveBoard::HexagonOwner(Hexagon* h) const {
  Player p = kInvalidPlayer;
  for (Hexagon* n : h->neighbours) {
    if (n->bug != kEmptyBug) {
      p = n->bug.player;
      if (p != n->bug.player) {
        return kInvalidPlayer;
      }
    }
  }
  return p;
}

void HiveBoard::CacheHexagonOwner(Hexagon* h) {
  Player p = HexagonOwner(h);
  if (p != kInvalidPlayer) {
    available_[p].insert(h);
  } else {
    available_[kBlack].erase(h);
    available_[kWhite].erase(h);
  }

  if (hexagons_.size() == 0) {
    available_[kWhite].clear();
    available_[kWhite].insert(GetHexagon(13, 13, 0));
  } else if (hexagons_.size() == 1) {
    available_[kBlack].clear();
    available_[kBlack].insert(GetHexagon(13, 14, 0));
  }
}

void HiveBoard::CacheUnpinnedHexagons() {
  // TODO: Find or create an articulation point vertex streaming algorithm.
  //       It's also possible there could be some optimizations for
  //       hexagon grid or planar graphs.
  unpinned_.empty();
  if (hexagons_.empty()) { return; }
  std::stack<Hexagon*> preorderStack;
  std::stack<Hexagon*> postorderStack;
  preorderStack.push((*hexagons_.begin())->Bottom());
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
      if (!n->visited && n->bug != kEmptyBug) {
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
      if (n->bug == kEmptyBug) { continue; }
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

Bug HiveBoard::RemoveBug(Hexagon* h) {
  if (h->bug == kEmptyBug) { return kEmptyBug; }
  Bug b = h->bug;
  hexagons_.erase(h);
  // TODO: use reps
  observation[h->loc.index] = 0;
  if (b.type == kBee) {
    bees_[b.player] = NULL;
  }
  zobrist_hash ^= zobristTable[b.player][b.type][h->loc.x][h->loc.y][h->loc.z];
  bug_collections_[h->bug.player].ReturnBug(h);
  CacheHexagonOwner(h);
  for (Hexagon* n : h->neighbours) {
    CacheHexagonOwner(n);
  }
  return b;
}

void HiveBoard::PlaceBug(Hexagon* h, Bug b) {
  bug_collections_[b.player].UseBug(h, b.type);
  zobrist_hash ^= zobristTable[to_play][h->bug.type][h->loc.x][h->loc.y][h->loc.z];
  hexagons_.insert(h);
  observation[h->loc.index] = b.type + 1;
  if (b.type == kBee) {
    bees_[b.player] = h;
  }
  CacheHexagonOwner(h);
  for (Hexagon* n : h->neighbours) {
    CacheHexagonOwner(n);
  }
}

std::vector<HiveMove> HiveBoard::LegalMoves() const {
  std::vector<HiveMove> legal_moves;

  if (bees_[to_play] != NULL) {
    for (Hexagon* h : unpinned_) {
      if (h->bug.player == to_play) {
        h->GenerateMoves(h->bug.type, legal_moves);
      }
    }
  }

  for (Hexagon* h : available_[to_play]) {
    std::cout << std::to_string(h->loc.x);
    std::cout << ", ";
    std::cout << std::to_string(h->loc.y);
    std::cout << "; ";
    for (int8_t bug=0; bug < kNumBugTypes; bug++) {
      if (bug_collections_[to_play].HasBug((BugType) bug)) {
        legal_moves.push_back(HiveMove((BugType) bug, h));
      }
    }
  }
  std::cout << "\n";

  if (legal_moves.size() == 0) {
    legal_moves.push_back(HiveMove());
  }
  return legal_moves;
};

void HiveBoard::CacheOutcome() {
  // TODO: check if it's possible to lose whithout an opposing bee
  if (bees_[kWhite] == NULL || bees_[kBlack] == NULL) {
    outcome = kInvalidPlayer;
    return;
  }

  bool white_surrounded = bees_[kWhite]->IsSurrounded();
  bool black_surrounded = bees_[kBlack]->IsSurrounded();
  if (white_surrounded ^ black_surrounded) {
    outcome = (Player) black_surrounded;
  } else {
    outcome = kInvalidPlayer;
  }
}

void HiveBoard::CacheIsTerminal() {
  if (bees_[kWhite] == NULL || bees_[kBlack] == NULL) {
    is_terminal = false;
    return;
  }
  is_terminal = bees_[kWhite]->IsSurrounded() || bees_[kBlack]->IsSurrounded();
}

void HiveBoard::PlayMove(HiveMove &m) {
  if (m.pass) {
    to_play = (Player) !to_play;
    return;
  }
  if (m.place) {
    PlaceBug(m.to, Bug{to_play, m.bug_type, 0});
  } else {
    // TODO: Remove bug from board and set to h
    Bug b = RemoveBug(m.from);
    if (b == kEmptyBug) {
      return;
    }
    PlaceBug(m.to, b);
  }
  // TODO: lazily eval legal moves, add getter
  CacheUnpinnedHexagons();
  CacheOutcome();
  CacheIsTerminal();
  to_play = (Player) !to_play;
}

void HiveBoard::UndoMove(HiveMove &m) {
  if (m.pass) {
    to_play = (Player) !to_play;
    return;
  } else if (m.place) {
    RemoveBug(m.to);
  } else {
    HiveMove m_inverse = HiveMove(m.to, m.from);
    PlayMove(m_inverse);
  }
  CacheUnpinnedHexagons();
  CacheOutcome();
  CacheIsTerminal();
  to_play = (Player) !to_play;
}

}  // namespace hive
}  // namespace open_spiel
