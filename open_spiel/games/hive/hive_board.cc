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

Hexagon HiveBoard::Bottom(Hexagon h) const {
  while (h.loc.z != 0) {
    h = GetHexagon(h.below);
  }
  return h;
}

// TODO: optimize
Hexagon HiveBoard::Top(Hexagon h) const {
  Hexagon top = Bottom(h);
  Hexagon above = GetHexagon(top.above);
  while (top.loc.z != kBoardHeight-1 && above.bug != kEmptyBug) {
    top = above;
    above = GetHexagon(top.above);
  }
  return top;
}

// TODO: fix bugs can be on top
int HiveBoard::FindClockwiseMove(Hexagon h, int prev_idx, Bug original) const {
  int j = hive::mod(prev_idx, 6);
  //std::cout << "j=" << j << "\n";
  //std::cout << "mod(j, 6)=" << hive::mod(j, 6) << "\n";
  //std::cout << "mod(j+1, 6)=" << hive::mod(j+1, 6) << "\n";
  Hexagon n1;
  Hexagon n2 = GetHexagon(h.neighbours[j]);
  Hexagon n3 = GetHexagon(h.neighbours[hive::mod(j+1, 6)]);
  for (int i=0; i < 6; i++) {
    std::cout << "h.neighbours[j].loc=" << OffsetToString(n2.loc) << "\n";
    j = mod(prev_idx + 1 - i, 6);
    n1 = n2;
    n2 = n3;
    n3 = GetHexagon(h.neighbours[hive::mod(j+1, 6)]);
    // A bug can go through itself
    // Specifically there's one special case for the spider
    if ((n1.bug != kEmptyBug || n1.bug == original) &&
        (n2.bug != kEmptyBug || n1.bug == original) &&
        (n3.bug != kEmptyBug || n1.bug == original)) {
      return i;
    }
  }
  return -1;
}

int HiveBoard::FindCounterClockwiseMove(Hexagon h, int prev_idx, Bug original) const {
  int j = hive::mod(prev_idx, 6);
  Hexagon n1;
  Hexagon n2 = GetHexagon(h.neighbours[hive::mod(j, 6)]);
  Hexagon n3 = GetHexagon(h.neighbours[hive::mod(j-1, 6)]);
  for (int i=0; i < 6; i++) {
    j = mod(prev_idx - 1 + i, 6);
    n1 = n2;
    n2 = n3;
    n3 = GetHexagon(h.neighbours[hive::mod(j-1, 6)]);
    if ((n1.bug != kEmptyBug || n1.bug == original) &&
        (n2.bug != kEmptyBug || n1.bug == original) &&
        (n3.bug != kEmptyBug || n1.bug == original)) {
      return i;
    }
  }
  return -1;
}

std::vector<Hexagon> HiveBoard::FindJumpMoves(Hexagon h) const {
  std::vector<Hexagon> jumps;
  Hexagon b = Bottom(h);
  for (int n_idx : b.neighbours) {
    Hexagon n = GetHexagon(n_idx);
    if (n.bug != kEmptyBug) {
      // TODO: filter gated
      Hexagon t = Top(n);
      jumps.push_back(t);
    }
  }
  return jumps;
}

bool HiveBoard::IsSurrounded(int h_idx) const {
  Hexagon h = GetHexagon(h_idx);
  bool res = true;
  for (int n_idx : h.neighbours) {
    Hexagon n = GetHexagon(n_idx);
    if (n.bug == kEmptyBug) {
      res = false;
    }
  }
  return res;
}

void HiveBoard::GenerateMoves(Hexagon h, BugType t, std::vector<HiveMove> &moves) const {
  SPIEL_DCHECK_NE(h.bug.order, -1);
  Hexagon above = GetHexagon(h.above);
  if (above.bug != kEmptyBug) { return; }
  switch (t) {
    case kBee:
      GenerateBeeMoves(h, moves);
      break;
    case kBeetle:
      GenerateBeetleMoves(h, moves);
      break;
    case kAnt:
      GenerateAntMoves(h, moves);
      break;
    case kGrasshopper:
      GenerateGrasshopperMoves(h, moves);
      break;
    case kSpider:
      GenerateSpiderMoves(h, moves);
      break;
    case kLadybug:
      GenerateLadybugMoves(h, moves);
      break;
    case kMosquito:
      GenerateMosquitoMoves(h, moves);
      break;
    case kPillbug:
      GeneratePillbugMoves(h, moves);
      break;
    default: break;
  };
}

