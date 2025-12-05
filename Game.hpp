#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "include/circulate.hpp"
#include "include/priority_queue.hpp"
#include "include/vector.hpp"

// int8_t: -1 = tap, -2 = invisible tap, >=1 = number of remaining fragments to
// hold, 0 = empty

template <class T> class game_priority_queue : public mystd::priority_queue<T> {
public:
  using mystd::priority_queue<T>::c;
};

struct NoteData {
  std::size_t startFragment;
  std::size_t lane;
  int8_t holds;
};

const uint32_t NO_LANE_EFFECT = 0u;
const uint32_t PERFECT = 1u;
const uint32_t GREAT = 1u << 1;
const uint32_t GOOD = 1u << 2;
const uint32_t BAD = 1u << 3;
const uint32_t MISS = 1u << 4;
const uint32_t HOLD_RELEASED = 1u << 5;
const uint32_t CLEAR = ~(PERFECT | GREAT | GOOD | BAD | MISS | HOLD_RELEASED);

const uint32_t COMBO = 1u;
const uint32_t SCORE = 2u;

struct Effect {
  uint32_t content;
  uint64_t endTime;
};

bool operator<(Effect lhs, Effect rhs) { return lhs.endTime > rhs.endTime; }

class Game {
public:
  mystd::vector<NoteData> notes; // sorted by startFragment
  std::size_t lanes;
  std::size_t fragments;    // visible fragments
  uint64_t msPerFragment;   // ms per fragment
  std::size_t loadNext = 0; // next note index to load

  mystd::vector<mystd::circulate<int8_t, mystd::vector<int8_t>>> highway;

  // 1: pressed, 0: not
  mystd::vector<bool> lanePressed;

  // Hold sustain timing
  mystd::vector<uint64_t> holdPressedTime;

  // Scoring, hold not counted for perfect to miss and combo
  uint32_t score = 0, perfectCount = 0, greatCount = 0, goodCount = 0,
           badCount = 0, missCount = 0, combo = 0, maxCombo = 0, heldTime = 0;

  std::size_t nowFragment = 0;

  mystd::vector<Effect> laneEffects;
  game_priority_queue<Effect> centerEffects = game_priority_queue<Effect>();

public:
  Game(std::size_t lanes_, std::size_t fragments_, uint64_t mpf)
      : lanes(lanes_), fragments(fragments_), msPerFragment(mpf) {
    highway.reserve(lanes);
    for (uint8_t i = 0; i < lanes; ++i) {
      highway.emplace_back(mystd::circulate<int8_t, mystd::vector<int8_t>>(
          mystd::vector<int8_t>(fragments, 0)));
    }
    lanePressed.assign(lanes, false);
    holdPressedTime.assign(lanes, 0);
    laneEffects.assign(lanes, {NO_LANE_EFFECT, 0});
  }

  inline void addCombo(uint64_t nowMs) {
    combo++;
    maxCombo = std::max(maxCombo, combo);
    if (combo > 1)
      centerEffects.push({COMBO, nowMs + msPerFragment * fragments});
  }

  inline void resetCombo() { combo = 0; }

  inline void clearExpiredEffects(uint64_t nowMs) {
    for (auto &i : laneEffects)
      if (i.endTime <= nowMs)
        i = {NO_LANE_EFFECT, 0};

    while (!centerEffects.empty() && centerEffects.top().endTime <= nowMs) {
      centerEffects.pop();
    }
  }

  void addTapScore(uint64_t nowMs, std::size_t lane) {
    double f =
        (double)(nowMs - nowFragment * msPerFragment) / double(msPerFragment);

    uint32_t prev = score / 1000;

    if (f <= 0.20) {
      score += 1000;
      perfectCount++;
      laneEffects[lane].content &= CLEAR;
      laneEffects[lane].content |= PERFECT;
      addCombo(nowMs);
    } else if (f <= 0.40) {
      score += 700;
      greatCount++;
      laneEffects[lane].content &= CLEAR;
      laneEffects[lane].content |= GREAT;
      addCombo(nowMs);
    } else if (f <= 0.60) {
      score += 300;
      goodCount++;
      laneEffects[lane].content &= CLEAR;
      laneEffects[lane].content |= GOOD;
      addCombo(nowMs);
    } else if (f <= 1.00) {
      score += 100;
      badCount++;
      laneEffects[lane].content &= CLEAR;
      laneEffects[lane].content |= BAD;
      resetCombo();
    }

    laneEffects[lane].endTime = nowMs + msPerFragment * fragments;

    if ((score / 1000 - prev) > 0)
      centerEffects.push({SCORE, nowMs + msPerFragment * fragments});
  }

  void addHoldScore(uint64_t nowMs, std::size_t lane) {
    uint64_t heldMs = nowMs - holdPressedTime[lane];
    heldTime += heldMs;
    double f = (double)heldMs * 400.0f / double(msPerFragment);
    laneEffects[lane].content &= CLEAR;
    laneEffects[lane].content |= HOLD_RELEASED;
    laneEffects[lane].endTime = nowMs + msPerFragment * fragments;
    uint32_t prev = score / 1000;
    score += static_cast<uint32_t>(f);
    if ((score / 1000 - prev) > 0) {
      centerEffects.push({SCORE, nowMs + msPerFragment * fragments});
      std::cout << "a";
    }
  }

  // Called every msPerFragment ms
  void loadFragment(std::function<void(Game &)> foo = nullptr,
                    bool before = true) {
    // 1. Process bottom fragments (misses + hold sustain end)
    for (std::size_t lane = 0; lane < lanes; ++lane) {
      int8_t &bottom = highway[lane].back();
      uint64_t nowMs = (nowFragment + 1) * msPerFragment;
      if (bottom < 0) { // tap
        missCount++;
        laneEffects[lane].content &= CLEAR;
        laneEffects[lane].content |= MISS;
        laneEffects[lane].endTime = nowMs + msPerFragment * fragments;
        resetCombo();
        bottom = 0;
      } else if (bottom > 0) { // hold
        if (lanePressed[lane]) {
          addHoldScore(nowMs, lane);
          holdPressedTime[lane] = nowMs;
        }
        bottom = 0;
      }
    }

    if (before && foo)
      foo(*this);

    // 2. Rotate all lanes
    for (std::size_t lane = 0; lane < lanes; ++lane) {
      highway[lane].rotate(-1);
    }

    // 3. Fill new top from previous top (holds - 1)
    for (std::size_t lane = 0; lane < lanes; ++lane) {
      highway[lane][0] = (highway[lane][1] > 1) ? highway[lane][1] - 1 : 0;
    }

    // 4. Load new notes into top
    while (loadNext < notes.size() &&
           notes[loadNext].startFragment == nowFragment) {
      const NoteData &nd = notes[loadNext];
      if (nd.lane < lanes)
        highway[nd.lane][0] = nd.holds;
      loadNext++;
    }
    nowFragment++;

    if (!before && foo)
      foo(*this);
  }

  void keyPressed(std::size_t lane, uint64_t nowMs) {
    lanePressed[lane] = true;
    int8_t &bottom = highway[lane].back();

    if (bottom < 0) { // tap hit
      addTapScore(nowMs, lane);
      bottom = 0;
      return;
    } else if (bottom > 0) { // hold pressed
      holdPressedTime[lane] = nowMs;
    }
  }

  void keyReleased(std::size_t lane, uint64_t nowMs) {
    lanePressed[lane] = false;
    int8_t &bottom = highway[lane].back();

    if (bottom > 0) { // hold released
      addHoldScore(nowMs, lane);
      bottom = 0;
    }
  }
};