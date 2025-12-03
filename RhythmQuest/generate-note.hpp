#include <vector>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include "Game.hpp"

std::vector<NoteData> generateRandomNotes(
    uint8_t lanes,
    uint8_t fragments,
    unsigned int numNotes
) {
    std::vector<NoteData> notes;
    std::srand((unsigned)std::time(nullptr));

    for (unsigned i = 0; i < numNotes; ++i) {
        NoteData n;
        n.lane = std::rand() % lanes;

        // startFragment between 0 and fragments*5
        n.startFragment = std::rand() % (fragments * 5);

        // 70% tap, 30% hold
        if (std::rand() % 10 < 7) {
            n.holds = -1; // tap
        } else {
            n.holds = static_cast<int8_t>(1 + std::rand() % 4); // hold 1~4
        }
        notes.push_back(n);
    }

    // sort by startFragment
    std::sort(notes.begin(), notes.end(), [](const NoteData &a, const NoteData &b) {
        return a.startFragment < b.startFragment;
    });

    return notes;
}
