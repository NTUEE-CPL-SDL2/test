#if __cplusplus < 202002L
#error "Require C++20 or later"
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>

#include "Game.hpp"
#include "Mods.hpp"
#include "Renderer.hpp"
#include "Settings.hpp"
#include "mods/GameOfLife.hpp"

TTF_Font *large_font, *medium_font, *small_font;
int SCREEN_WIDTH = 1024;
int SCREEN_HEIGHT = 768;

std::size_t LANES = 8;
std::size_t FRAGMENTS = 10;
uint32_t MS_PER_FRAGMENT = 200;
std::string MOD;
SettingsFunc modSettingsFunc;

Game *game = static_cast<Game *>(::operator new(sizeof(Game)));
Renderer *gameRenderer =
    static_cast<Renderer *>(::operator new(sizeof(Renderer)));

enum class GameState { SETTINGS, COUNTDOWN, GAME, PAUSE };

GameState currentState;
bool running = true;

void showPauseMenu(SDL_Renderer *renderer) {
  bool running = true;
  SDL_Event e;

  SDL_Color white = {255, 255, 255, 255};
  SDL_Color blue = {0, 128, 255, 255};
  SDL_Color dark = {20, 20, 20, 200};

  SDL_Rect resumeButton = {SCREEN_WIDTH / 2 - 100, 150, 200, 60};
  SDL_Rect newGameButton = {SCREEN_WIDTH / 2 - 100, 230, 200, 60};
  SDL_Rect exitButton = {SCREEN_WIDTH / 2 - 100, 310, 200, 60};

  int choice = 0; // 1=resume, 2=newgame, 3=exit

  while (running) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        return;

      if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (pointInRect(mx, my, resumeButton)) {
          choice = 1;
          running = false;
        } else if (pointInRect(mx, my, newGameButton)) {
          choice = 2;
          running = false;
        } else if (pointInRect(mx, my, exitButton)) {
          choice = 3;
          running = false;
        }
      }
    }

    SDL_SetRenderDrawColor(renderer, dark.r, dark.g, dark.b, dark.a);
    SDL_RenderFillRect(renderer, nullptr);

    renderText(renderer, large_font, "Paused", SCREEN_WIDTH / 2 - 80, 60,
               white);

    renderRoundedRect(renderer, resumeButton, 15, blue);
    renderRoundedRect(renderer, newGameButton, 15, blue);
    renderRoundedRect(renderer, exitButton, 15, blue);

    renderText(renderer, medium_font, "Resume", resumeButton.x + 45,
               resumeButton.y + 15, white);
    renderText(renderer, medium_font, "New Game", newGameButton.x + 35,
               newGameButton.y + 15, white);
    renderText(renderer, medium_font, "Exit Game", exitButton.x + 45,
               exitButton.y + 15, white);

    SDL_RenderPresent(renderer);
  }

  if (choice == 1) {
    return;
  } else if (choice == 2) {
    currentState = GameState::SETTINGS;
  } else if (choice == 3) {
    running = false;
  }
}

void showCountdown(SDL_Renderer *renderer) {
  SDL_Color white = {255, 255, 255, 255};

  SDL_Surface *goSurface = IMG_Load("res/img/GO.png");
  if (!goSurface) {
    std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
    return;
  }
  SDL_Texture *goTexture = SDL_CreateTextureFromSurface(renderer, goSurface);
  SDL_FreeSurface(goSurface);

  int goW, goH;
  SDL_QueryTexture(goTexture, nullptr, nullptr, &goW, &goH);
  SDL_Rect goRect = {(SCREEN_WIDTH - goW) / 2, (SCREEN_HEIGHT - goH) / 2 - 40,
                     goW, goH};

  int count = 3;
  Uint32 lastTick = SDL_GetTicks();
  bool showGo = false;

  bool running = true;
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        running = false;
      }
    }

    Uint32 now = SDL_GetTicks();
    if (!showGo && now - lastTick >= 1000) {
      count--;
      lastTick = now;
      if (count == 0) {
        showGo = true;
        lastTick = now;
      }
    } else if (showGo && now - lastTick >= 1000) {
      break;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (!showGo) {
      renderText(renderer, large_font, std::to_string(count), SCREEN_WIDTH / 2,
                 SCREEN_HEIGHT / 2, white);
    } else {
      SDL_RenderCopy(renderer, goTexture, nullptr, &goRect);
      renderText(renderer, large_font, "GO!", SCREEN_WIDTH / 2,
                 goRect.y + goRect.h + 20, white);
    }

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(goTexture);
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

  large_font = TTF_OpenFont("XITS-Regular.otf", 72);
  medium_font = TTF_OpenFont("XITS-Regular.otf", 40);
  small_font = TTF_OpenFont("XITS-Regular.otf", 28);

  if (!large_font || !medium_font || !small_font) {
    std::cerr << "Failed to load fonts: " << TTF_GetError() << std::endl;
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
  }

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

  currentState = GameState::SETTINGS;
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
      showSettings(renderer);
      currentState = GameState::COUNTDOWN;
      break;

    case GameState::COUNTDOWN:
      showCountdown(renderer);
      gameStartTime = SDL_GetTicks();
      lastFragmentTime = gameStartTime;
      currentState = GameState::GAME;
      break;

    case GameState::GAME: {
      game->clearExpiredEffects(currentTime - gameStartTime);

      uint32_t offsetMs = currentTime - lastFragmentTime;

      if (offsetMs >= MS_PER_FRAGMENT) {
        game->loadFragment(mystd::get<0>(getModMap()[MOD]),
                           mystd::get<1>(getModMap()[MOD]));
        lastFragmentTime += MS_PER_FRAGMENT;
        offsetMs = 0;
      }
      gameRenderer->render(renderer, offsetMs);

      break;
    }

    case GameState::PAUSE:
      showPauseMenu(renderer);
      break;
    }

    SDL_RenderPresent(renderer);

    Uint32 tmpTime = SDL_GetTicks();
    gameRenderer->fps = 1000.0f / (float)(tmpTime - currentTime);
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
