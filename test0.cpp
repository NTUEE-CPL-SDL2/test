#include "Game.hpp"
#include "generate-note.hpp"

int main() {
    const unsigned char lanes = 4;
    const unsigned char fragments = 6;
    const unsigned int timePerFragment = 500; // ms

    Game game(lanes, fragments, timePerFragment);

    // generate 15 notes
    game.notes = generateRandomNotes(lanes, fragments, 15);

    // print notes
    std::cout << "Generated Notes (startFragment, lane, holds):\n";
    for (auto &n : game.notes) {
        std::cout << n.startFragment << ", " << (int)n.lane << ", " 
                  << (int)n.holds << "\n";
    }

    // Simulate loading fragments over time
    uint64_t now = 0;
    const uint64_t totalTime = timePerFragment * fragments * 5;

    std::cout << "\nSimulating fragment loads...\n";

    while (now <= totalTime) {
        std::cout << "Time " << now << " ms\n";

        game.loadFragment(now);

        // Print highway state
        for (unsigned lane = 0; lane < lanes; ++lane) {
            auto & [pressed, circ] = game.highway[lane];
            std::cout << "Lane " << (int)lane << ": ";
            for (std::size_t i = 0; i < circ.size(); ++i) {
                char f = circ[i];
                if (Game::isTap(f)) std::cout << "T ";
                else if (Game::holdRem(f) > 0) std::cout << "H" << Game::holdRem(f) << " ";
                else std::cout << ". ";
            }
            std::cout << "\n";
        }

        now += timePerFragment;
        std::cout << "--------------------\n";
    }

    return 0;
}
