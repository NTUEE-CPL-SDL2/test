#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <cstddef>
#include <iostream>

#include "Game.hpp"
#include "Mods.hpp"
#include "Renderer.hpp"
#include "generate-notes.hpp"
#include "GameOfLife.hpp"

const std::size_t LANES = 4;
const std::size_t FRAGMENTS = 10;
const uint64_t MS_PER_FRAGMENT = 100;
std::string MOD = "Game of Life, Survive: 2, Revive: 3, Hold Treated as Alive, Before New Fragments Load";

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize: " << SDL_GetError() << std::endl;
    return 1;
  }

  if (TTF_Init() < 0) {
    std::cerr << "SDL_ttf could not initialize: " << TTF_GetError() << std::endl;
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    std::cerr << "SDL_image could not initialize: " << IMG_GetError()
              << std::endl;
    SDL_Quit();
    return 1;
  }

  TTF_Font *large_font = TTF_OpenFont("XITS-Regular.otf", 72);
  TTF_Font *medium_font = TTF_OpenFont("XITS-Regular.otf", 48);
  TTF_Font *small_font = TTF_OpenFont("XITS-Regular.otf", 36);

  if (!large_font || !medium_font || !small_font) {
    std::cerr << "Failed to load fonts: " << TTF_GetError() << std::endl;
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  const int SCREEN_WIDTH = 1024;
  const int SCREEN_HEIGHT = 768;

  SDL_Window *window = SDL_CreateWindow(
      "Rhythm Quest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::cerr << "Window could not be created: " << SDL_GetError() << std::endl;
    TTF_CloseFont(large_font);
    TTF_CloseFont(medium_font);
    TTF_CloseFont(small_font);
    TTF_Quit();
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    std::cerr << "Renderer could not be created: " << SDL_GetError()
              << std::endl;
    SDL_DestroyWindow(window);
    TTF_CloseFont(large_font);
    TTF_CloseFont(medium_font);
    TTF_CloseFont(small_font);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

  Game game(LANES, FRAGMENTS, MS_PER_FRAGMENT);

  game.notes = generateRandomNotes(LANES, 500, 500);

  Renderer gameRenderer(game, SCREEN_WIDTH, SCREEN_HEIGHT, renderer, large_font,
                        medium_font, small_font);

  bool running = true;
  Uint32 currentTime = SDL_GetTicks();
  Uint32 lastUpdateTime = SDL_GetTicks();
  Uint32 lastFragmentTime = lastUpdateTime;
  // Need to be moved to game state start after other states are added
  Uint32 gameStartTime = lastUpdateTime;

  while (running) {
    Uint32 tmpCurrentTime = SDL_GetTicks();
    float diffT = tmpCurrentTime - currentTime;
    if (diffT > 0)
      gameRenderer.fps = 1000.0f / diffT;
    currentTime = tmpCurrentTime;

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

        std::size_t lane = -1;
        switch (event.key.keysym.sym) {
        case SDLK_a:
          lane = 0;
          break;
        case SDLK_s:
          lane = 1;
          break;
        case SDLK_d:
          lane = 2;
          break;
        case SDLK_f:
          lane = 3;
          break;
        case SDLK_g:
          lane = 4;
          break;
        case SDLK_h:
          lane = 5;
          break;
        case SDLK_j:
          lane = 6;
          break;
        case SDLK_k:
          lane = 7;
          break;
        case SDLK_l:
          lane = 8;
          break;
        default:
          break;
        }

        if (lane < LANES) {
          game.keyPressed(lane, SDL_GetTicks() - gameStartTime);
        }
      } else if (event.type == SDL_KEYUP) {
        std::size_t lane = -1;
        switch (event.key.keysym.sym) {
        case SDLK_a:
          lane = 0;
          break;
        case SDLK_s:
          lane = 1;
          break;
        case SDLK_d:
          lane = 2;
          break;
        case SDLK_f:
          lane = 3;
          break;
        case SDLK_g:
          lane = 4;
          break;
        case SDLK_h:
          lane = 5;
          break;
        case SDLK_j:
          lane = 6;
          break;
        case SDLK_k:
          lane = 7;
          break;
        case SDLK_l:
          lane = 8;
          break;
        default:
          break;
        }

        if (lane < LANES) {
          game.keyReleased(lane, SDL_GetTicks() - gameStartTime);
        }
      }
    }

    game.clearExpiredEffects(currentTime);

    if (currentTime - lastFragmentTime >= MS_PER_FRAGMENT) {
      auto& modEntry = mods::getModMap()[MOD];
game.loadFragment(mystd::get<0>(modEntry), mystd::get<1>(modEntry));
      lastFragmentTime += MS_PER_FRAGMENT;
    }

    gameRenderer.render(renderer);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_CloseFont(large_font);
  TTF_CloseFont(medium_font);
  TTF_CloseFont(small_font);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();

  return 0;
}