void HiveBoard::GenerateBeeMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<int> beeMoves;
  beeMoves.insert(FindClockwiseMove(h, 0, h.bug));
  beeMoves.insert(FindClockwiseMove(h, 3, h.bug));
  beeMoves.insert(FindCounterClockwiseMove(h, 0, h.bug));
  beeMoves.insert(FindCounterClockwiseMove(h, 3, h.bug));
  beeMoves.erase(-1);
  for (int i : beeMoves) {
    moves.push_back(HiveMove(h.loc.index, h.neighbours[i]));
  }
}

// TODO: fix bugs can be on top
void HiveBoard::GenerateBeetleMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  GenerateBeeMoves(h, moves);
  for (Hexagon n : FindJumpMoves(h)) {
    moves.push_back(HiveMove(h.loc.index, n.loc.index));
  }
}

void HiveBoard::GenerateAntMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<int> antMoves;

  int i = FindClockwiseMove(h, 0, h.bug);
  int init_idx = i;
  Hexagon next = GetHexagon(h.neighbours[i]);
  int root = next.loc.index;
  while (i != -1 && (next.loc.index != root || i != init_idx)) {
    antMoves.insert(h.loc.index);

    i = FindClockwiseMove(next, i, h.bug);
    next = GetHexagon(next.neighbours[i]);
  }

  for (int p : antMoves) {
    moves.push_back(HiveMove(h.loc.index, p));
  }
}

void HiveBoard::GenerateGrasshopperMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  for (int8_t i=0; i < 6; i++) {
    Hexagon n = GetHexagon(h.neighbours[i]);
    if (n.bug != kEmptyBug) {
      // There aren't enough hexagons to wrap around the board
      // So no infinite loop
      while(n.bug != kEmptyBug) {
        n = GetHexagon(n.neighbours[i]);
      }
      moves.push_back(HiveMove(h.loc.index, n.loc.index));
    }
  }
}

int HiveBoard::WalkThree(Hexagon h, int i, bool clockwise) const {
  if (clockwise) {
    int j = FindClockwiseMove(h, i, h.bug);
    h = GetHexagon(h.neighbours[j]);
    if (j == -1) { return -1; }
    j = FindClockwiseMove(h, j, h.bug);
    h = GetHexagon(h.neighbours[j]);
    j = FindClockwiseMove(h, j, h.bug);
  } else {
    int j = FindCounterClockwiseMove(h, i, h.bug);
    h = GetHexagon(h.neighbours[j]);
    if (j == -1) { return -1; }
    j = FindCounterClockwiseMove(h, j, h.bug);
    h = GetHexagon(h.neighbours[j]);
    j = FindCounterClockwiseMove(h, j, h.bug);
  }
  return h.neighbours[i];
}

void HiveBoard::GenerateSpiderMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<int> spiderMoves;
  spiderMoves.insert(WalkThree(h, 0, true));
  spiderMoves.insert(WalkThree(h, 3, true));
  spiderMoves.insert(WalkThree(h, 0, false));
  spiderMoves.insert(WalkThree(h, 3, false));
  spiderMoves.erase(-1);
  for (int p : spiderMoves) {
    moves.push_back(HiveMove(h.loc.index, p));
  }
}

void HiveBoard::GenerateLadybugMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<int> ladybugMoves;
  for (Hexagon n : FindJumpMoves(h)) {
    for (Hexagon o : FindJumpMoves(n)) {
      // Can't jump on self
      if (o.bug == h.bug) { continue; }
      for (int p_idx : o.neighbours) {
        Hexagon p = Bottom(GetHexagon(p_idx));
        if (p.bug == kEmptyBug) {
          ladybugMoves.insert(p_idx);
        }
      }
    }
  }
  for (int p_idx : ladybugMoves) {
    moves.push_back(HiveMove(h.loc.index, p_idx));
  }
}

