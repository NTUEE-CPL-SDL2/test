#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <sstream>
#include <cmath>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int RAILS = 4;
const int NOTE_WIDTH = 80;
const int NOTE_HEIGHT = 20;
const int LONG_NOTE_HEIGHT = 200;
const int NOTE_SPEED = 5;
const int SPAWN_INTERVAL = 30;
const int HIT_ZONE = 50;

// Judgment window sizes (timing thresholds)
const int JUDGE_PERFECT = 15;
const int JUDGE_GREAT = 30;
const int JUDGE_GOOD = 45;

// 判定等級
enum Judgment {
    JUDGMENT_NONE,
    JUDGMENT_MISS,
    JUDGMENT_GOOD,
    JUDGMENT_GREAT,
    JUDGMENT_PERFECT
};

// 音符類型
enum NoteType {
    NOTE_TAP,
    NOTE_HOLD,
    NOTE_GREEN,
    NOTE_SLIDE,
    NOTE_OBSTACLE
};

// 遊戲狀態
enum GameState { 
    STATE_MENU, 
    STATE_COUNTDOWN, 
    STATE_PLAYING, 
    STATE_RESULTS 
};

struct Note {
    int rail;
    int y;
    NoteType type;
    int length;
    bool active;
    bool holding;
    Uint32 spawnTime;
    bool judged;
};

struct JudgmentEffect {
    Judgment type;
    int rail;
    int timer;
    int x, y;
};

// 玩家角色
struct Player {
    float x;
    float y;
    float targetX;
    bool isMoving;
    int animationFrame;
    Uint32 lastAnimationTime;
    bool isJumping;
    bool isDucking;
    float jumpVelocity;
};

class ScoreBar {
private:
    int score;
    int combo;
    int maxCombo;
    SDL_Renderer* renderer;
    TTF_Font* font;
    
public:
    ScoreBar(SDL_Renderer* renderer, TTF_Font* font) 
        : score(0), combo(0), maxCombo(0), renderer(renderer), font(font) {}
    
    void addScore(Judgment judgment) {
        int points = 0;
        switch(judgment) {
            case JUDGMENT_PERFECT: points = 100; break;
            case JUDGMENT_GREAT: points = 75; break;
            case JUDGMENT_GOOD: points = 50; break;
            default: points = 0; break;
        }
        
        // Combo加成
        if (combo >= 50) points *= 2;
        else if (combo >= 30) points = points * 3 / 2;
        else if (combo >= 10) points = points * 6 / 5;
        
        score += points;
        
        if (judgment != JUDGMENT_MISS && judgment != JUDGMENT_NONE) {
            combo++;
            maxCombo = std::max(maxCombo, combo);
        } else {
            combo = 0;
        }
    }
    
    void reset() {
        score = 0;
        combo = 0;
        maxCombo = 0;
    }
    
    void render() {
        if (!font) return;
        
        SDL_Color color;
        if (combo >= 50) color = {255, 215, 0, 255}; // 金色
        else if (combo >= 30) color = {255, 165, 0, 255}; // 橙色
        else color = {173, 216, 230, 255}; // 淺藍色
        
        // 顯示分數
        std::string scoreText = "Score: " + std::to_string(score);
        SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreText.c_str(), color);
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL_Rect scoreRect = {20, 20, scoreSurface->w, scoreSurface->h};
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        
        // 顯示連擊
        if (combo > 0) {
            std::string comboText = "Combo: " + std::to_string(combo);
            SDL_Surface* comboSurface = TTF_RenderText_Solid(font, comboText.c_str(), color);
            SDL_Texture* comboTexture = SDL_CreateTextureFromSurface(renderer, comboSurface);
            SDL_Rect comboRect = {20, 60, comboSurface->w, comboSurface->h};
            SDL_RenderCopy(renderer, comboTexture, NULL, &comboRect);
            SDL_FreeSurface(comboSurface);
            SDL_DestroyTexture(comboTexture);
        }
        
        SDL_FreeSurface(scoreSurface);
        SDL_DestroyTexture(scoreTexture);
    }
    
    int getScore() const { return score; }
    int getCombo() const { return combo; }
    int getMaxCombo() const { return maxCombo; }
};

