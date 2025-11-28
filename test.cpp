#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <map>
#include <iostream>
/*
run command
g++ test.cpp -o test.exe -IC:/SDL2/include -LC:/SDL2/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer
./game.exe
*/


// ============================================
// MusicManager Class - 音樂管理系統
// ============================================
class MusicManager {
private:
    Mix_Music* currentMusic;                    // 當前播放的背景音樂
    std::map<std::string, Mix_Chunk*> sfxMap;   // 音效庫（存放多個音效）
    bool initialized;                           // 是否已初始化
    int musicVolume;                            // 音樂音量 (0-128)
    int sfxVolume;                              // 音效音量 (0-128)
    Uint32 musicStartTime;                      // 音樂開始播放的時間戳記
    bool isPaused;                              // 是否暫停中

public:
    // 建構子
    MusicManager() 
        : currentMusic(nullptr), initialized(false), 
          musicVolume(MIX_MAX_VOLUME), sfxVolume(MIX_MAX_VOLUME),
          musicStartTime(0), isPaused(false) {
        init();
    }

    // 解構子 - 清理所有資源
    ~MusicManager() {
        cleanup();
    }

    // ========================================
    // 初始化 SDL_mixer
    // ========================================
    bool init() {
        if (initialized) return true;

        // 初始化 SDL_mixer
        // 參數: 頻率, 格式, 聲道數, 緩衝區大小
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "[ERROR] SDL_mixer 初始化失敗: " << Mix_GetError() << std::endl;
            return false;
        }

        // 設定可以同時播放的音效數量
        Mix_AllocateChannels(16);

