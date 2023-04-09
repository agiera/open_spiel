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
  SPIEL_DCHECK_NE(h.bug.order, -1);
  SPIEL_DCHECK_NE(h.bug.below, h.bug.idx);
  Hexagon bottom = h;
  while (bottom.bug.below != (uint8_t) -1) {
    bottom = GetHexagonFromBug(bottom.bug.below);
  }
  return bottom;
}

Hexagon HiveBoard::Top(Hexagon h) const {
  SPIEL_DCHECK_NE(h.bug.order, -1);
  SPIEL_DCHECK_NE(h.bug.above, h.bug.idx);
  Hexagon top = h;
  while (top.bug.above != (uint8_t) -1) {
    top = GetHexagonFromBug(top.bug.above);
  }
  return top;
}

int HiveBoard::Height(Hexagon h) const {
  int height = 0;
  Hexagon cur_hex = h;
  while (cur_hex.bug.below != (uint8_t) -1) {
    cur_hex = GetHexagonFromBug(cur_hex.bug.below);
    height++;
  }
  return height;
}

int HiveBoard::FindClockwiseMove(Offset o, int prev_idx, Bug original) const {
  int j = mod(prev_idx, 6);
  //std::cout << "j=" << j << "\n";
  //std::cout << "mod(j, 6)=" << hive::mod(j, 6) << "\n";
  //std::cout << "mod(j+1, 6)=" << hive::mod(j+1, 6) << "\n";
  Hexagon n1;
  Hexagon n2 = GetHexagon(o.neighbours[mod(j-1, 6)]);
  Hexagon n3 = GetHexagon(o.neighbours[j]);
  for (int i=0; i < 6; i++) {
    //std::cout << "h.neighbours[j].loc=" << OffsetToString(n2.loc) << "\n";
    j = mod(prev_idx + i, 6);
    n1 = n2;
    n2 = n3;
    n3 = GetHexagon(o.neighbours[mod(j+1, 6)]);
    // A bug can go through itself
    // Specifically there's one special case for the spider
    if ((n1.bug != kEmptyBug && n1.bug != original) &&
        (n2.bug == kEmptyBug || n2.bug == original) &&
        (n3.bug == kEmptyBug || n3.bug == original)) {
      return j;
    }
  }
  return -1;
}

int HiveBoard::FindCounterClockwiseMove(Offset o, int prev_idx, Bug original) const {
  int j = mod(prev_idx, 6);
  Hexagon n1;
  Hexagon n2 = GetHexagon(o.neighbours[mod(j-1, 6)]);
  Hexagon n3 = GetHexagon(o.neighbours[j]);
  for (int i=0; i < 6; i++) {
    j = mod(prev_idx + i, 6);
    n1 = n2;
    n2 = n3;
    n3 = GetHexagon(o.neighbours[mod(j+1, 6)]);
    if ((n1.bug == kEmptyBug || n1.bug == original) &&
        (n2.bug == kEmptyBug || n2.bug == original) &&
        (n3.bug != kEmptyBug && n3.bug != original)) {
      return j;
    }
  }
  return -1;
}

std::vector<Hexagon> HiveBoard::FindJumpMoves(Hexagon h) const {
  std::vector<Hexagon> jumps;
  Hexagon n1;
  Hexagon n2 = GetHexagon(h.loc.neighbours[5]);
  Hexagon n3 = GetHexagon(h.loc.neighbours[0]);
  for (int i=0; i < 6; i++) {
    n1 = n2;
    n2 = n3;
    n3 = GetHexagon(h.loc.neighbours[mod(i+1, 6)]);
    if (n2.bug != kEmptyBug &&
        // Jump moves are gated
        (Height(n1) <= Height(n2) || Height(n3) <= Height(n2))) {
      jumps.push_back(n2);
    }
  }
  return jumps;
}

bool HiveBoard::IsSurrounded(BugIdx h_idx) const {
  if (h_idx == (uint8_t) -1) return false;
  Hexagon h = GetHexagonFromBug(h_idx);
  for (OffsetIdx n_idx : h.loc.neighbours) {
    Hexagon n = GetHexagon(n_idx);
    if (n.bug == kEmptyBug) {
      return false;
    }
  }
  return true;
}