class Decoration {
private:
    SDL_Renderer* renderer;
    int combo;
    bool active;
    
public:
    Decoration(SDL_Renderer* renderer) 
        : renderer(renderer), combo(0), active(false) {}
    
    void update(int currentCombo) {
        combo = currentCombo;
        active = (combo >= 20);
    }
    
    void render() {
        if (!active) return;
        
        // 根據連擊數顯示不同裝飾
        if (combo >= 50) {
            // 金色賽博風格裝飾
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 100);
            for (int i = 0; i < 5; i++) {
                SDL_Rect rect = {100 + i * 120, 100, 80, 400};
                SDL_RenderFillRect(renderer, &rect);
            }
        } else if (combo >= 30) {
            // 橙色裝飾
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
            for (int i = 0; i < 3; i++) {
                SDL_Rect rect = {200 + i * 200, 150, 60, 300};
                SDL_RenderFillRect(renderer, &rect);
            }
        } else if (combo >= 20) {
            // 淺藍色觀眾效果
            SDL_SetRenderDrawColor(renderer, 173, 216, 230, 60);
            for (int i = 0; i < 8; i++) {
                SDL_Rect rect = {50 + i * 90, 500, 40, 80};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
};

class MusicManager {
private:
    Mix_Music* backgroundMusic;
    std::map<std::string, Mix_Chunk*> soundEffects;
    bool musicLoaded;
    
public:
    MusicManager() : backgroundMusic(nullptr), musicLoaded(false) {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        }
    }
    
    ~MusicManager() {
        if (backgroundMusic) {
            Mix_FreeMusic(backgroundMusic);
        }
        for (auto& sfx : soundEffects) {
            Mix_FreeChunk(sfx.second);
        }
        Mix_CloseAudio();
    }
    
    bool loadMusic(const std::string& filePath) {
        backgroundMusic = Mix_LoadMUS(filePath.c_str());
        if (!backgroundMusic) {
            printf("Failed to load music! SDL_mixer Error: %s\n", Mix_GetError());
            return false;
        }
        musicLoaded = true;
        return true;
    }
    
    bool loadSoundEffect(const std::string& name, const std::string& filePath) {
        Mix_Chunk* sound = Mix_LoadWAV(filePath.c_str());
        if (!sound) {
            printf("Failed to load sound effect! SDL_mixer Error: %s\n", Mix_GetError());
            return false;
        }
        soundEffects[name] = sound;
        return true;
    }
    
    void playMusic() {
        if (musicLoaded) {
            Mix_PlayMusic(backgroundMusic, -1);
        }
    }
    
    void playSoundEffect(const std::string& name) {
        auto it = soundEffects.find(name);
        if (it != soundEffects.end()) {
            Mix_PlayChannel(-1, it->second, 0);
        }
    }
    
    void stopMusic() {
        Mix_HaltMusic();
    }
};

class NoteManager {
private:
    std::vector<Note> notes;
    std::vector<JudgmentEffect> judgmentEffects;
    SDL_Renderer* renderer;
    TTF_Font* font;
    
public:
    NoteManager(SDL_Renderer* renderer, TTF_Font* font) 
        : renderer(renderer), font(font) {}
    
    void addNote(int rail, NoteType type, int length = NOTE_HEIGHT) {
        notes.push_back({rail, 0, type, length, true, false, SDL_GetTicks(), false});
    }
    
    void update() {
        // 更新音符位置
        for (auto& note : notes) {
            if (note.active) {
                note.y += NOTE_SPEED;
                if (note.y > WINDOW_HEIGHT + 100) {
                    note.active = false;
                    if (!note.judged) {
                        // 未判定的音符視為MISS
                        addJudgmentEffect(JUDGMENT_MISS, note.rail);
                    }
                }
            }
        }
        
        // 移除非活動音符
        notes.erase(std::remove_if(notes.begin(), notes.end(), 
            [](const Note& n) { return !n.active; }), notes.end());
        
        // 更新判定效果計時器
        for (auto& effect : judgmentEffects) {
            effect.timer--;
        }
        judgmentEffects.erase(std::remove_if(judgmentEffects.begin(), judgmentEffects.end(),
            [](const JudgmentEffect& e) { return e.timer <= 0; }), judgmentEffects.end());
    }
    
