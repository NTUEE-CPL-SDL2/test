#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <vector>
#include "circulate.hpp"

struct NoteData {
    std::size_t startFragment;
    std::size_t lane;
    int8_t holds; // -1 = tap, >=1 = number of fragments to hold
};

class Game {
public:
    std::vector<NoteData> notes; // sorted by startFragment
    std::size_t lanes;
    std::size_t fragments; // visible fragments
    uint64_t msPerFragment; // ms per fragment
    std::size_t loadNext = 0; // next note index to load

    std::vector<extend::circulate<int8_t, std::vector<int8_t>>> highway;

    // 1: pressed, 0: not
    std::vector<bool> lanePressed;

    // Hold sustain timing
    std::vector<uint64_t> holdPressedTime;

    // Scoring, hold not counted for perfect to miss and combo
    uint64_t score = 0, perfectCount = 0, greatCount = 0, goodCount = 0, badCount = 0, missCount = 0, combo = 0, maxCombo = 0, heldTime = 0;

    std::size_t nowFragment = 0;

public:
    Game(std::size_t lanes_, std::size_t fragments_, uint64_t mpf)
        : lanes(lanes_), fragments(fragments_), msPerFragment(mpf)
    {
        highway.reserve(lanes);
        for(uint8_t i = 0; i < lanes; ++i) {
            highway.emplace_back(extend::circulate<int8_t, std::vector<int8_t>>(std::vector<int8_t>(fragments, 0)));
        }
        lanePressed.assign(lanes, false);
        holdPressedTime.assign(lanes, 0);
    }

    inline void addCombo() {
        combo++;
        maxCombo = std::max(maxCombo, combo);
    }

    inline void resetCombo() {
        combo = 0;
    }

    void addTapScore(uint64_t diffMs) {
        double f = double(diffMs) / double(msPerFragment);

        if (f <= 0.20) { score += 1000; perfectCount++; addCombo(); }
        else if (f <= 0.40) { score += 700; greatCount++; addCombo(); }
        else if (f <= 0.60) { score += 300; goodCount++; addCombo(); }
        else if (f <= 1.00) { score += 100; badCount++; resetCombo(); }
        else { missCount++; resetCombo(); }
    }

    void addHoldScore(uint64_t heldMs) {
          heldTime += heldMs;
          double f = double(heldMs) * 400.0f / double(msPerFragment);
          score += static_cast<uint64_t>(f);
    }

    // Called once every msPerFragment ms
    void loadFragment() {
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
                    uint64_t held = nowMs - holdPressedTime[lane];
                    addHoldScore(held);
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
               notes[loadNext].startFragment == nowFragment)
        {
            const NoteData& nd = notes[loadNext];
            if (nd.lane < lanes) highway[nd.lane][0] = nd.holds;
            loadNext++;
        }
        nowFragment++;
    }

    void keyPressed(std::size_t lane, uint64_t nowMs) {
        lanePressed[lane] = true;
        int8_t &bottom = highway[lane].back();

        if (bottom < 0) { // tap hit
            addTapScore(nowMs - nowFragment * msPerFragment);
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
            uint64_t held = nowMs - holdPressedTime[lane];
            addHoldScore(held);
            bottom = 0;
        }
    }
};