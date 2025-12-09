#include <SDL2/SDL.h>
#include <iostream>
#include "MusicManager.hpp"
#include "ChartParser.hpp"

int main(int argc, char* argv[]) {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::cout << "=== MusicManager and ChartParser testing ===" << std::endl;

    // ========================================
    // 1. 音樂管理器測試
    // ========================================
    std::cout << "\n--- testing MusicManager ---" << std::endl;
    MusicManager musicMgr;
    
    // 載入音樂（請確保檔案存在）
    if (musicMgr.loadMusic("./music/test_music.mp3")) {
        std::cout << "[TEST] Music loaded successfully" << std::endl;
    }
    
    // 載入音效（可選）
    musicMgr.loadSoundEffect("perfect", "./sfx/perfect.wav");
    musicMgr.loadSoundEffect("great", "./sfx/great.wav");
    musicMgr.loadSoundEffect("good", "./sfx/good.wav");
    musicMgr.loadSoundEffect("miss", "./sfx/miss.wav");

    // ========================================
    // 2. 譜面解析器測試
    // ========================================
    std::cout << "\n--- testing ChartParser ---" << std::endl;
    ChartParser parser;
    
    if (!parser.load("./chart/test_chart.txt")) {
        std::cerr << "[ERROR] Failed to load chart" << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // 顯示譜面資訊
    parser.printChart();

    // ========================================
    // 3. 整合測試：同學A的使用方式
    // ========================================
    std::cout << "\n--- example usage for Willie ---" << std::endl;
    const std::vector<KeyNoteData>& keyNotes = parser.getKeyNotes();
    
    std::cout << "number of notes: " << keyNotes.size() << std::endl;
    std::cout << "Example Data：" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), keyNotes.size()); i++) {
        std::cout << "  Note " << i << ": ";
        std::cout << "startFragment=" << keyNotes[i].startFragment << ", ";
        std::cout << "lane=" << keyNotes[i].lane << ", ";
        std::cout << "holds=" << (int)keyNotes[i].holds;
        std::cout << " (time=" << parser.getFragmentTime(keyNotes[i].startFragment) << "ms)";
        std::cout << std::endl;
    }
    
    // 同學A可以這樣用：
    // for (const auto& note : keyNotes) {
    //     if (currentFragment == note.startFragment) {
    //         // 在 note.lane 軌道生成音符
    //         // 如果 note.holds == -1 → TAP
    //         // 如果 note.holds > 0 → HOLD，持續 note.holds 個 fragments
    //     }
    // }

    // ========================================
    // 4. 整合測試：同學B的使用方式
    // ========================================
    std::cout << "\n--- example usage for GRtaun ---" << std::endl;
    const std::vector<MouseNoteData>& mouseNotes = parser.getMouseNotes();
    
    std::cout << "number of notes: " << mouseNotes.size() << std::endl;
    std::cout << "Example Data：" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), mouseNotes.size()); i++) {
        std::cout << "  Object " << i << ": ";
        std::cout << "startFragment=" << mouseNotes[i].startFragment << ", ";
        std::cout << "x=" << mouseNotes[i].x << ", ";
        std::cout << "type=" << (mouseNotes[i].type == 0 ? "GREEN(eat)" : "RED(dodge)");
        std::cout << " (time=" << parser.getFragmentTime(mouseNotes[i].startFragment) << "ms)";
        std::cout << std::endl;
    }
    
    // GRtaun可以這樣用：
    // for (const auto& obj : mouseNotes) {
    //     if (currentFragment == obj.startFragment) {
    //         // 在 X=obj.x 的位置生成物件
    //         // 如果 obj.type == 0 → 綠色拾取物
    //         // 如果 obj.type == 1 → 紅色障礙物
    //     }
    // }

    // ========================================
    // 5. 遊戲參數
    // ========================================
    std::cout << "\n--- game parameter ---" << std::endl;
    int bpm = parser.getBPM();
    int fragmentsPerBeat = parser.getFragmentsPerBeat();
    double beatDuration = 60000.0 / bpm;
    double fragmentDuration = beatDuration / fragmentsPerBeat;
    
    std::cout << "BPM: " << bpm << std::endl;
    std::cout << "time per beat: " << beatDuration << "ms" << std::endl;
    std::cout << "Fragments/Beat: " << fragmentsPerBeat << std::endl;
    std::cout << "time per Fragment: " << fragmentDuration << "ms" << std::endl;
    
    /*
    std::cout << "\n建議遊戲配置：" << std::endl;
    std::cout << "  const std::size_t SCREEN_FRAGMENTS = 10;  // 螢幕顯示10格（固定）" << std::endl;
    std::cout << "  uint64_t MS_PER_FRAGMENT = " << (int)fragmentDuration << ";  // 從譜面計算" << std::endl;
    */

    // ========================================
    // 6. 音樂播放測試
    // ========================================
    std::cout << "\n--- tesing music playing ---" << std::endl;
    std::cout << "Press enter to play \"test_music.mp3\"(or press ctrl+C to quit)..." << std::endl;
    std::cin.get();
    
    musicMgr.playMusic(0);  // 播放一次
    
    std::cout << "music playing, press Enter to stop music" << std::endl;
    std::cin.get();
    
    musicMgr.stopMusic();
    std::cout << "music stopped" << std::endl;

    // 清理
    SDL_Quit();
    
    std::cout << "\n=== test complete ===" << std::endl;
    return 0;
}