    Judgment checkHit(int rail, int keyY) {
        for (auto& note : notes) {
            if (note.active && !note.judged && note.rail == rail) {
                int distance = std::abs((note.y + NOTE_HEIGHT/2) - keyY);
                
                if (distance <= JUDGE_PERFECT) {
                    note.judged = true;
                    addJudgmentEffect(JUDGMENT_PERFECT, rail);
                    return JUDGMENT_PERFECT;
                } else if (distance <= JUDGE_GREAT) {
                    note.judged = true;
                    addJudgmentEffect(JUDGMENT_GREAT, rail);
                    return JUDGMENT_GREAT;
                } else if (distance <= JUDGE_GOOD) {
                    note.judged = true;
                    addJudgmentEffect(JUDGMENT_GOOD, rail);
                    return JUDGMENT_GOOD;
                }
            }
        }
        return JUDGMENT_NONE;
    }
    
    void addJudgmentEffect(Judgment type, int rail) {
        JudgmentEffect effect;
        effect.type = type;
        effect.rail = rail;
        effect.timer = 30;
        effect.x = rail * NOTE_WIDTH + NOTE_WIDTH / 2;
        effect.y = WINDOW_HEIGHT - 100;
        judgmentEffects.push_back(effect);
    }
    
    void render() {
        // 渲染音符
        for (const auto& note : notes) {
            if (!note.active) continue;
            
            SDL_Rect rect = {note.rail * NOTE_WIDTH + 10, note.y, 
                            NOTE_WIDTH - 20, note.length};
            
            switch(note.type) {
                case NOTE_TAP:
                    SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255); // 藍色
                    break;
                case NOTE_HOLD:
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // 橙色
                    break;
                case NOTE_GREEN:
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // 綠色
                    break;
                case NOTE_OBSTACLE:
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 紅色
                    break;
                default:
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            
            if (note.holding) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // 黃色（長按中）
            }
            
            SDL_RenderFillRect(renderer, &rect);
        }
        
        // 渲染判定效果
        renderJudgmentEffects();
    }
    
    void renderJudgmentEffects() {
        if (!font) return;
        
        for (const auto& effect : judgmentEffects) {
            SDL_Color color;
            std::string text;
            
            switch(effect.type) {
                case JUDGMENT_PERFECT:
                    color = {255, 20, 147, 255}; // 深粉色
                    text = "PERFECT";
                    break;
                case JUDGMENT_GREAT:
                    color = {255, 215, 0, 255}; // 金色
                    text = "GREAT";
                    break;
                case JUDGMENT_GOOD:
                    color = {255, 255, 255, 255}; // 白色
                    text = "GOOD";
                    break;
                case JUDGMENT_MISS:
                    color = {0, 0, 0, 255}; // 黑色
                    text = "MISS";
                    break;
                default:
                    continue;
            }
            
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            
            SDL_Rect textRect = {
                effect.x - textSurface->w / 2,
                effect.y - textSurface->h / 2,
                textSurface->w,
                textSurface->h
            };
            
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }
    }
    
    void clear() {
        notes.clear();
        judgmentEffects.clear();
    }
};

class InputHandler {
private:
    std::map<SDL_Keycode, int> keyBindings;
    bool keys[4] = {false};
    
public:
    InputHandler() {
        // 默認按鍵綁定：D F J K
        keyBindings[SDLK_d] = 0;
        keyBindings[SDLK_f] = 1;
        keyBindings[SDLK_j] = 2;
        keyBindings[SDLK_k] = 3;
    }
    
    int getRailFromKey(SDL_Keycode key) {
        auto it = keyBindings.find(key);
        if (it != keyBindings.end()) {
            return it->second;
        }
        return -1;
    }
    
    void setKeyState(SDL_Keycode key, bool state) {
        int rail = getRailFromKey(key);
        if (rail != -1) {
            keys[rail] = state;
        }
    }
    
    bool isKeyPressed(int rail) {
        return keys[rail];
    }
};

class RecordManager {
private:
    std::string recordFile;
    int highScore;
    
public:
    RecordManager() : highScore(0) {
        recordFile = "game_records.txt";
        loadRecords();
    }
    
    void loadRecords() {
        std::ifstream file(recordFile);
        if (file.is_open()) {
            file >> highScore;
            file.close();
        }
    }
    
