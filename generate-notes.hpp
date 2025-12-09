#include <cstdint>
#include <cstdlib>
#include <ctime>

#include "include/qsort.hpp"
#include "include/vector.hpp"

#include "Game.hpp"

mystd::vector<NoteData> generateRandomNotes(std::size_t lanes,
                                            std::size_t fragments,
                                            unsigned int numNotes,
                                            unsigned int tapPercent = 70) {
  mystd::vector<NoteData> notes;

  if (lanes == 0 || fragments == 0 || numNotes == 0) {
    return notes;
  }

  std::srand((unsigned)std::time(nullptr));

  for (unsigned int i = 0; i < numNotes; ++i) {
    NoteData n;
    n.lane = std::rand() % lanes;
    n.startFragment = std::rand() % (fragments * 5);

    if (std::rand() % 100 < tapPercent) {
      n.holds = -1; // tap
    } else {
      n.holds = static_cast<int8_t>(1 + std::rand() % 5); // hold 1-5
    }
    notes.push_back(n);
  }

  qsort(notes.begin(), notes.end(), [](const NoteData &a, const NoteData &b) {
    return a.startFragment < b.startFragment;
  });

  return notes;
}
