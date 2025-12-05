#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "include/circulate.hpp"
#include "include/vector.hpp"

// int8_t: -1 = tap, -2 = invisible tap, >=1 = number of remaining fragments to
// hold, 0 = empty

struct NoteData {
  std::size_t startFragment;
  std::size_t lane;
  int8_t holds;
};

enum LaneEffect : uint32_t {
  NO_EFFECT = 0u,
  PERFECT = 1u,
  GREAT = 1u << 1,
  GOOD = 1u << 2,
  BAD = 1u << 3,
  MISS = 1u << 4,
  HOLD_RELEASED = 1u << 5,
  MASK = PERFECT | GREAT | GOOD | BAD | MISS | HOLD_RELEASED,
  CLEAR = ~MASK
};

enum CenterEffect : uint32_t { NO_EFFECT = 0u, COMBO = 1u, SCORE = 1u << 1 };

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

  mystd::vector<LaneEffect> laneEffects;
  CenterEffect centerEffect;

public:
  Game(std::size_t lanes_, std::size_t fragments_, uint64_t mpf)
      : lanes(lanes_), fragments(fragments_), msPerFragment(mpf),
        centerEffect(NO_EFFECT) {
    highway.reserve(lanes);
    for (uint8_t i = 0; i < lanes; ++i) {
      highway.emplace_back(mystd::circulate<int8_t, mystd::vector<int8_t>>(
          mystd::vector<int8_t>(fragments, 0)));
    }
    lanePressed.assign(lanes, false);
    holdPressedTime.assign(lanes, 0);
    laneEffects.assign(lanes, NO_EFFECT);
  }

  inline void addCombo() {
    combo++;
    maxCombo = std::max(maxCombo, combo);
    centerEffect |= COMBO;
  }

  inline void resetCombo() { combo = 0; }

  inline void clearEffects() {
    for (auto &i : laneEffects)
      i = NO_EFFECT;
    centerEffect = NO_EFFECT;
  }

  void addTapScore(int64_t diffMs, std::size_t lane) {
    double f = double(std::abs(diffMs)) / double(msPerFragment);

    uint32_t prev = score / 1000;

    if (f <= 0.20) {
      score += 1000;
      perfectCount++;
      laneEffects[lane] &= CLEAR;
      laneEffects[lane] |= PERFECT;
      addCombo();
    } else if (f <= 0.40) {
      score += 700;
      greatCount++;
      laneEffects[lane] &= CLEAR;
      laneEffects[lane] |= GREAT;
      addCombo();
    } else if (f <= 0.60) {
      score += 300;
      goodCount++;
      laneEffects[lane] &= CLEAR;
      laneEffects[lane] |= GOOD;
      addCombo();
    } else if (f <= 1.00) {
      score += 100;
      badCount++;
      laneEffects[lane] &= CLEAR;
      laneEffects[lane] |= BAD;
      resetCombo();
    } else {
      missCount++;
      laneEffects[lane] &= CLEAR;
      laneEffects[lane] |= MISS;
      resetCombo();
    }

    if ((score / 1000 - prev) > 0)
      centerEffect |= SCORE;
  }

  void addHoldScore(int64_t heldMs, std::size_t lane) {
    heldTime += heldMs;
    double f = double(std::abs(heldMs)) * 400.0f / double(msPerFragment);
    laneEffects[lane] &= CLEAR;
    laneEffects[lane] |= HOLD_RELEASED;
    uint32_t prev = score / 1000;
    score += static_cast<uint32_t>(f);
    if ((score / 1000 - prev) > 0)
      centerEffect |= SCORE;
  }

  // Called once every msPerFragment ms
  void loadFragment(void (*foo)(Game &) = nullptr) {
    // 1. Process bottom fragments (misses + hold sustain end)
    for (std::size_t lane = 0; lane < lanes; ++lane) {
      int8_t &bottom = highway[lane].back();
      if (bottom < 0) { // tap
        missCount++;
        resetCombo();
        bottom = 0;
      } else if (bottom > 0) { // hold
        if (lanePressed[lane]) {
          uint64_t nowMs = (nowFragment + 1) * msPerFragment;
          int64_t heldMs = nowMs - holdPressedTime[lane];
          addHoldScore(heldMs, lane);
          holdPressedTime[lane] = nowMs;
        }
        bottom = 0;
      }
    }

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

    // 5. Foo
    if (foo)
      foo(*this);
  }

  void keyPressed(std::size_t lane, uint64_t nowMs) {
    lanePressed[lane] = true;
    int8_t &bottom = highway[lane].back();

    if (bottom < 0) { // tap hit
      addTapScore(nowMs - nowFragment * msPerFragment, lane);
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
      int64_t heldMs = nowMs - holdPressedTime[lane];
      addHoldScore(heldMs, lane);
      bottom = 0;
    }
  }
};