    void saveRecord(int score) {
        if (score > highScore) {
            highScore = score;
            std::ofstream file(recordFile);
            if (file.is_open()) {
                file << highScore;
                file.close();
            }
        }
    }
    
    int getHighScore() const {
        return highScore;
    }
};

class ResultAnalyzer {
private:
    int perfectCount;
    int greatCount;
    int goodCount;
    int missCount;
    int maxCombo;
    
public:
    ResultAnalyzer() : perfectCount(0), greatCount(0), goodCount(0), missCount(0), maxCombo(0) {}
    
    void addJudgment(Judgment judgment, int combo) {
        switch(judgment) {
            case JUDGMENT_PERFECT: perfectCount++; break;
            case JUDGMENT_GREAT: greatCount++; break;
            case JUDGMENT_GOOD: goodCount++; break;
            case JUDGMENT_MISS: missCount++; break;
            default: break;
        }
        maxCombo = std::max(maxCombo, combo);
    }
    
    std::string getGrade() const {
        int total = perfectCount + greatCount + goodCount + missCount;
        if (total == 0) return "F";
        
        double accuracy = (perfectCount * 1.0 + greatCount * 0.75 + goodCount * 0.5) / total;
        
        if (accuracy >= 0.95 && missCount == 0) return "SS";
        if (accuracy >= 0.9 && missCount == 0) return "S";
        if (accuracy >= 0.85) return "A";
        if (accuracy >= 0.75) return "B";
        if (accuracy >= 0.6) return "C";
        return "D";
    }
    
    void reset() {
        perfectCount = greatCount = goodCount = missCount = maxCombo = 0;
    }
    