        initialized = true;
        std::cout << "[OK] MusicManager 初始化成功" << std::endl;
        return true;
    }

    // ========================================
    // 載入背景音樂
    // 路徑: ./music/song.mp3
    // ========================================
    bool loadMusic(const std::string& filepath) {
        // 釋放舊的音樂
        if (currentMusic) {
            Mix_FreeMusic(currentMusic);
            currentMusic = nullptr;
        }

        // 載入新音樂
        currentMusic = Mix_LoadMUS(filepath.c_str());
        if (!currentMusic) {
            std::cerr << "[ERROR] 無法載入音樂: " << filepath << std::endl;
            std::cerr << "        SDL_mixer Error: " << Mix_GetError() << std::endl;
            return false;
        }

        std::cout << "[OK] 音樂載入成功: " << filepath << std::endl;
        return true;
    }

    // ========================================
    // 播放背景音樂
    // loops: -1 = 無限循環, 0 = 播放一次, n = 播放 n+1 次
    // ========================================
    void playMusic(int loops = -1) {
        if (!currentMusic) {
            std::cerr << "[ERROR] 沒有載入音樂" << std::endl;
            return;
        }

        if (Mix_PlayMusic(currentMusic, loops) == -1) {
            std::cerr << "[ERROR] 音樂播放失敗: " << Mix_GetError() << std::endl;
            return;
        }

        musicStartTime = SDL_GetTicks();
        isPaused = false;
        std::cout << "[INFO] 音樂開始播放" << std::endl;
    }

    // ========================================
    // 暫停/恢復音樂
    // ========================================
    void pauseMusic() {
        if (Mix_PlayingMusic() && !isPaused) {
            Mix_PauseMusic();
            isPaused = true;
            std::cout << "[INFO] 音樂暫停" << std::endl;
        }
    }

    void resumeMusic() {
        if (isPaused) {
            Mix_ResumeMusic();
            isPaused = false;
            std::cout << "[INFO] 音樂恢復播放" << std::endl;
        }
    }

    // ========================================
    // 停止音樂
    // ========================================
    void stopMusic() {
        Mix_HaltMusic();
        isPaused = false;
        std::cout << "[INFO] 音樂停止" << std::endl;
    }

    // ========================================
    // 設定音樂音量 (0-128)
    // ========================================
    void setMusicVolume(int volume) {
        musicVolume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : volume);
        Mix_VolumeMusic(musicVolume);
        std::cout << "[INFO] 音樂音量設定為: " << musicVolume << std::endl;
    }

    // ========================================
    // 取得當前音樂播放時間（毫秒）
    // 用於音符生成的時間同步
    // ========================================
    Uint32 getMusicTime() {
        if (!Mix_PlayingMusic() || isPaused) {
            return 0;
        }
        return SDL_GetTicks() - musicStartTime;
    }

    // ========================================
    // 載入音效
    // 路徑: ./sfx/hit.wav
    // name: 音效的識別名稱（如 "perfect", "great", "miss"）
    // ========================================
    bool loadSoundEffect(const std::string& name, const std::string& filepath) {
        Mix_Chunk* sound = Mix_LoadWAV(filepath.c_str());
        if (!sound) {
            std::cerr << "[ERROR] 無法載入音效: " << filepath << std::endl;
            std::cerr << "        SDL_mixer Error: " << Mix_GetError() << std::endl;
            return false;
        }

        // 如果已存在同名音效，先釋放
        auto it = sfxMap.find(name);
        if (it != sfxMap.end()) {
            Mix_FreeChunk(it->second);
        }

        sfxMap[name] = sound;
        std::cout << "[OK] 音效載入成功: " << name << " (" << filepath << ")" << std::endl;
        return true;
    }

    // ========================================
    // 播放音效
    // ========================================
    void playSoundEffect(const std::string& name, int loops = 0) {
        auto it = sfxMap.find(name);
        if (it == sfxMap.end()) {
            std::cerr << "[ERROR] 找不到音效: " << name << std::endl;
            return;
        }

        // -1 = 自動選擇可用的音軌
        if (Mix_PlayChannel(-1, it->second, loops) == -1) {
            std::cerr << "[ERROR] 音效播放失敗: " << Mix_GetError() << std::endl;
        }
    }

    // ========================================
    // 設定音效音量 (0-128)
    // ========================================
    void setSFXVolume(int volume) {
        sfxVolume = (volume < 0) ? 0 : (volume > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : volume);
        
        // 設定所有音效的音量
        for (auto& pair : sfxMap) {
            Mix_VolumeChunk(pair.second, sfxVolume);
        }
        std::cout << "[INFO] 音效音量設定為: " << sfxVolume << std::endl;
    }

    // ========================================
    // 檢查音樂是否正在播放
    // ========================================
    bool isMusicPlaying() {
        return Mix_PlayingMusic() && !isPaused;
    }

    // ========================================
    // 淡入淡出效果
    // ========================================
    void fadeInMusic(int ms, int loops = -1) {
        if (!currentMusic) {
            std::cerr << "[ERROR] 沒有載入音樂" << std::endl;
            return;
        }
        Mix_FadeInMusic(currentMusic, loops, ms);
        musicStartTime = SDL_GetTicks();
        isPaused = false;
        std::cout << "[INFO] 音樂淡入播放 (" << ms << "ms)" << std::endl;
    }

    void fadeOutMusic(int ms) {
        Mix_FadeOutMusic(ms);
        std::cout << "[INFO] 音樂淡出 (" << ms << "ms)" << std::endl;
    }

    // ========================================
    // 清理資源
    // ========================================
    void cleanup() {
        // 釋放背景音樂
        if (currentMusic) {
            Mix_FreeMusic(currentMusic);
            currentMusic = nullptr;
        }

        // 釋放所有音效
        for (auto& pair : sfxMap) {
            Mix_FreeChunk(pair.second);
        }
        sfxMap.clear();

        // 關閉 SDL_mixer
        if (initialized) {
            Mix_CloseAudio();
            initialized = false;
        }

        std::cout << "[INFO] MusicManager 資源已清理" << std::endl;
    }
};

