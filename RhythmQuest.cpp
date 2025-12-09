#if __cplusplus < 202002L
#error "Require C++20 or later"
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <cstddef>
#include <functional>
#include <iostream>

#include "include/tuple.hpp"

#include "Game.hpp"
#include "Mods.hpp"
#include "Renderer.hpp"
#include "generate-notes.hpp"
#include "mods/GameOfLife.hpp"

std::size_t LANES;
std::size_t FRAGMENTS;
uint64_t MS_PER_FRAGMENT;
std::string MOD;
SettingsFunc modSettingsFunc;

Game* game = static_cast<Game*>(::operator new(sizeof(Game)));
Renderer* gameRenderer = static_cast<Renderer*>(::operator new(sizeof(Renderer)));

enum class GameState { SETTINGS, COUNTDOWN, GAME, PAUSE };

void showSettings(SDL_Renderer *renderer, TTF_Font *large_font, TTF_Font *medium_font, TTF_Font *small_font, int SCREEN_WIDTH, int SCREEN_HEIGHT) {
  std::cout << "=== Settings Menu ===" << std::endl;
  std::cout << "1. Mod Settings" << std::endl;
  std::cout << "2. Audio Settings" << std::endl;
  std::cout << "3. Gameplay Settings" << std::endl;
  std::cout << "4. Back to Game" << std::endl;

  LANES = 9;
  FRAGMENTS = 10;
  MS_PER_FRAGMENT = 100;
  MOD = "Game of Life (hold notes counted as alive cell, before new fragments "
        "load)";
  modSettingsFunc = mystd::get<2>(getModMap()[MOD]);
  new (game) Game(LANES, FRAGMENTS, MS_PER_FRAGMENT);
  game->notes = generateRandomNotes(LANES, 500, 500, 70);
  new (gameRenderer) Renderer(*game, SCREEN_WIDTH, SCREEN_HEIGHT, renderer, large_font,
                        medium_font, small_font);

  if (modSettingsFunc) {
    modSettingsFunc(renderer, small_font, SCREEN_WIDTH, SCREEN_HEIGHT);
  }

  std::cout << "Settings loaded. Entering game..." << std::endl;
}

void showCountdown(SDL_Renderer *renderer, TTF_Font *large_font) {
  std::cout << "Starting countdown..." << std::endl;
  for (int i = 3; i > 0; --i) {
    std::cout << i << "..." << std::endl;
    SDL_Delay(1000);
  }
  std::cout << "GO!" << std::endl;
}

void showPauseMenu() {
  std::cout << "=== Pause Menu ===" << std::endl;
  std::cout << "1. Resume" << std::endl;
  std::cout << "2. Restart" << std::endl;
  std::cout << "3. Settings" << std::endl;
  std::cout << "4. Exit to Menu" << std::endl;
}

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize: " << SDL_GetError() << std::endl;
    return 1;
  }

  if (TTF_Init() < 0) {
    std::cerr << "SDL_ttf could not initialize: " << TTF_GetError()
              << std::endl;
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

  int SCREEN_WIDTH = 1024;
  int SCREEN_HEIGHT = 768;

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


  GameState currentState = GameState::SETTINGS;
  bool running = true;
  Uint32 currentTime = 0, lastFragmentTime = 0, gameStartTime = 0;

  while (running) {
    currentTime = SDL_GetTicks();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
        continue;
      } else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          int newWidth = event.window.data1;
          int newHeight = event.window.data2;
          gameRenderer->updateDimension(newWidth, newHeight);
        }
      }

      switch (currentState) {
      case GameState::SETTINGS:
        if (event.type == SDL_KEYDOWN) {
          if (event.key.keysym.sym == SDLK_RETURN) {
            currentState = GameState::COUNTDOWN;
          } else if (event.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
          }
        }
        break;

      case GameState::COUNTDOWN:
        break;

      case GameState::GAME:
        if (event.type == SDL_KEYDOWN) {
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            currentState = GameState::PAUSE;
          } else if (event.key.keysym.sym == SDLK_p) {
            currentState = GameState::PAUSE;
          } else {
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
              game->keyPressed(lane, SDL_GetTicks() - gameStartTime);
            }
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
            game->keyReleased(lane, SDL_GetTicks() - gameStartTime);
          }
        }
        break;

      case GameState::PAUSE:
        if (event.type == SDL_KEYDOWN) {
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
          } else if (event.key.keysym.sym == SDLK_p ||
                     event.key.keysym.sym == SDLK_RETURN) {
            currentState = GameState::GAME;
          } else if (event.key.keysym.sym == SDLK_s) {
            currentState = GameState::SETTINGS;
          }
        }
        break;
      }
    }

    switch (currentState) {
    case GameState::SETTINGS:
      showSettings(renderer, large_font, medium_font, small_font, SCREEN_WIDTH, SCREEN_HEIGHT);
      currentState = GameState::COUNTDOWN;
      break;

    case GameState::COUNTDOWN:
      showCountdown(renderer, large_font);
      gameStartTime = SDL_GetTicks();
      lastFragmentTime = gameStartTime;
      currentState = GameState::GAME;
      break;

    case GameState::GAME:
      game->clearExpiredEffects(currentTime - gameStartTime);

      if (currentTime - lastFragmentTime >= MS_PER_FRAGMENT) {
        game->loadFragment(mystd::get<0>(getModMap()[MOD]),
                          mystd::get<1>(getModMap()[MOD]));
        lastFragmentTime += MS_PER_FRAGMENT;
      }

      break;

    case GameState::PAUSE:
      showPauseMenu();
      break;
    }

    switch (currentState) {
    case GameState::SETTINGS:
      gameRenderer->render(renderer);
      break;

    case GameState::COUNTDOWN:
      gameRenderer->render(renderer);
      break;

    case GameState::GAME:
      gameRenderer->render(renderer);
      break;

    case GameState::PAUSE:
      gameRenderer->render(renderer);
      break;
    }

    SDL_RenderPresent(renderer);
    
    Uint32 tmpTime = SDL_GetTicks();
    gameRenderer->fps = 1000.0f / (float)(tmpTime - currentTime);

    SDL_Delay(1);
  }

  delete gameRenderer;
  delete game;
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