void HiveBoard::GenerateMoves(Hexagon h, BugType t, std::vector<HiveMove> &moves) const {
  SPIEL_DCHECK_NE(h.bug.order, -1);
  // Mosquitos and Pillbugs can generate moves without moving themselves
  // They can do that by moving other bugs
  // So we handle BugCanMove within their generating functions
  bool is_special_bug = h.bug.type == kPillbug || h.bug.type == kMosquito;
  if (!BugCanMove(h) && !is_special_bug) return;
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
  std::unordered_set<int8_t> beeMoves;
  beeMoves.insert(FindClockwiseMove(h.loc, 0, h.bug));
  beeMoves.insert(FindClockwiseMove(h.loc, 3, h.bug));
  beeMoves.insert(FindCounterClockwiseMove(h.loc, 0, h.bug));
  beeMoves.insert(FindCounterClockwiseMove(h.loc, 3, h.bug));
  beeMoves.erase(-1);
  for (int8_t i : beeMoves) {
    moves.push_back(HiveMove(h.loc.idx, h.loc.neighbours[i]));
  }
}

// TODO: fix bugs can be on top
void HiveBoard::GenerateBeetleMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  GenerateBeeMoves(h, moves);
  for (Hexagon n : FindJumpMoves(h)) {
    moves.push_back(HiveMove(h.loc.idx, n.loc.idx));
  }
  // Beetles can hop off
  if (h.bug.below != (BugIdx) -1) {
    for (OffsetIdx n_idx : h.loc.neighbours) {
      Hexagon n = GetHexagon(n_idx);
      if (n.bug == kEmptyBug) {
        moves.push_back(HiveMove(h.loc.idx, n_idx));
      }
    }
  }
}

void HiveBoard::GenerateAntMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<int> antMoves;

  int i = FindClockwiseMove(h.loc, 0, h.bug);
  if (i == -1) return;
  int init_idx = i;
  Offset next = board_[h.loc.neighbours[i]];
  int root = next.idx;
  do {
    antMoves.insert(next.idx);

    i = FindClockwiseMove(next, i, h.bug);
    next = board_[next.neighbours[i]];
  } while (next.idx != root || i != init_idx);

  // Ants aren't allows to move where they already are
  antMoves.erase(h.loc.idx);

  for (int p : antMoves) {
    moves.push_back(HiveMove(h.loc.idx, p));
  }
}

void HiveBoard::GenerateGrasshopperMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  for (int8_t i=0; i < 6; i++) {
    OffsetIdx o = h.loc.neighbours[i];
    if (board_[o].bug_idx != (uint8_t) -1) {
      // There aren't enough hexagons to wrap around the board
      // So no infinite loop
      while(board_[o].bug_idx != (uint8_t) -1) {
        o = board_[o].neighbours[i];
      }
      moves.push_back(HiveMove(h.loc.idx, o));
    }
  }
}

OffsetIdx HiveBoard::WalkThree(Hexagon h, int i, bool clockwise) const {
  int j;
  Offset o = h.loc;
  if (clockwise) {
    j = FindClockwiseMove(o, i, h.bug);
    if (j == -1) { return (OffsetIdx) -1; }
    o = board_[o.neighbours[j]];
    j = FindClockwiseMove(o, j, h.bug);
    o = board_[o.neighbours[j]];
    j = FindClockwiseMove(o, j, h.bug);
  } else {
    j = FindCounterClockwiseMove(o, i, h.bug);
    if (j == -1) { return (OffsetIdx) -1; }
    o = board_[o.neighbours[j]];
    j = FindCounterClockwiseMove(o, j, h.bug);
    o = board_[o.neighbours[j]];
    j = FindCounterClockwiseMove(o, j, h.bug);
  }
  return o.neighbours[j];
}

void HiveBoard::GenerateSpiderMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<OffsetIdx> spiderMoves;
  spiderMoves.insert(WalkThree(h, 0, true));
  spiderMoves.insert(WalkThree(h, 3, true));
  spiderMoves.insert(WalkThree(h, 0, false));
  spiderMoves.insert(WalkThree(h, 3, false));
  spiderMoves.erase((OffsetIdx) -1);
  for (OffsetIdx p : spiderMoves) {
    moves.push_back(HiveMove(h.loc.idx, p));
  }
}

