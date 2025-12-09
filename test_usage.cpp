#include <SDL2/SDL.h>
#include <iostream>
#include "MusicManager.hpp"
#include "ChartParser.hpp"

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  Music Manager & Chart Parser Test" << std::endl;
    std::cout << "========================================" << std::endl;

    // ========================================
    // 1. Test Music Manager
    // ========================================
    std::cout << "\n--- Testing Music Manager ---" << std::endl;
    MusicManager musicMgr;
    
    // Load music (ensure file exists)
    if (musicMgr.loadMusic("./music/test_music.mp3")) {
        std::cout << "[PASS] Music loaded successfully" << std::endl;
    } else {
        std::cout << "[WARN] Music file not found (optional)" << std::endl;
    }
    
    // Load sound effects (optional)
    std::cout << "\nLoading sound effects..." << std::endl;
    bool sfxLoaded = true;
    sfxLoaded &= musicMgr.loadSoundEffect("perfect", "./sfx/perfect.wav");
    sfxLoaded &= musicMgr.loadSoundEffect("great", "./sfx/great.wav");
    sfxLoaded &= musicMgr.loadSoundEffect("good", "./sfx/good.wav");
    sfxLoaded &= musicMgr.loadSoundEffect("miss", "./sfx/miss.wav");
    
    if (!sfxLoaded) {
        std::cout << "[WARN] Some sound effects not found (optional)" << std::endl;
    }

    // ========================================
    // 2. Test Chart Parser
    // ========================================
    std::cout << "\n--- Testing Chart Parser ---" << std::endl;
    ChartParser parser;
    
    if (!parser.load("./chart/test_chart.txt")) {
        std::cerr << "[ERROR] Failed to load chart file" << std::endl;
        std::cerr << "Please ensure ./chart/test_chart.txt exists" << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Display chart info
    parser.printChart();

    // ========================================
    // 3. Key Notes for Willie (Student A)
    // ========================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Data for Willie (Key Notes)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    const std::vector<KeyNoteData>& keyNotes = parser.getKeyNotes();
    
    std::cout << "Total key notes: " << keyNotes.size() << std::endl;
    std::cout << "\nFirst 10 notes:" << std::endl;
    std::cout << "Index\tFragment\tLane\tHolds\tTime(ms)" << std::endl;
    std::cout << "-----\t--------\t----\t-----\t--------" << std::endl;
    
    for (size_t i = 0; i < std::min(size_t(10), keyNotes.size()); i++) {
        const auto& note = keyNotes[i];
        std::cout << i << "\t"
                  << note.startFragment << "\t\t"
                  << note.lane << "\t";
        
        if (note.holds == -1) {
            std::cout << "TAP";
        } else {
            std::cout << note.holds;
        }
        
        std::cout << "\t" << parser.getFragmentTime(note.startFragment) << std::endl;
    }
    
    std::cout << "\n[INFO] Usage for Willie:" << std::endl;
    std::cout << "  for (const auto& note : keyNotes) {" << std::endl;
    std::cout << "    if (currentFragment == note.startFragment) {" << std::endl;
    std::cout << "      // Generate note at lane: note.lane (0-3)" << std::endl;
    std::cout << "      // If note.holds == -1  -> TAP note" << std::endl;
    std::cout << "      // If note.holds > 0    -> HOLD note (duration in fragments)" << std::endl;
    std::cout << "    }" << std::endl;
    std::cout << "  }" << std::endl;

    // ========================================
    // 4. Mouse Notes for GRtaun (Student B)
    // ========================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Data for GRtaun (Mouse Notes)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    const std::vector<MouseNoteData>& mouseNotes = parser.getMouseNotes();
    
    std::cout << "Total mouse notes: " << mouseNotes.size() << std::endl;
    std::cout << "\nFirst 10 objects:" << std::endl;
    std::cout << "Index\tFragment\tLane\tType\tTime(ms)" << std::endl;
    std::cout << "-----\t--------\t----\t----\t--------" << std::endl;
    
    for (size_t i = 0; i < std::min(size_t(10), mouseNotes.size()); i++) {
        const auto& obj = mouseNotes[i];
        std::cout << i << "\t"
                  << obj.startFragment << "\t\t"
                  << obj.lane << "\t"
                  << (obj.type == 0 ? "GREEN" : "RED") << "\t"
                  << parser.getFragmentTime(obj.startFragment) << std::endl;
    }
    
    std::cout << "\n[INFO] Usage for GRtaun:" << std::endl;
    std::cout << "  for (const auto& obj : mouseNotes) {" << std::endl;
    std::cout << "    if (currentFragment == obj.startFragment) {" << std::endl;
    std::cout << "      // Generate object at lane: obj.lane (0-3)" << std::endl;
    std::cout << "      // If obj.type == 0  -> GREEN (collect)" << std::endl;
    std::cout << "      // If obj.type == 1  -> RED (dodge)" << std::endl;
    std::cout << "    }" << std::endl;
    std::cout << "  }" << std::endl;

    // ========================================
    // 5. Game Configuration Parameters
    // ========================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Game Configuration" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int bpm = parser.getBPM();
    int offset = parser.getOffset();
    int fragmentsPerBeat = parser.getFragmentsPerBeat();
    double beatDuration = 60000.0 / bpm;
    double fragmentDuration = beatDuration / fragmentsPerBeat;
    
    std::cout << "BPM: " << bpm << std::endl;
    std::cout << "Offset: " << offset << " ms" << std::endl;
    std::cout << "Fragments per beat: " << fragmentsPerBeat << std::endl;
    std::cout << "Beat duration: " << beatDuration << " ms" << std::endl;
    std::cout << "Fragment duration: " << fragmentDuration << " ms" << std::endl;
    std::cout << "Music file: " << parser.getMusicFile() << std::endl;
    
    std::cout << "\n[INFO] Recommended game setup:" << std::endl;
    std::cout << "  const std::size_t SCREEN_FRAGMENTS = 10;  // Fixed screen display" << std::endl;
    std::cout << "  uint64_t MS_PER_FRAGMENT = " << (int)fragmentDuration << ";  // From chart" << std::endl;

    // ========================================
    // 6. Fragment Time Conversion Examples
    // ========================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Fragment Time Conversion" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "Fragment -> Time examples:" << std::endl;
    for (int frag = 0; frag <= 20; frag += 4) {
        std::cout << "  Fragment " << frag << " -> " 
                  << parser.getFragmentTime(frag) << " ms" << std::endl;
    }

    // ========================================
    // 7. Music Playback Test (Optional)
    // ========================================
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Music Playback Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "\nPress Enter to play music (or Ctrl+C to quit)..." << std::endl;
    std::cin.get();
    
    musicMgr.playMusic(0);  // Play once
    std::cout << "[INFO] Music playing..." << std::endl;
    
    std::cout << "Press Enter to stop music..." << std::endl;
    std::cin.get();
    
    musicMgr.stopMusic();
    std::cout << "[INFO] Music stopped" << std::endl;

    // ========================================
    // 8. Sound Effect Test (Optional)
    // ========================================
    if (sfxLoaded) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Sound Effect Test" << std::endl;
        std::cout << "========================================" << std::endl;
        
        std::cout << "\nPress Enter to test sound effects..." << std::endl;
        std::cin.get();
        
        std::cout << "Playing: Perfect" << std::endl;
        musicMgr.playSoundEffect("perfect");
        SDL_Delay(500);
        
        std::cout << "Playing: Great" << std::endl;
        musicMgr.playSoundEffect("great");
        SDL_Delay(500);
        
        std::cout << "Playing: Good" << std::endl;
        musicMgr.playSoundEffect("good");
        SDL_Delay(500);
        
        std::cout << "Playing: Miss" << std::endl;
        musicMgr.playSoundEffect("miss");
        SDL_Delay(500);
        
        std::cout << "[INFO] Sound effect test complete" << std::endl;
    }

    // Cleanup
    SDL_Quit();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Test Complete" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}