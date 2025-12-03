#include <iostream>
#include <vector>
#include "Game.hpp"

// Generate test notes
std::vector<NoteData> generateNotes() {
    return {
        // lane, startFragment, holds
        {0, -1, 0},   // tap
        {1, 2, 1},    // hold 2 fragments
        {2, -1, 2},   // tap
        {3, 3, 3},    // hold 3 fragments
        {4, -1, 0},   // tap
        {5, -1, 1},   // tap
        {6, 1, 2},    // hold 1 fragment
        {7, -1, 3},   // tap
        {8, 4, 0},    // hold 4 fragments
        {10, -1, 1},  // tap
        {11, -1, 2},  // tap
        {12, 2, 3},   // hold 2 fragments
        {13, -1, 0},  // tap
        {14, -1, 1},  // tap
        {15, -1, 3},  // tap
    };
}

// Helper: print highway state
void printHighway(Game &g) {
    for(uint8_t lane=0; lane<g.lanes; lane++){
        auto &[pressed, circ] = g.highway[lane];
        std::cout << "Lane " << int(lane) << ": ";
        for(size_t i=0;i<g.fragments;i++){
            int8_t f = circ[i];
            if(Game::isTap(f)) std::cout << "T ";
            else if(Game::holdRem(f)>0) std::cout << "H" << Game::holdRem(f) << " ";
            else std::cout << ". ";
        }
        std::cout << (std::get<0>(g.highway[lane]) ? " P" : "") << "\n"; // show pressed
    }
}

int main() {
    Game g(4, 6, 500); // 4 lanes, 6 fragments, 500 ms per fragment
    g.notes = generateNotes();

    uint64_t now = 0;

    // Event plan: each tuple = (time, action, lane)
    struct Event { uint64_t time; char type; uint8_t lane; uint64_t timeDiff; };
    std::vector<Event> events = {
        {0, 'p', 0, 0},      // press lane 0 tap perfectly
        {250, 'r', 0, 250},  // release immediately
        {500, 'p', 1, 0},    // press lane 1 hold
        {1000, 'r', 1, 500}, // release early → partial
        {1500, 'p', 3, 0},   // press lane 3 hold
        {3000, 'r', 3, 1500},// release late → fully held
        {2000, 'p', 2, 0},   // press lane 2 tap late → should be bad
        {2100, 'r', 2, 100}, // release
        {2500, 'p', 0, 0},   // press tap again
        {2550, 'r', 0, 50},  // perfect
        {2700, 'p', 1, 0},   // press hold again
        {3200, 'r', 1, 500}, // partial again
        {3500, 'p', 0, 0},   // press tap too late → miss
        {4200, 'r', 0, 700}, // release
        {4500, 'p', 3, 0},   // press hold
        {5200, 'r', 3, 700}, // release early
    };

    size_t nextEvent = 0;
    uint64_t endTime = 6000;

    while(now <= endTime){
        // process events at this time
        while(nextEvent < events.size() && events[nextEvent].time <= now){
            Event &e = events[nextEvent];
            if(e.type == 'p') g.keyPressed(e.lane, now, e.timeDiff);
            else if(e.type == 'r') g.keyReleased(e.lane, now);
            nextEvent++;
        }

        // Load next fragment
        g.loadFragment(now);

        // Print state
        std::cout << "Time " << now << " ms\n";
        printHighway(g);
        std::cout << "Score: " << g.score
                  << " | Perfect: " << g.perfectCount
                  << " | Great: " << g.greatCount
                  << " | Good: " << g.goodCount
                  << " | Bad: " << g.badCount
                  << " | Miss: " << g.missCount
                  << " | Combo: " << g.combo
                  << " | MaxCombo: " << g.maxCombo
                  << "\n--------------------\n";

        now += 500;
    }

    return 0;
}