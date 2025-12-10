#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// 定義圖片路徑（路徑更改區）
const std::string LOGO_PATH = "img/BeatRunnerIcon.png";
const std::string START_PATH = "img/StartButtonLight.png";
const std::string SONGS_PATH = "img/SongsButtonLight.png";
const std::string SETTINGS_PATH = "img/SettingsButtonLight.png";
const std::string BACKGROUND_PATH = "img/Background.png";
const std::string SETTINGS_WINDOW_PATH = "img/SettingsWindow.png";
const std::string VOLUME_ICON_PATH = "img/VolumeIcon.png";
const std::string SLIDER_KNOB_PATH = "img/Slider.png";

// 狀態控制變數
bool gShowSettings = false; // 是否顯示設定視窗
float gVolume = 0.5f;       // 音量 (0.0 到 1.0)
bool gIsDraggingSlider = false; // 滑桿拖曳狀態

// --- 新增的功能函式 (點擊按鈕後執行) ---

void StartGame() {
    // 進入遊戲主介面或開始遊戲的邏輯
    std::cout << "Starting Game..." << std::endl;
}

void ShowSongList() {
    // 顯示歌曲選擇清單的邏輯
    std::cout << "Showing Song List..." << std::endl;
}

void OpenSettings() {
    gShowSettings = true; // 開啟設定視窗狀態
    std::cout << "DEBUG: OpenSettings() Called" << std::endl;
}

// ----------------------------------------

// 檢查座標 (x, y) 是否在矩形 (rect) 內
bool isInside(int x, int y, const SDL_Rect& rect) {
    return (x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h);
}