void HiveBoard::GenerateLadybugMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<OffsetIdx> ladybugMoves;
  for (Hexagon n : FindJumpMoves(h)) {
    for (Hexagon o : FindJumpMoves(n)) {
      // Can't jump on self
      if (o.bug == h.bug) { continue; }
      for (OffsetIdx p_idx : o.loc.neighbours) {
        Hexagon p = GetHexagon(p_idx);
        if (p.bug == kEmptyBug) {
          ladybugMoves.insert(p_idx);
        }
      }
    }
  }
  for (OffsetIdx p_idx : ladybugMoves) {
    moves.push_back(HiveMove(h.loc.idx, p_idx));
  }
}

void HiveBoard::GenerateMosquitoMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  std::unordered_set<BugType> mirrors;
  for (OffsetIdx n_idx : h.loc.neighbours) {
    Hexagon n = GetHexagon(n_idx);
    if (n.bug != kEmptyBug) {
      mirrors.insert(n.bug.type);
    }
  }

  // If Mosquito can't move then it can only move other bugs via the Pillbug
  // ability
  if (!BugCanMove(h)) {
    if (mirrors.count(kPillbug)) {
      GeneratePillbugMoves(h, moves);
    }
    return;
  }

  mirrors.erase(kMosquito);
  for (BugType t : mirrors) {
    SPIEL_DCHECK_NE(h.bug.order, -1);
    GenerateMoves(h, t, moves);
  }
}