    int getPerfectCount() const { return perfectCount; }
    int getGreatCount() const { return greatCount; }
    int getGoodCount() const { return goodCount; }
    int getMissCount() const { return missCount; }
    int getMaxCombo() const { return maxCombo; }
};

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_Window* window = SDL_CreateWindow("Rhythm Quest",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    TTF_Init();
    TTF_Font* font = TTF_OpenFont("XITS-Regular.otf", 72);
    TTF_Font* smallFont = TTF_OpenFont("XITS-Regular.otf", 36);
    TTF_Font* mediumFont = TTF_OpenFont("XITS-Regular.otf", 48);
    
    std::srand(std::time(nullptr));

    // 初始化遊戲組件
    MusicManager musicManager;
    ScoreBar scoreBar(renderer, smallFont);
    Decoration decoration(renderer);
    NoteManager noteManager(renderer, mediumFont);
    InputHandler inputHandler;
    RecordManager recordManager;
    ResultAnalyzer resultAnalyzer;

    // 玩家角色
    Player player = {WINDOW_WIDTH / 2, WINDOW_HEIGHT - 100, WINDOW_WIDTH / 2, 
                    false, 0, 0, false, false, 0.0f};

    std::vector<Note> notes;
    int frameCount = 0;
    GameState state = STATE_COUNTDOWN;
    int countdown = 180;

    bool running = true;
    SDL_Event e;

    Uint32 startTime = 0;
    Uint32 endTime = 0;

    // 載入音樂和音效
    musicManager.loadMusic("Rick Astley - Never Gonna Give You Up (Official Music Video).mp3");
    musicManager.loadSoundEffect("perfect", "a.wav");
    musicManager.loadSoundEffect("great", "a.wav");
    musicManager.loadSoundEffect("good", "a.wav");

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = false;

            if (state == STATE_PLAYING) {
                if (e.type == SDL_KEYDOWN) {
                    int keyRail = inputHandler.getRailFromKey(e.key.keysym.sym);
                    if (keyRail != -1) {
                        inputHandler.setKeyState(e.key.keysym.sym, true);
                        Judgment judgment = noteManager.checkHit(keyRail, WINDOW_HEIGHT - 50);
                        if (judgment != JUDGMENT_NONE) {
                            scoreBar.addScore(judgment);
                            resultAnalyzer.addJudgment(judgment, scoreBar.getCombo());
                            
                            // 播放音效
                            switch(judgment) {
                                case JUDGMENT_PERFECT: 
                                    // musicManager.playSoundEffect("perfect");
                                    break;
                                case JUDGMENT_GREAT:
                                    // musicManager.playSoundEffect("great");
                                    break;
                                case JUDGMENT_GOOD:
                                    // musicManager.playSoundEffect("good");
                                    break;
                                default: break;
                            }
                        }
                    }
                } else if (e.type == SDL_KEYUP) {
                    int keyRail = inputHandler.getRailFromKey(e.key.keysym.sym);
                    if (keyRail != -1) {
                        inputHandler.setKeyState(e.key.keysym.sym, false);
                    }
                } else if (e.type == SDL_MOUSEMOTION) {
                    // 滑鼠控制玩家移動
                    player.targetX = e.motion.x;
                    player.isMoving = true;
                }
            } else if (state == STATE_RESULTS && e.type == SDL_KEYDOWN) {
                // 在結果畫面按任意鍵返回
                state = STATE_COUNTDOWN;
                countdown = 180;
                scoreBar.reset();
                resultAnalyzer.reset();
                noteManager.clear();
                frameCount = 0;
            }
        }

        // 更新遊戲狀態
        if (state == STATE_COUNTDOWN) {
            countdown--;
            if (countdown <= 0) {
                state = STATE_PLAYING;
                startTime = SDL_GetTicks();
                // musicManager.playMusic();
            }
        } else if (state == STATE_PLAYING) {
            // 生成音符
            if (frameCount % SPAWN_INTERVAL == 0) {
                NoteType type = static_cast<NoteType>(rand() % 5);
                int length = (type == NOTE_HOLD) ? LONG_NOTE_HEIGHT : NOTE_HEIGHT;
                noteManager.addNote(rand() % RAILS, type, length);
            }

            // 更新遊戲組件
            noteManager.update();
            decoration.update(scoreBar.getCombo());

            // 更新玩家動畫
            if (player.isMoving) {
                float dx = player.targetX - player.x;
                player.x += dx * 0.1f;
                if (std::abs(dx) < 1.0f) {
                    player.isMoving = false;
                }
                
                if (SDL_GetTicks() - player.lastAnimationTime > 100) {
                    player.animationFrame = (player.animationFrame + 1) % 4;
                    player.lastAnimationTime = SDL_GetTicks();
                }
            }

            frameCount++;
        
            if (frameCount > 3600) {
                endTime = SDL_GetTicks();
                state = STATE_RESULTS;
                recordManager.saveRecord(scoreBar.getScore());
                // musicManager.stopMusic();
            }
        }

        // 渲染
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // 根據連擊數改變背景色
        if (scoreBar.getCombo() >= 50) {
            SDL_SetRenderDrawColor(renderer, 50, 25, 0, 255); // 橙金色背景
            SDL_RenderClear(renderer);
        } else if (scoreBar.getCombo() >= 30) {
            SDL_SetRenderDrawColor(renderer, 30, 15, 40, 255); // 紫色背景
            SDL_RenderClear(renderer);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 30, 255); // 深藍色賽博背景
            SDL_RenderClear(renderer);
        }

        if (state == STATE_COUNTDOWN) {
            int num = countdown / 60 + 1;

            if (font) {
                SDL_Color white = {255, 255, 255, 255};
                std::string numText = std::to_string(num);
                SDL_Surface* textSurface = TTF_RenderText_Solid(font, numText.c_str(), white);
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                
                SDL_Rect textRect = {
                    WINDOW_WIDTH / 2 - textSurface->w / 2,
                    WINDOW_HEIGHT / 2 - textSurface->h / 2,
                    textSurface->w,
                    textSurface->h
                };
                
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_FreeSurface(textSurface);
                SDL_DestroyTexture(textTexture);
            }
        } else if (state == STATE_PLAYING) {
            // 渲染軌道
            for (int i = 0; i < RAILS; i++) {
                SDL_Rect rail = {i * NOTE_WIDTH, 0, NOTE_WIDTH, WINDOW_HEIGHT};
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &rail);
            }

            // 渲染裝飾
            decoration.render();

            // 渲染音符和判定效果
            noteManager.render();

            // 渲染分數條
            scoreBar.render();

            // 渲染玩家角色
            SDL_Rect playerRect = {static_cast<int>(player.x - 20), 
                                 static_cast<int>(player.y - 40), 40, 80};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &playerRect);

            // 渲染判定線
            SDL_Rect hitLine = {0, WINDOW_HEIGHT - 50, WINDOW_WIDTH, 5};
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &hitLine);
        } else if (state == STATE_RESULTS) {
            // 結果畫面
            SDL_SetRenderDrawColor(renderer, 30, 30, 60, 255);
            SDL_RenderClear(renderer);
            
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect background = {WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT / 2 - 250, 400, 500};
            SDL_RenderFillRect(renderer, &background);
            
            if (font || smallFont) {
                SDL_Color black = {0, 0, 0, 255};
                SDL_Color blue = {0, 0, 200, 255};
                
                // 遊戲結束標題
                if (font) {
                    SDL_Surface* gameOverSurface = TTF_RenderText_Solid(font, "Game Over", blue);
                    SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);
                    
                    SDL_Rect gameOverRect = {
                        WINDOW_WIDTH / 2 - gameOverSurface->w / 2,
                        WINDOW_HEIGHT / 2 - 200,
                        gameOverSurface->w,
                        gameOverSurface->h
                    };
                    
                    SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
                    SDL_FreeSurface(gameOverSurface);
                    SDL_DestroyTexture(gameOverTexture);
                }
                
                // 分數和評級
                if (smallFont) {
                    std::string grade = resultAnalyzer.getGrade();
                    std::string gradeText = "Grade: " + grade;
                    SDL_Surface* gradeSurface = TTF_RenderText_Solid(smallFont, gradeText.c_str(), black);
                    SDL_Texture* gradeTexture = SDL_CreateTextureFromSurface(renderer, gradeSurface);
                    
                    SDL_Rect gradeRect = {
                        WINDOW_WIDTH / 2 - gradeSurface->w / 2,
                        WINDOW_HEIGHT / 2 - 100,
                        gradeSurface->w,
                        gradeSurface->h
                    };
                    
                    SDL_RenderCopy(renderer, gradeTexture, NULL, &gradeRect);
                    SDL_FreeSurface(gradeSurface);
                    SDL_DestroyTexture(gradeTexture);
                    
                    // 詳細統計
                    std::vector<std::string> stats = {
                        "Perfect: " + std::to_string(resultAnalyzer.getPerfectCount()),
                        "Great: " + std::to_string(resultAnalyzer.getGreatCount()),
                        "Good: " + std::to_string(resultAnalyzer.getGoodCount()),
                        "Miss: " + std::to_string(resultAnalyzer.getMissCount()),
                        "Max Combo: " + std::to_string(resultAnalyzer.getMaxCombo()),
                        "Final Score: " + std::to_string(scoreBar.getScore()),
                        "High Score: " + std::to_string(recordManager.getHighScore())
                    };
                    
                    for (size_t i = 0; i < stats.size(); i++) {
                        SDL_Surface* statSurface = TTF_RenderText_Solid(smallFont, stats[i].c_str(), black);
                        SDL_Texture* statTexture = SDL_CreateTextureFromSurface(renderer, statSurface);
                        
                        SDL_Rect statRect = {
                            WINDOW_WIDTH / 2 - statSurface->w / 2,
                            WINDOW_HEIGHT / 2 - 50 + static_cast<int>(i) * 40,
                            statSurface->w,
                            statSurface->h
                        };
                        
                        SDL_RenderCopy(renderer, statTexture, NULL, &statRect);
                        SDL_FreeSurface(statSurface);
                        SDL_DestroyTexture(statTexture);
                    }
                    
                    // 退出提示
                    std::string exitText = "Press any key to continue";
                    SDL_Surface* exitSurface = TTF_RenderText_Solid(smallFont, exitText.c_str(), black);
                    SDL_Texture* exitTexture = SDL_CreateTextureFromSurface(renderer, exitSurface);
                    
                    SDL_Rect exitRect = {
                        WINDOW_WIDTH / 2 - exitSurface->w / 2,
                        WINDOW_HEIGHT / 2 + 200,
                        exitSurface->w,
                        exitSurface->h
                    };
                    
                    SDL_RenderCopy(renderer, exitTexture, NULL, &exitRect);
                    SDL_FreeSurface(exitSurface);
                    SDL_DestroyTexture(exitTexture);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // 清理資源
    if (font) TTF_CloseFont(font);
    if (smallFont) TTF_CloseFont(smallFont);
    if (mediumFont) TTF_CloseFont(mediumFont);
    
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}