void HiveBoard::GenerateMosquitoMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<BugType> mirrors;
  for (int n_idx : h.neighbours) {
    Hexagon n = Top(GetHexagon(n_idx));
    if (n.bug != kEmptyBug) {
      mirrors.insert(n.bug.type);
    }
  }
  mirrors.erase(kMosquito);
  for (BugType t : mirrors) {
    SPIEL_DCHECK_NE(h.bug.order, -1);
    GenerateMoves(h, t, moves);
  }
}

void HiveBoard::GeneratePillbugMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  GenerateBeeMoves(h, moves);
  std::vector<int> emptyNeighbours;
  for (int n_idx : h.neighbours) {
    Hexagon n = GetHexagon(n_idx);
    if (n.bug == kEmptyBug) {
      emptyNeighbours.push_back(n.loc.index);
    }
  }
  for (int n_idx : h.neighbours) {
    Hexagon n = GetHexagon(n_idx);
    if (n.bug != kEmptyBug && BugCanMove(n)) {
      for (int empty : emptyNeighbours) {
        moves.push_back(HiveMove(n_idx, empty));
      }
    }
  }
}

HiveBoard::HiveBoard()
  : bug_collections_({ BugCollection(kWhite), BugCollection(kBlack) }) {
  Clear();
  InitBoard();
}

void HiveBoard::Clear() {  
  hexagons_.clear();
  bees_ = {(int) -1, (int) -1};
  to_play = kWhite;
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
  available_[kWhite].insert(starting_hexagon);
}

void HiveBoard::InitBoard() {
  // Initialize hexagon neighbours
  // TODO: make sure this is done at compile time
  for (int8_t x=0; x < kBoardSize; x++) {
    for (int8_t y=0; y < kBoardSize; y++) {
      for (int8_t z=0; z < kBoardHeight; z++) {
        Offset o = Offset(x, y, z);
        Hexagon* h = &board_[o.index];
        h->loc = o;
        std::array<Offset, 6> neighbours = o.neighbours();
        for (int n=0; n < 6; n++) {
          h->neighbours[n] = neighbours[n].index;
        }
        h->above = Offset(x, y, z + 1).index;
        h->below = Offset(x, y, z - 1).index;
      }
    }
  }
}

Hexagon HiveBoard::GetHexagon(int idx) const {
  return board_[idx];
}

Hexagon HiveBoard::GetHexagon(Offset o) const {
  return board_[o.index];
}

Hexagon HiveBoard::GetHexagon(int x, int y, int z) const {
  return GetHexagon(Offset(x, y, z));
}

// TODO: use optional return type
Hexagon HiveBoard::GetHexagon(Bug b) const {
  absl::optional<Offset> o = bug_collections_[b.player].GetBug(b);
  if (o.has_value()) {
    return GetHexagon(o.value());
  } else {
    return kEmptyHexagon;
  }
}

int HiveBoard::NumBugs() const {
  return hexagons_.size();
}

int HiveBoard::NumBugs(Player p, BugType bt) const {
  return bug_collections_[p].NumBugs(bt);
}

bool HiveBoard::BugCanMove(Hexagon h) const {
  Hexagon above = GetHexagon(h.above);
  return (!pinned_.count(h.loc.index) || h.loc.z > 0) \
         && above.bug == kEmptyBug && last_moved_.top() != h.loc.index;
}

Player HiveBoard::HexagonOwner(Hexagon h) const {
  std::cout << "\nHexagonOwner\n";
  Player p = kInvalidPlayer;

  // Can't place bug on top of your own
  Hexagon top = Top(h);
  if (top.bug != kEmptyBug) { return p; }

  std::cout << "top.loc=" << OffsetToString(top.loc);
  // Make sure there aren't enemy bugs around hex
  for (int n_idx : h.neighbours) {
    Hexagon n = Top(GetHexagon(n_idx));
    std::cout << "n.loc=" << OffsetToString(n.loc);
    std::cout << "n.bug=" << BugToString(n.bug) << "\n";
    if (n.bug != kEmptyBug) {
      if (p != kInvalidPlayer && p != n.bug.player) {
        return kInvalidPlayer;
      }
      p = n.bug.player;
    }
  }
  return p;
}

