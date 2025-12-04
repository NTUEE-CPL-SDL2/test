#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#include "Game.hpp"

mystd::vector<NoteData> generateRandomNotes(std::size_t lanes,
                                            std::size_t fragments,
                                            unsigned int numNotes) {
  mystd::vector<NoteData> notes;
  std::srand((unsigned)std::time(nullptr));

  for (unsigned int i = 0; i < numNotes; ++i) {
    NoteData n;
    n.lane = std::rand() % lanes;

    // startFragment between 0 and fragments*5
    n.startFragment = std::rand() % (fragments * 5);

    // 70% tap, 30% hold
    if (std::rand() % 10 < 7) {
      n.holds = -1; // tap
    } else {
      n.holds = static_cast<int8_t>(1 + std::rand() % 5); // hold 1-5
    }
    notes.push_back(n);
  }

  // sort by startFragment
  std::sort(notes.begin(), notes.end(),
            [](const NoteData &a, const NoteData &b) {
              return a.startFragment < b.startFragment;
            });

  return notes;
}