void HiveBoard::GeneratePillbugMoves(Hexagon h, std::vector<HiveMove> &moves) const {
  if (BugCanMove(h)) {
    GenerateBeeMoves(h, moves);
  }

  std::vector<OffsetIdx> emptyNeighbours;
  for (OffsetIdx n_idx : h.loc.neighbours) {
    if (board_[n_idx].bug_idx == (uint8_t) -1) {
      emptyNeighbours.push_back(n_idx);
    }
  }

  for (OffsetIdx n_idx : h.loc.neighbours) {
    Hexagon n = GetHexagon(n_idx);
    if (n.bug != kEmptyBug && BugCanMove(n)) {
      for (OffsetIdx empty : emptyNeighbours) {
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
  outcome = kInvalidPlayer;
  is_terminal = false;
  zobrist_hash = 0;

  bug_collections_[kBlack].Reset();
  bug_collections_[kWhite].Reset();

  available_[kWhite].clear();
  available_[kBlack].clear();
  available_[kWhite].insert(starting_hexagon);

  pinned_.clear();

  bees_ = {(uint8_t) -1, (uint8_t) -1};
  last_moved_.clear();
  last_moved_.push_back(-1);
  to_play = kWhite;
  
  for(Hexagon& h : hexagons_) {
    h.bug = kEmptyBug;
  }

  for (Offset& o : board_) {
    o.bug_idx = -1;
  }

  top_hexagons_.clear();
  //std::fill(hexagons_.begin(), hexagons_.end(), kEmptyHexagon);
  for (int i=0; i < hexagons_.size(); i++) {
    hexagons_[i] = kEmptyHexagon;
  }
}

void HiveBoard::InitBoard() {
  // Initialize hexagon neighbours
  // TODO: make sure this is done at compile time
  for (uint8_t x=0; x < kBoardSize; x++) {
    for (uint8_t y=0; y < kBoardSize; y++) {
      Offset o = Offset(x, y);
      board_[o.idx] = o;
    }
  }
}

Hexagon HiveBoard::GetHexagon(OffsetIdx idx) const {
  SPIEL_DCHECK_GE(idx, -1);
  SPIEL_DCHECK_LT(idx, kBoardSize*kBoardSize);
  if (board_[idx].bug_idx == (uint8_t) -1) return kEmptyHexagon;
  return hexagons_[board_[idx].bug_idx];
}

Hexagon HiveBoard::GetHexagon(Offset o) const {
  return GetHexagon(o.idx);
}

Hexagon HiveBoard::GetHexagon(uint8_t x, uint8_t y, uint8_t z) const {
  return GetHexagon(Offset(x, y));
}

Hexagon HiveBoard::GetHexagonFromBug(BugIdx bug_idx) const {
  SPIEL_DCHECK_GE(bug_idx, 0);
  SPIEL_DCHECK_LT(bug_idx, kNumBugs);
  return hexagons_[bug_idx];
}

Hexagon HiveBoard::GetHexagonFromBug(Bug b) const {
  return hexagons_[b.idx];
}

int HiveBoard::NumBugs(Player p, BugType bt) const {
  return bug_collections_[p].NumBugs(bt);
}

bool HiveBoard::BugCanMove(Hexagon h) const {
  bool unpinned =  (!pinned_.count(h.bug.idx)) || (h.bug.below != (BugIdx) -1);
  bool not_smothered = h.bug.above == (BugIdx) -1;
  bool last_moved = last_moved_.back() != h.bug.idx;
  return unpinned && not_smothered && last_moved;
}

Player HiveBoard::HexagonOwner(OffsetIdx idx) const {
  //std::cout << "\nHexagonOwner\n";
  Player p = kInvalidPlayer;

  // Can't place bug on top of another
  if (board_[idx].bug_idx != (BugIdx) -1) { return p; }

  //std::cout << "top.loc=" << OffsetToString(top.loc);
  // Make sure there aren't enemy bugs around hex
  for (OffsetIdx n_idx : board_[idx].neighbours) {
    Hexagon n = GetHexagon(n_idx);
    //std::cout << "n.loc=" << OffsetToString(n.loc);
    //std::cout << "n.bug=" << BugToString(n.bug) << "\n";
    if (n.bug != kEmptyBug) {
      SPIEL_DCHECK_GE(n.bug.player, kInvalidPlayer);
      SPIEL_DCHECK_LE(n.bug.player, kBlack);
      if (p != kInvalidPlayer && p != n.bug.player) {
        return kInvalidPlayer;
      }
      p = n.bug.player;
    }
  }
  return p;
}

void HiveBoard::CacheHexagonOwner(OffsetIdx idx) {
  Player p = HexagonOwner(idx);
  if (p != kInvalidPlayer) {
    //std::cout << "available_ insert " << OffsetToString(board_[idx]) << "\n";
    //std::cout << "owner: " << p << "\n";
    available_[p].insert(idx);
    available_[!p].erase(idx);
  } else {
    available_[kWhite].erase(idx);
    available_[kBlack].erase(idx);
  }
}

void HiveBoard::CacheHexagonArea(OffsetIdx idx) {
  //std::cout << "\nHexagonArea\n";
  //std::cout << "idx=" << idx << "\n";

  // Erase the availables next to the first white bug
  // Before then caching the first black bug area
  if (top_hexagons_.size() == 2) {
    available_[kBlack].clear();
  }

  CacheHexagonOwner(idx);
  for(uint8_t i=0; i < 6; i++) {
    OffsetIdx n_idx = board_[idx].neighbours[i];
    // Set bug neighbours
    if (board_[idx].bug_idx != (BugIdx) -1) {
      Hexagon* h = &hexagons_[board_[idx].bug_idx];
      h->bug.neighbours[i] = board_[n_idx].bug_idx;
      if (board_[n_idx].bug_idx != (BugIdx) -1) {
        Hexagon* n = &hexagons_[board_[n_idx].bug_idx];
        n->bug.neighbours[mod(i - 3, 6)] = h->bug.idx;
      }
    }
    CacheHexagonOwner(n_idx);
  }

  //std::cout << "top_hexagons_.size=" << top_hexagons_.size() << "\n";
  // Initialize starting hexagons which subvert the normal rules
  if (top_hexagons_.size() == 0) {
    available_[kWhite].clear();
    available_[kWhite].insert(starting_hexagon);
  } else if (top_hexagons_.size() == 1) {
    available_[kBlack].clear();
    for (OffsetIdx idx : board_[starting_hexagon].neighbours) {
      available_[kBlack].insert(idx);
    }
  }
}

void HiveBoard::CachePinnedHexagons() {
  //std::cout << "\nCachePinnedHexagons\n";
  // TODO: Find or create an articulation point vertex streaming algorithm.
  //       It's also possible there could be some optimizations for
  //       hexagon grid or planar graphs.
  pinned_.clear();
  if (top_hexagons_.empty()) { return; }

  std::stack<std::pair<Hexagon*, Hexagon*>> preorderStack;
  std::stack<Hexagon*> postorderStack;

  Hexagon* root = &hexagons_[*(top_hexagons_.begin())];
  SPIEL_DCHECK_NE(root->bug.order, -1);

  std::pair<Hexagon*, Hexagon*> hexagon_pair(root, NULL);
  preorderStack.push(hexagon_pair);

  int num = 0;
  // Pre-order traversal
  // Builds DFS tree
  while (!preorderStack.empty()) {
    std::pair<Hexagon*, Hexagon*> hexagon_pair = preorderStack.top();
    Hexagon* curNode = hexagon_pair.first;
    Hexagon* parent = hexagon_pair.second;
    preorderStack.pop();
    SPIEL_DCHECK_NE(curNode->bug.order, -1);

    if (!curNode->bug.visited) {
      curNode->bug.visited = true;
      postorderStack.push(curNode);

      curNode->bug.num = num++;
      curNode->bug.low = curNode->bug.num;
      curNode->bug.children = 0;

      if (parent != NULL) {
        curNode->bug.parent = parent->bug.idx;
        parent->bug.children++;
      } else {
        curNode->bug.parent = -1;
      }
    }

    for (OffsetIdx n_idx : curNode->loc.neighbours) {
      if (board_[n_idx].bug_idx == (BugIdx) -1) { continue; }
      Hexagon* n = &hexagons_[board_[n_idx].bug_idx];

      if (!n->bug.visited) {
        preorderStack.push(std::pair(n, curNode));
      }
    }
  }
  root->bug.parent = (BugIdx) -1;
  // Post-order traversal
  // Caluclates articulation points based on DFS tree structure
  while(!postorderStack.empty()) {
    Hexagon* curNode = postorderStack.top();
    postorderStack.pop();

    for (OffsetIdx n_idx : curNode->loc.neighbours) {
      if (board_[n_idx].bug_idx == (BugIdx) -1) { continue; }
      Hexagon* n = &hexagons_[board_[n_idx].bug_idx];

      curNode->bug.low = std::min(curNode->bug.low, n->bug.low);

      if (n->bug.parent == curNode->bug.idx) {
        //std::cout << "n->bug=" << BugToString(n->bug) << ";";
        //std::cout << "n->bug.low=" << n->bug.low << ";\n";
        if (curNode->bug.parent != (BugIdx) -1 && n->bug.low >= curNode->bug.num) {
          //std::cout << "pinned: " << BugToString(curNode->bug) << "\n";
          pinned_.insert(curNode->bug.idx);
        }
      }
    }
    if (curNode->bug.parent == (BugIdx) -1 && curNode->bug.children > 1) {
      //std::cout << "pinned: " << BugToString(curNode->bug) << "\n";
      pinned_.insert(curNode->bug.idx);
    }
    //std::cout << "bug=" << BugToString(curNode->bug) << ";";
    //std::cout << "parent=" << BugToString(Bug(curNode->bug.parent)) << ";";
    //std::cout << "parent=" << (int) curNode->bug.parent << ";";
    //std::cout << "children=" << curNode->bug.children << ";";
    //std::cout << "num=" << curNode->bug.num << ";";
    //std::cout << "low=" << curNode->bug.low << ";\n";
    // Reset attributes for next fn call
    curNode->bug.visited = false;
  }
}

Bug HiveBoard::MoveBug(OffsetIdx from_idx, OffsetIdx to_idx) {
  Hexagon* from = &hexagons_[board_[from_idx].bug_idx];
  SPIEL_DCHECK_NE(from->bug.order, -1);

  Bug b = from->bug;

  // Remove bug
  if (from->bug.below != (uint8_t) -1) {
    Hexagon* below = &hexagons_[from->bug.below];
    below->bug.above = -1;

    top_hexagons_.insert(from->bug.below);
  }
  board_[from->loc.idx].bug_idx = from->bug.below;
  from->bug.below = -1;
  zobrist_hash ^= zobristTable[b.player][b.type][from->loc.x][from->loc.y];
  
  // Replace bug
  if (board_[to_idx].bug_idx != (uint8_t) -1) {
    Hexagon* below = &hexagons_[board_[to_idx].bug_idx];
    below->bug.above = from->bug.idx;

    top_hexagons_.erase(below->bug.idx);

    from->bug.below = below->bug.idx;
  }
  board_[to_idx].bug_idx = from->bug.idx;
  from->loc = board_[to_idx];

  zobrist_hash ^= zobristTable[to_play][b.type][from->loc.x][from->loc.y];

  CacheHexagonArea(from_idx);
  CacheHexagonArea(from->loc.idx);
  return b;
}

Bug HiveBoard::RemoveBug(OffsetIdx h_idx) {
  Hexagon* h = &hexagons_[board_[h_idx].bug_idx];
  SPIEL_DCHECK_NE(h->bug.order, -1);
  Bug b = h->bug;

  if (h->bug.below != (uint8_t) -1) {
    Hexagon* below = &hexagons_[h->bug.below];
    below->bug.above = -1;

    top_hexagons_.insert(h->bug.below);

    h->bug.below = -1;
  }

  top_hexagons_.erase(h->bug.idx);
  board_[h->loc.idx].bug_idx = h->bug.below;

  if (b.type == kBee) {
    bees_[b.player] = -1;
  }
  zobrist_hash ^= zobristTable[b.player][b.type][h->loc.x][h->loc.y];
  bug_collections_[h->bug.player].ReturnBug(h);
  CacheHexagonArea(h->loc.idx);
  return b;
}

void HiveBoard::PlaceBug(OffsetIdx h_idx, Bug b) {
  Bug bug = bug_collections_[b.player].UseBug(b.type);
  Hexagon* h = &hexagons_[bug.idx];
  SPIEL_DCHECK_NE(b.order, -1);
  SPIEL_DCHECK_EQ(h->bug.order, -1);
  h->bug = bug;

  if (board_[h_idx].bug_idx != (uint8_t) -1) {
    Hexagon* below = &hexagons_[board_[h_idx].bug_idx];
    below->bug.above = h->bug.idx;

    h->bug.below = below->bug.idx;

    top_hexagons_.erase(below->bug.idx);
  }

  board_[h_idx].bug_idx = bug.idx;
  h->loc = board_[h_idx];
  top_hexagons_.insert(bug.idx);

  zobrist_hash ^= zobristTable[to_play][bug.type][h->loc.x][h->loc.y];
  if (b.type == kBee) {
    bees_[b.player] = bug.idx;
  }
  CacheHexagonArea(h->loc.idx);
}

std::vector<HiveMove> HiveBoard::LegalMoves() const {
  //std::cout << "\nLegalMoves\n";
  std::vector<HiveMove> legal_moves;
  // Player can only move bugs after the bee has been placed
  if (bees_[to_play] != (uint8_t) -1) {
    //std::cout << "found bee\n";
    for (BugIdx bug_idx : top_hexagons_) {
      Hexagon h = hexagons_[bug_idx];
      if (h.bug.player == to_play) {
        SPIEL_DCHECK_NE(h.bug.order, -1);
        GenerateMoves(h, h.bug.type, legal_moves);
      }
    }
  } else if (top_hexagons_.size() >= 2) {
    for (OffsetIdx idx : available_[to_play]) {
      legal_moves.push_back(HiveMove(kBee, idx));
    }
    // Player must place bee on the second, third, or fourth turns
    if (top_hexagons_.size() >= 6) {
      return legal_moves;
    }
  }


  // Player may always place bugs
  for (OffsetIdx idx : available_[to_play]) {
    //std::cout << "available_[i]=" << idx << "\n";
    //std::cout << "available_[i].bug=" << BugToString(GetHexagon(idx).bug) << "\n";
    // Skip kBee as that's covered in the rules above
    for (int8_t bug=1; bug < kNumBugTypes; bug++) {
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
  if (bees_[kWhite] == (uint8_t) -1 || bees_[kBlack] == (uint8_t) -1) {
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
    PlaceBug(m.to, Bug{to_play, m.bug_type, 0});
  } else {
    MoveBug(m.from, m.to);
  }
  last_moved_.push_back(board_[m.to].bug_idx);

  CachePinnedHexagons();
  CacheOutcome();
  to_play = (Player) !to_play;
}

void HiveBoard::UndoMove(HiveMove &m) {
  if (m.pass) {
    to_play = (Player) !to_play;
    return;
  } else if (m.place) {
    RemoveBug(m.to);
  } else {
    MoveBug(m.to, m.from);
  }
  last_moved_.pop_back();

  CachePinnedHexagons();
  CacheOutcome();
  to_play = (Player) !to_play;
}

}  // namespace hive
}  // namespace open_spiel