void HiveBoard::CacheHexagonOwner(Hexagon h) {
  Player p = HexagonOwner(h);
  if (p != kInvalidPlayer) {
    std::cout << "caching in available_\n";
    available_[p].insert(h.loc.index);
  } else {
    available_[kBlack].erase(h.loc.index);
    available_[kWhite].erase(h.loc.index);
  }
}

void HiveBoard::CacheHexagonArea(Hexagon h) {
  CacheHexagonOwner(h);
  for (int idx : h.neighbours) {
    CacheHexagonOwner(GetHexagon(idx));
  }

  if (hexagons_.size() == 0) {
    available_[kWhite].clear();
    available_[kWhite].insert(starting_hexagon);
  } else if (hexagons_.size() == 1) {
    available_[kBlack].clear();
    available_[kBlack].insert(starting_hexagon + 1u);
  }
}

void HiveBoard::CachePinnedHexagons() {
  // TODO: Find or create an articulation point vertex streaming algorithm.
  //       It's also possible there could be some optimizations for
  //       hexagon grid or planar graphs.
  pinned_.clear();
  if (hexagons_.empty()) { return; }
  std::stack<Hexagon*> preorderStack;
  std::stack<Hexagon*> postorderStack;
  Hexagon* root = &board_[*(hexagons_.begin())];
  SPIEL_DCHECK_NE(root->bug.order, -1);
  root = &board_[Bottom(*root).loc.index];
  root->bug.visited = true;
  preorderStack.push(root);
  int num = 0;
  // Pre-order traversal
  // Builds DFS tree
  while (!preorderStack.empty()) {
    Hexagon* curNode = preorderStack.top();
    preorderStack.pop();
    postorderStack.push(curNode);
    SPIEL_DCHECK_NE(curNode->bug.order, -1);

    curNode->bug.num = num++;
    curNode->bug.low = curNode->bug.num;
    curNode->bug.children = 0;

    for (int n_idx : curNode->neighbours) {
      Hexagon* n = &board_[n_idx];
      if (n->bug == kEmptyBug) { continue; }

      if (!n->bug.visited) {
        curNode->bug.children++;

        n->bug.parent = curNode->loc.index;

        n->bug.visited = true;
        preorderStack.push(n);
      }
    }
  }
  // Post-order traversal
  // Caluclates articulation points based on DFS tree structure
  while(!postorderStack.empty()) {
    Hexagon* curNode = postorderStack.top();
    postorderStack.pop();
    for (int n_idx : curNode->neighbours) {
      Hexagon* n = &board_[n_idx];
      if (n->bug == kEmptyBug) { continue; }

      curNode->bug.low = std::min(curNode->bug.low, n->bug.low);
      if (n->bug.low >= curNode->bug.num) {
        if (curNode->bug.parent != -1 || curNode->bug.children > 1) {
          std::cout << "pinned: " << BugToString(curNode->bug) << "\n";
          pinned_.insert(curNode->loc.index);
        }
      }
    }
    // Reset attributes for next fn call
    curNode->bug.visited = false;
    curNode->bug.parent = -1;
  }
}

Bug HiveBoard::MoveBug(Hexagon* from, Hexagon* to) {
  SPIEL_DCHECK_NE(from->bug.order, -1);
  SPIEL_DCHECK_EQ(to->bug.order, -1);

  Bug b = from->bug;

  // Remove bug
  hexagons_.erase(from->loc.index);
  observation[from->loc.index] = 0;
  if (b.type == kBee) {
    bees_[b.player] = -1;
  }
  zobrist_hash ^= zobristTable[b.player][b.type][from->loc.x][from->loc.y][from->loc.z];
  
  // Replace bug
  bug_collections_[b.player].MoveBug(from, to);
  zobrist_hash ^= zobristTable[to_play][b.type][to->loc.x][to->loc.y][to->loc.z];
  hexagons_.insert(to->loc.index);
  observation[to->loc.index] = b.type + 1;
  if (b.type == kBee) {
    bees_[b.player] = to->loc.index;
  }

  CacheHexagonArea(*from);
  CacheHexagonArea(*to);
  return b;
}

