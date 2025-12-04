#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <chrono>
#include "Game.hpp"
#include "Renderer.hpp"
#include "generate-notes.hpp"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() < 0) {
        std::cerr << "TTF could not initialize: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    TTF_Font* large_font = TTF_OpenFont("XITS-Regular.otf", 72);
    TTF_Font* medium_font = TTF_OpenFont("XITS-Regular.otf", 48);
    TTF_Font* small_font = TTF_OpenFont("XITS-Regular.otf", 36);

    if (!large_font || !medium_font || !small_font) {
        std::cerr << "Failed to load fonts: " << TTF_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    const int SCREEN_WIDTH = 1024;
    const int SCREEN_HEIGHT = 768;

    SDL_Window* window = SDL_CreateWindow(
        "Rhythm Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Window could not be created: " << SDL_GetError() << std::endl;
        TTF_CloseFont(large_font);
        TTF_CloseFont(medium_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_CloseFont(large_font);
        TTF_CloseFont(medium_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    const size_t LANES = 4;
    const size_t FRAGMENTS = 12;
    const uint64_t MS_PER_FRAGMENT = 100;

    Game game(LANES, FRAGMENTS, MS_PER_FRAGMENT);

    game.notes = generateRandomNotes(LANES, 500, 500);

    Renderer gameRenderer(game, SCREEN_WIDTH, SCREEN_HEIGHT,
                         large_font, medium_font, small_font);

    bool running = true;
    auto lastUpdateTime = std::chrono::steady_clock::now();
    auto lastFragmentTime = lastUpdateTime;

    Uint32 frameCount = 0;
    auto lastFPSUpdate = std::chrono::steady_clock::now();
    float fps = 0;

    // Need to be moved to game state start after other states are added
    auto gameStartTime = SDL_GetTicks();

    while (running) {
        auto currentTime = std::chrono::steady_clock::now();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int newWidth = event.window.data1;
                    int newHeight = event.window.data2;
                    gameRenderer.updateDimension(newWidth, newHeight);
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }

                size_t lane = -1;
                switch (event.key.keysym.sym) {
                    case SDLK_a: lane = 0; break;
                    case SDLK_s: lane = 1; break;
                    case SDLK_d: lane = 2; break;
                    case SDLK_f: lane = 3; break;
                    case SDLK_g: lane = 4; break;
                    case SDLK_h: lane = 5; break;
                    case SDLK_j: lane = 6; break;
                    case SDLK_k: lane = 7; break;
                    case SDLK_l: lane = 8; break;
                    default: break;
                }

                if (lane < LANES) {
                    game.keyPressed(lane, SDL_GetTicks() - gameStartTime);
                    std::cout << "Lane " << lane << " pressed\n";
                    std::cout << "Score: " << game.score << ", Perfect: " << game.perfectCount << ", Great: " << game.greatCount << ", Good: " << game.goodCount << ", Bad: " << game.badCount << ", Miss: " << game.missCount << ", Combo: " << game.combo << ", MaxCombo: " << game.maxCombo << ", HeldTime: " << game.heldTime << std::endl;
                }
            } else if (event.type == SDL_KEYUP) {
                size_t lane = -1;
                switch (event.key.keysym.sym) {
                    case SDLK_a: lane = 0; break;
                    case SDLK_s: lane = 1; break;
                    case SDLK_d: lane = 2; break;
                    case SDLK_f: lane = 3; break;
                    case SDLK_g: lane = 4; break;
                    case SDLK_h: lane = 5; break;
                    case SDLK_j: lane = 6; break;
                    case SDLK_k: lane = 7; break;
                    case SDLK_l: lane = 8; break;
                    default: break;
                }

                if (lane < LANES) {
                    game.keyReleased(lane, SDL_GetTicks() - gameStartTime);
                    std::cout << "Lane " << lane << " released\n";
                    std::cout << "Score: " << game.score << ", Perfect: " << game.perfectCount << ", Great: " << game.greatCount << ", Good: " << game.goodCount << ", Bad: " << game.badCount << ", Miss: " << game.missCount << ", Combo: " << game.combo << ", MaxCombo: " << game.maxCombo << ", HeldTime: " << game.heldTime << std::endl;
                }
            }
        }

        auto timeSinceLastFragment = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastFragmentTime).count();

        while (timeSinceLastFragment >= MS_PER_FRAGMENT) {
            game.loadFragment();
            lastFragmentTime += std::chrono::milliseconds(MS_PER_FRAGMENT);
            timeSinceLastFragment -= MS_PER_FRAGMENT;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        gameRenderer.render(renderer);

        frameCount++;
        auto fpsUpdateTime = std::chrono::steady_clock::now();
        auto fpsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            fpsUpdateTime - lastFPSUpdate).count();

        if (fpsElapsed > 1000) {
            fps = frameCount * 1000.0f / fpsElapsed;
            frameCount = 0;
            lastFPSUpdate = fpsUpdateTime;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(large_font);
    TTF_CloseFont(medium_font);
    TTF_CloseFont(small_font);
    TTF_Quit();
    SDL_Quit();

    return 0;
}