// 載入紋理
SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == nullptr) {
        std::cerr << "ERROR: Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    } else {
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (newTexture == nullptr) {
            std::cerr << "ERROR: Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        }
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

// 初始化 SDL
bool init(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    *window = SDL_CreateWindow(
        "Beat Runner Mockup - Image Version",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED
    );

    if (*window == NULL) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (*renderer == NULL) {
        std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    return true;
}

// 關閉並釋放資源 (參數已更新)
void close(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* logoTex, SDL_Texture* startTex, SDL_Texture* songsTex, SDL_Texture* settingsTex, SDL_Texture* backgroundTex, SDL_Texture* windowTex, SDL_Texture* volIconTex, SDL_Texture* knobTex) {
    if (logoTex) SDL_DestroyTexture(logoTex);
    if (startTex) SDL_DestroyTexture(startTex);
    if (songsTex) SDL_DestroyTexture(songsTex);
    if (settingsTex) SDL_DestroyTexture(settingsTex);
    if (backgroundTex) SDL_DestroyTexture(backgroundTex);

    // 釋放新的紋理
    if (windowTex) SDL_DestroyTexture(windowTex);
    if (volIconTex) SDL_DestroyTexture(volIconTex);
    if (knobTex) SDL_DestroyTexture(knobTex);

    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* args[]) {
    SDL_Window* gWindow = NULL;
    SDL_Renderer* gRenderer = NULL;

    SDL_Texture* gLogoTexture = nullptr;
    SDL_Texture* gStartTexture = nullptr;
    SDL_Texture* gSongsTexture = nullptr;
    SDL_Texture* gSettingsTexture = nullptr;
    SDL_Texture* gBackgroundTexture = nullptr;

    // --- 新增紋理變數 ---
    SDL_Texture* gSettingsWindowTexture = nullptr;
    SDL_Texture* gVolumeIconTexture = nullptr;
    SDL_Texture* gSliderKnobTexture = nullptr;
    // ----------------------

    if (!init(&gWindow, &gRenderer)) {
        std::cerr << "Failed to initialize!" << std::endl;
        std::cout << "Initialization failed. Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    // --- 載入紋理 ---
    gLogoTexture = loadTexture(LOGO_PATH, gRenderer);
    gStartTexture = loadTexture(START_PATH, gRenderer);
    gSongsTexture = loadTexture(SONGS_PATH, gRenderer);
    gSettingsTexture = loadTexture(SETTINGS_PATH, gRenderer);
    gBackgroundTexture = loadTexture(BACKGROUND_PATH, gRenderer);
    // 載入新的紋理
    gSettingsWindowTexture = loadTexture(SETTINGS_WINDOW_PATH, gRenderer);
    gVolumeIconTexture = loadTexture(VOLUME_ICON_PATH, gRenderer);
    gSliderKnobTexture = loadTexture(SLIDER_KNOB_PATH, gRenderer);

    if (gLogoTexture == nullptr || gStartTexture == nullptr || gSongsTexture == nullptr || gSettingsTexture == nullptr || gBackgroundTexture == nullptr || gSettingsWindowTexture == nullptr || gVolumeIconTexture == nullptr || gSliderKnobTexture == nullptr) {
        std::cerr << "One or more textures failed to load. Please check the DLLs or image paths." << std::endl;
        // 傳遞所有紋理到 close 函式
        close(gWindow, gRenderer, gLogoTexture, gStartTexture, gSongsTexture, gSettingsTexture, gBackgroundTexture, gSettingsWindowTexture, gVolumeIconTexture, gSliderKnobTexture);
        std::cout << "Loading failed. Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }


    // --- 圖片大小和位置調整區 ---

    // 1. 背景 Rect (位置/大小：覆蓋整個視窗)
    SDL_Rect backgroundRect = {
        0, // X 位置 (通常為 0)
        0, // Y 位置 (通常為 0)
        SCREEN_WIDTH, // 寬度
        SCREEN_HEIGHT // 高度
    };

    // 2. Logo Rect (位置/大小)
    int logoSize = (int)(SCREEN_WIDTH * 0.22); // 調整 Logo 尺寸 (寬和高)
    SDL_Rect logoRect = {
        (int)(SCREEN_WIDTH * 0.5 - logoSize / 2), // X 位置 (螢幕中央)
        (int)(SCREEN_HEIGHT * 0.05),              // Y 位置 (頂部 5%)
        logoSize,
        logoSize
    };

    // 3. 按鈕的通用尺寸和間距
    int buttonWidth = (int)(SCREEN_WIDTH * 0.22);     // 調整按鈕寬度
    int buttonHeight = (int)(SCREEN_HEIGHT * 0.132);  // 調整按鈕高度
    int buttonX = (SCREEN_WIDTH - buttonWidth) / 2;   // 按鈕 X 位置 (螢幕中央)
    int buttonSpacing = (int)(SCREEN_HEIGHT * 0.02);  // 按鈕間距

    // 4. START 按鈕 Rect (位置)
    SDL_Rect startButtonRect = {
        buttonX,
        (int)(SCREEN_HEIGHT * 0.5), // 調整 START 按鈕 Y 位置
        buttonWidth,
        buttonHeight
    };

    // 5. SONGS 按鈕 Rect (位置)
    SDL_Rect songsButtonRect = {
        buttonX,
        startButtonRect.y + startButtonRect.h + buttonSpacing, // Y 位置
        buttonWidth,
        buttonHeight
    };

    // 6. SETTINGS 按鈕 Rect (位置)
    SDL_Rect settingsButtonRect = {
        buttonX,
        songsButtonRect.y + songsButtonRect.h + buttonSpacing, // Y 位置
        buttonWidth,
        buttonHeight
    };

    // --- 設定視窗元件的固定尺寸 ---
    // 視窗尺寸和位置
    SDL_Rect windowRect = {
        SCREEN_WIDTH / 4,
        SCREEN_HEIGHT / 4,
        SCREEN_WIDTH / 2,
        (int)(SCREEN_HEIGHT * 0.4) // 調整高度，確保是橫長方形
    };

    // 滑桿背景條的矩形
    SDL_Rect sliderBar = {
        windowRect.x + (int)(windowRect.w * 0.3),                 // 讓滑桿靠右一點
        windowRect.y + (int)(windowRect.h * 0.48),                // 居中
        (int)(windowRect.w * 0.5),                                // 滑桿長度
        10                                                        // 滑桿高度
    };

    // 音量圖標的矩形
    SDL_Rect volIconRect = {
        windowRect.x + (int)(windowRect.w * 0.1),                // 靠左(數字小)一點
        sliderBar.y - (int)(windowRect.h * 0.19),                  // 稍微往上(數字大)對齊
        (int)(windowRect.w * 0.2),                                // 寬度
        (int)(windowRect.w * 0.2)                                 // 高度
    };
    // ------------------------------

    // --- 遊戲迴圈和互動邏輯 ---
    bool quit = false;
    SDL_Event e;
    int mouseX, mouseY;

    // 調整亮度乘數 (R, G, B)
    const Uint8 NORMAL_MOD = 230;
    const Uint8 HOVER_MOD = 255;

    while (!quit) {
        SDL_GetMouseState(&mouseX, &mouseY);

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            }

            if (gShowSettings) {
                // --- 設定視窗互動邏輯 ---
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        // 檢查是否點擊到滑桿旋鈕區域開始拖曳
                        SDL_Rect knobCheckRect = {
                            (int)(sliderBar.x + (gVolume * sliderBar.w) - 20), // 擴大點擊範圍
                            sliderBar.y - 20,
                            40,
                            sliderBar.h + 40
                        };

                        if (isInside(e.button.x, e.button.y, knobCheckRect)) {
                            gIsDraggingSlider = true;
                        } else if (!isInside(e.button.x, e.button.y, windowRect)) {
                            // 點擊視窗外關閉設定
                            gShowSettings = false;
                            gIsDraggingSlider = false; // 確保關閉時停止拖曳
                        }
                    }
                } else if (e.type == SDL_MOUSEBUTTONUP) {
                    // 釋放左鍵時停止拖曳
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        gIsDraggingSlider = false;
                    }
                } else if (e.type == SDL_MOUSEMOTION && gIsDraggingSlider) {
                    // 拖曳滑桿
                    int newKnobX = e.motion.x - sliderBar.x;

                    // 限制 newKnobX 在 0 到 sliderBar.w 之間
                    if (newKnobX < 0) newKnobX = 0;
                    if (newKnobX > sliderBar.w) newKnobX = sliderBar.w;

                    // 更新音量比例
                    gVolume = (float)newKnobX / sliderBar.w;
                    std::cout << "Volume: " << gVolume * 100.0f << "%" << std::endl;
                }

            } else {
                // --- 主選單互動邏輯 ---
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    int clickX = e.button.x;
                    int clickY = e.button.y;

                    if (isInside(clickX, clickY, startButtonRect)) {
                        StartGame();
                    }
                    else if (isInside(clickX, clickY, songsButtonRect)) {
                        ShowSongList();
                    }
                    else if (isInside(clickX, clickY, settingsButtonRect)) {
                        OpenSettings();
                    }
                }
            }
        }

        // ------------------ 渲染開始 ------------------

        SDL_SetRenderDrawColor(gRenderer, 0x1A, 0x00, 0x40, 0xFF);
        SDL_RenderClear(gRenderer);

        // 1. 繪製背景
        SDL_RenderCopy(gRenderer, gBackgroundTexture, NULL, &backgroundRect);

        // 2. 繪製 Logo 圖片
        SDL_RenderCopy(gRenderer, gLogoTexture, NULL, &logoRect);

        // --- 懸停亮度調整區 (僅在設定視窗關閉時才生效) ---
        if (!gShowSettings) {
            SDL_SetTextureColorMod(gStartTexture, NORMAL_MOD, NORMAL_MOD, NORMAL_MOD);
            SDL_SetTextureColorMod(gSongsTexture, NORMAL_MOD, NORMAL_MOD, NORMAL_MOD);
            SDL_SetTextureColorMod(gSettingsTexture, NORMAL_MOD, NORMAL_MOD, NORMAL_MOD);

            if (isInside(mouseX, mouseY, startButtonRect)) {
                SDL_SetTextureColorMod(gStartTexture, HOVER_MOD, HOVER_MOD, HOVER_MOD);
            }
            if (isInside(mouseX, mouseY, songsButtonRect)) {
                SDL_SetTextureColorMod(gSongsTexture, HOVER_MOD, HOVER_MOD, HOVER_MOD);
            }
            if (isInside(mouseX, mouseY, settingsButtonRect)) {
                SDL_SetTextureColorMod(gSettingsTexture, HOVER_MOD, HOVER_MOD, HOVER_MOD);
            }
        }

        // 3. 繪製按鈕圖片
        SDL_RenderCopy(gRenderer, gStartTexture, NULL, &startButtonRect);
        SDL_RenderCopy(gRenderer, gSongsTexture, NULL, &songsButtonRect);
        SDL_RenderCopy(gRenderer, gSettingsTexture, NULL, &settingsButtonRect);


        if (gShowSettings) {
            // --- 繪製設定視窗在最上層 ---

            // 1. 繪製半透明黑色遮罩 (讓背景變暗)
            SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 150);
            SDL_RenderFillRect(gRenderer, &backgroundRect);

            // 2. 繪製設定小視窗
            SDL_RenderCopy(gRenderer, gSettingsWindowTexture, NULL, &windowRect);

            // 3. 繪製音量圖標
            SDL_RenderCopy(gRenderer, gVolumeIconTexture, NULL, &volIconRect);

            // 4. 繪製滑桿底條 (使用 SDL 繪圖功能代替紋理)
            SDL_SetRenderDrawColor(gRenderer, 50, 50, 50, 255); // 深灰色
            SDL_RenderFillRect(gRenderer, &sliderBar);

            // 5. 繪製滑桿進度條 (使用 SDL 繪圖功能)
            SDL_Rect sliderProgress = sliderBar;
            sliderProgress.w = (int)(gVolume * sliderBar.w);
            SDL_SetRenderDrawColor(gRenderer, 0x00, 0xCC, 0xFF, 0xFF); // 賽博藍
            SDL_RenderFillRect(gRenderer, &sliderProgress);


            // 6. 繪製滑桿旋鈕
            SDL_Rect knobRect = {
                (int)(sliderBar.x + (gVolume * sliderBar.w) - 10), // x: 滑桿位置減去旋鈕半寬
                sliderBar.y - 10,                                  // y: 滑桿位置往上提一點
                20,
                30
            };
            SDL_RenderCopy(gRenderer, gSliderKnobTexture, NULL, &knobRect);
        }

        SDL_RenderPresent(gRenderer);
    }

    // 關閉時傳遞所有紋理
    close(gWindow, gRenderer, gLogoTexture, gStartTexture, gSongsTexture, gSettingsTexture, gBackgroundTexture, gSettingsWindowTexture, gVolumeIconTexture, gSliderKnobTexture);

    return 0;
}