Bug HiveBoard::RemoveBug(Hexagon* h) {
  SPIEL_DCHECK_NE(h->bug.order, -1);
  Bug b = h->bug;
  hexagons_.erase(h->loc.index);
  // TODO: use reps
  observation[h->loc.index] = 0;
  if (b.type == kBee) {
    bees_[b.player] = -1;
  }
  zobrist_hash ^= zobristTable[b.player][b.type][h->loc.x][h->loc.y][h->loc.z];
  bug_collections_[h->bug.player].ReturnBug(h);
  CacheHexagonArea(*h);
  return b;
}

void HiveBoard::PlaceBug(Hexagon* h, Bug b) {
  SPIEL_DCHECK_NE(b.order, -1);
  SPIEL_DCHECK_EQ(h->bug.order, -1);
  bug_collections_[b.player].UseBug(h, b.type);
  zobrist_hash ^= zobristTable[to_play][h->bug.type][h->loc.x][h->loc.y][h->loc.z];
  hexagons_.insert(h->loc.index);
  observation[h->loc.index] = b.type + 1;
  if (b.type == kBee) {
    bees_[b.player] = h->loc.index;
  }
  CacheHexagonArea(*h);
}

std::vector<HiveMove> HiveBoard::LegalMoves() const {
  //std::cout << "\nLegalMoves\n";
  std::vector<HiveMove> legal_moves;

  // Player can only move bugs after the bee has been placed
  if (bees_[to_play] != -1) {
    //std::cout << "found bee\n";
    for (int idx : hexagons_) {
      Hexagon h = board_[idx];
      if (h.bug.player == to_play && BugCanMove(h)) {
        SPIEL_DCHECK_NE(h.bug.order, -1);
        GenerateMoves(h, h.bug.type, legal_moves);
      }
    }
  // Player must place bee by the third move
  } else if (hexagons_.size() >= 4) {
    for (int idx : available_[to_play]) {
      legal_moves.push_back(HiveMove(kBee, idx));
    }
    return legal_moves;
  }


  // Player may always place bugs
  for (int idx : available_[to_play]) {
    //std::cout << "idx=" << idx << "\n";
    std::cout << "available.bug=" << BugToString(GetHexagon(idx).bug) << "\n";
    for (int8_t bug=0; bug < kNumBugTypes; bug++) {
      if (bug_collections_[to_play].HasBug((BugType) bug)) {
        //std::cout << kBugTypeChars[bug] << "\n";
        legal_moves.push_back(HiveMove((BugType) bug, idx));
      }
    }
  }

  // Player may pass if there are no legal moves
  if (legal_moves.size() == 0) {
    legal_moves.push_back(HiveMove());
  }
  return legal_moves;
};

void HiveBoard::CacheOutcome() {
  if (bees_[kWhite] == -1 || bees_[kBlack] == -1) {
    outcome = kInvalidPlayer;
    is_terminal = false;
    return;
  }

  bool white_surrounded = IsSurrounded(bees_[kWhite]);
  bool black_surrounded = IsSurrounded(bees_[kBlack]);
  if (white_surrounded ^ black_surrounded) {
    outcome = (Player) black_surrounded;
  } else {
    outcome = kInvalidPlayer;
  }

  is_terminal = outcome != kInvalidPlayer;
}

void HiveBoard::PlayMove(HiveMove &m) {
  //std::cout << "\nPlayMove\n";
  if (m.pass || (!m.place && m.from == m.to)) {
    to_play = (Player) !to_play;
    return;
  } else if (m.place) {
    PlaceBug(&board_[m.to], Bug{to_play, m.bug_type, 0});
  } else {
    MoveBug(&board_[m.from], &board_[m.to]);
  }
  last_moved_.push(m.to);

  // TODO: lazily eval legal moves, add getter
  CachePinnedHexagons();
  CacheOutcome();
  to_play = (Player) !to_play;
}

void HiveBoard::UndoMove(HiveMove &m) {
  if (m.pass) {
    to_play = (Player) !to_play;
    return;
  } else if (m.place) {
    RemoveBug(&board_[m.to]);
  } else {
    MoveBug(&board_[m.to], &board_[m.from]);
  }
  last_moved_.pop();

  CachePinnedHexagons();
  CacheOutcome();
  to_play = (Player) !to_play;
}

}  // namespace hive
}  // namespace open_spiel
