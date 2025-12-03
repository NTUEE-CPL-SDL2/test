#pragma once
#include <vector>
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include "circulate.hpp"

struct NoteData {
    std::size_t startFragment;
    int8_t holds;      // -1 = tap, >=1 = number of fragments to hold
    uint8_t lane;
};

class Game {
public:
    // =========================
    //  CONFIGURATION / STATE
    // =========================

    std::vector<NoteData> notes;       // sorted by startFragment
    uint8_t lanes;               // default = 4
    uint8_t fragments;           // visible fragments
    unsigned int timePerFragment;      // ms per fragment
    std::size_t loadNext = 0;          // next note index to load
    unsigned int duration;

    // Per lane:
    //   get<0> = bool lanePressed
    //   get<1> = circulate<int8_t, std::vector<int8_t>> (fragment buffer)
    std::vector<std::tuple<
        bool,
        extend::circulate<int8_t, std::vector<int8_t>>
    >> highway;

    // Hold sustain timing
    std::vector<uint64_t> holdPressedTime;

    // Scoring
    unsigned int score = 0;
    unsigned int perfectCount = 0, greatCount = 0, goodCount = 0, badCount = 0;
    unsigned int missCount = 0;
    unsigned int combo = 0, maxCombo = 0;

public:
    // =========================
    //  CONSTRUCTION
    // =========================
    Game(uint8_t lanes_, uint8_t fragments_, unsigned int tpf)
        : lanes(lanes_), fragments(fragments_), timePerFragment(tpf)
    {
        highway.reserve(lanes);
        for(unsigned i = 0; i < lanes; ++i) {
            highway.emplace_back(
                false,
                extend::circulate<int8_t, std::vector<int8_t>>(
                    std::vector<int8_t>(fragments, 0), // empty fragments
                    0
                )
            );
        }
        holdPressedTime.assign(lanes, 0);
    }

    // =========================
    //   FRAGMENT ENCODING HELPERS
    // =========================

    static inline bool isTap(int8_t f) { return f & 0x80; }
    static inline int holdRem(int8_t f) { return (f >> 1) & 0x3F; }
    static inline int8_t makeTap() { return 0x80; }
    static inline int8_t makeHold(int rem) {
        if(rem < 1) rem = 1;
        if(rem > 63) rem = 63;
        return static_cast<char>(rem << 1);
    }
    static inline int8_t empty() { return 0; }

    // =========================
    //   SCORING HELPERS
    // =========================

    inline void addCombo() {
        combo++;
        maxCombo = std::max(maxCombo, combo);
    }

    inline void resetCombo() {
        combo = 0;
    }

    void addTapScore(unsigned diffMs) {
        // normalize to fraction of fragment time
        double f = double(diffMs) / double(timePerFragment);

        if (f <= 0.20) { score += 1000; perfectCount++; addCombo(); }
        else if (f <= 0.40) { score += 700; greatCount++; addCombo(); }
        else if (f <= 0.60) { score += 300; goodCount++; addCombo(); }
        else if (f <= 1.00) { score += 100; badCount++; resetCombo(); }
        else { missCount++; resetCombo(); }
    }

    // =========================
    //   loadFragment()
    //   Called once every timePerFragment ms
    // =========================

    void loadFragment(uint64_t nowMs) {
        // 1. Process bottom fragments (misses + hold sustain end)
        for (unsigned lane = 0; lane < lanes; ++lane) {
            auto & [pressed, circ] = highway[lane];
            int8_t bottom = circ[circ.ssize() - 1];

            if (isTap(bottom)) {
                // missed tap
                missCount++;
                resetCombo();
            }
            else {
                int hr = holdRem(bottom);
                if (hr > 0) {
                    // hold fragment
                    if (pressed) {
                        // add sustain score for entire fragment duration
                        uint64_t held = nowMs - holdPressedTime[lane];
                        score += held;  // you may apply a factor here
                        holdPressedTime[lane] = nowMs;
                    } else {
                        // player did NOT hold → miss sustain
                        missCount++;
                        resetCombo();
                    }
                }
            }
        }

        // 2. rotate all lanes
        for (unsigned lane = 0; lane < lanes; ++lane) {
            auto & [pressed, circ] = highway[lane];
            circ.rotate(-1);
        }

        // 3. Fill new top from previous top (holdRemaining - 1)
        for (unsigned lane = 0; lane < lanes; ++lane) {
            auto & [pressed, circ] = highway[lane];
            int8_t prevTop = circ[1]; // previous fragment before rotation
            int8_t newTop = empty();

            if (holdRem(prevTop) > 1) {
                newTop = makeHold(holdRem(prevTop) - 1);
            }

            circ[0] = newTop;
        }

        // 4. Load new notes into top
        while (loadNext < notes.size() &&
               notes[loadNext].startFragment * timePerFragment <= nowMs) 
        {
            const NoteData& nd = notes[loadNext];
            auto & [pressed, circ] = highway[nd.lane];

            if (nd.holds < 0) {
                circ[0] = makeTap();
            } else {
                circ[0] = makeHold(nd.holds);
            }
            loadNext++;
        }
    }

    // =========================
    //   KEY PRESSED
    //   timeDiff = ms since last loadFragment()
    // =========================

    void keyPressed(uint8_t lane, uint64_t nowMs, uint64_t timeDiff) {
        auto & [pressed, circ] = highway[lane];

        pressed = true;

        int8_t bottom = circ[circ.ssize() - 1];

        if (isTap(bottom)) {
            // tap hit
            addTapScore(timeDiff);
            circ[circ.ssize() - 1] = empty();
            return;
        }

        int hr = holdRem(bottom);
        if (hr > 0) {
            // hold fragment
            if (hr == holdRem(circ[circ.ssize() - 2])) {
                // This is the FIRST fragment of the hold
                holdPressedTime[lane] = nowMs;
                addCombo();
            }
        }
    }

    // =========================
    //   KEY RELEASED
    // =========================

    void keyReleased(uint8_t lane, uint64_t nowMs) {
        auto & [pressed, circ] = highway[lane];

        pressed = false;

        int8_t bottom = circ[circ.ssize() - 1];
        int hr = holdRem(bottom);

        if (hr == 1) {
            // final fragment: tail hit
            uint64_t held = nowMs - holdPressedTime[lane];
            score += held;  // tail bonus (apply factor if needed)
            addCombo();
            circ[circ.ssize() - 1] = empty();
        }

        holdPressedTime[lane] = 0;
    }

    // =========================
    //   COLOR HELPER
    // TODO
    // =========================

    static /*SDL_Color*/ int getColor(int8_t f, bool pressed) {
        if (isTap(f)) return 1;           // red
        int hr = holdRem(f);
        if (hr == 0) return 0;            // empty
        if (!pressed) return 2;           // light green
        return 3;                         // deep green
    }
};