// ============================================
// 測試主程式
// ============================================
int main(int argc, char* argv[]) {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL 初始化失敗: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 建立視窗（用於事件處理）
    SDL_Window* window = SDL_CreateWindow(
        "Music Manager Test - Press Keys to Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // ========================================
    // 建立 MusicManager
    // ========================================
    MusicManager musicMgr;

    // ========================================
    // 載入音樂和音效
    // 📁 請將檔案放在以下路徑：
    // ========================================
    std::cout << "\n=== 音樂檔案路徑說明 ===" << std::endl;
    std::cout << "請將音樂檔案放在以下位置：" << std::endl;
    std::cout << "  ./music/song.mp3       <- 背景音樂（支援 MP3, OGG, WAV）" << std::endl;
    std::cout << "  ./sfx/perfect.wav      <- 完美判定音效" << std::endl;
    std::cout << "  ./sfx/great.wav        <- 良好判定音效" << std::endl;
    std::cout << "  ./sfx/good.wav         <- 普通判定音效" << std::endl;
    std::cout << "  ./sfx/miss.wav         <- 失誤判定音效" << std::endl;
    std::cout << "==========================\n" << std::endl;

    // 載入背景音樂（請改成你的實際檔案路徑）
    if (!musicMgr.loadMusic("./music/unity.mp3")) {
        std::cout << "[WARNING] 無法載入背景音樂，請確認檔案是否存在" << std::endl;
    }

    // 載入音效
    musicMgr.loadSoundEffect("perfect", "./sfx/perfect.wav");
    musicMgr.loadSoundEffect("great", "./sfx/great.wav");
    musicMgr.loadSoundEffect("good", "./sfx/good.wav");
    musicMgr.loadSoundEffect("miss", "./sfx/miss.wav");

    // ========================================
    // 操作說明
    // ========================================
    std::cout << "\n=== 操作說明 ===" << std::endl;
    std::cout << "空白鍵 (SPACE)  - 播放/暫停音樂" << std::endl;
    std::cout << "S 鍵            - 停止音樂" << std::endl;
    std::cout << "1-4 鍵          - 播放不同判定音效" << std::endl;
    std::cout << "↑/↓ 方向鍵      - 調整音樂音量" << std::endl;
    std::cout << "F 鍵            - 淡入播放音樂" << std::endl;
    std::cout << "ESC / 關閉視窗  - 退出程式" << std::endl;
    std::cout << "=================\n" << std::endl;

    // 主迴圈
    bool running = true;
    SDL_Event e;
    bool musicPlaying = false;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } 
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;

                    case SDLK_p:
                        // 播放/暫停音樂
                        if (musicPlaying) {
                            musicMgr.pauseMusic();
                            musicPlaying = false;
                        } else {
                            if (musicMgr.isMusicPlaying()) {
                                musicMgr.resumeMusic();
                            } else {
                                musicMgr.playMusic();
                            }
                            musicPlaying = true;
                        }
                        break;

                    case SDLK_s:
                        // 停止音樂
                        musicMgr.stopMusic();
                        musicPlaying = false;
                        break;

                    case SDLK_f:
                        // 淡入播放
                        musicMgr.fadeInMusic(2000); // 2秒淡入
                        musicPlaying = true;
                        break;

                    case SDLK_1:
                        musicMgr.playSoundEffect("perfect");
                        std::cout << "[播放] PERFECT 音效" << std::endl;
                        break;

                    case SDLK_2:
                        musicMgr.playSoundEffect("great");
                        std::cout << "[播放] GREAT 音效" << std::endl;
                        break;

                    case SDLK_3:
                        musicMgr.playSoundEffect("good");
                        std::cout << "[播放] GOOD 音效" << std::endl;
                        break;

                    case SDLK_4:
                        musicMgr.playSoundEffect("miss");
                        std::cout << "[播放] MISS 音效" << std::endl;
                        break;

                    case SDLK_UP:
                        musicMgr.setMusicVolume(MIX_MAX_VOLUME); // 最大音量
                        break;

                    case SDLK_DOWN:
                        musicMgr.setMusicVolume(MIX_MAX_VOLUME / 2); // 半音量
                        break;
                }
            }
        }

        // 顯示當前音樂時間
        if (musicMgr.isMusicPlaying()) {
            Uint32 musicTime = musicMgr.getMusicTime();
            // 可以用這個時間來同步譜面
        }

        // 簡單的背景
        SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
        SDL_RenderClear(renderer);

        // 顯示狀態指示
        if (musicMgr.isMusicPlaying()) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // 綠色 = 播放中
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 紅色 = 停止
        }
        SDL_Rect statusRect = {350, 250, 100, 100};
        SDL_RenderFillRect(renderer, &statusRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // 清理資源
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
