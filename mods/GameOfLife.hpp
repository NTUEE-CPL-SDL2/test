#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <functional>
#include <string>

#include "../include/array.hpp"

#include "../Game.hpp"
#include "../Mods.hpp"
#include "../Renderer.hpp"

namespace gameOfLife {

static mystd::array<uint16_t, 2> currentRules = {0b111111111, 0};

template <bool hold_alive> void gameOfLife(Game &game) {
  auto oldHighway = game.highway;
  const auto &rules = currentRules;

  auto isAlive = [](int8_t val) -> bool {
    if (val == -1)
      return true;
    if (val > 0)
      return hold_alive;
    return false;
  };

  for (std::size_t lane = 0; lane < game.lanes; ++lane) {
    for (std::size_t f = 0; f < game.fragments; ++f) {
      int aliveCount = 0;

      for (int dl = -1; dl <= 1; ++dl) {
        for (int df = -1; df <= 1; ++df) {
          if (dl == 0 && df == 0)
            continue;
          int nl = static_cast<int>(lane) + dl;
          int nf = static_cast<int>(f) + df;

          if (nl >= 0 && nl < static_cast<int>(game.lanes) && nf >= 0 &&
              nf < static_cast<int>(game.fragments)) {
            if (isAlive(oldHighway[nl][nf]))
              aliveCount++;
          }
        }
      }

      int8_t &cell = game.highway[lane][f];

      if (cell == -1) { // alive
        if (!((rules[0] >> aliveCount) & 0b1)) {
          cell = 0; // die
        }
      } else if (cell == 0) { // dead
        if ((rules[1] >> aliveCount) & 0b1) {
          cell = -1; // born
        }
      }
      // >0 (hold) stays unchanged
    }
  }
}

void gameOfLifeHoldAlive(Game &game) { return gameOfLife<true>(game); }

void gameOfLifeHoldDead(Game &game) { return gameOfLife<false>(game); }

void drawText(SDL_Renderer *rnd, const std::string &text, int x, int y,
              TTF_Font *font, SDL_Color color, Alignment align = ALIGN_LEFT) {
  if (text.empty())
    return;
  SDL_Surface *textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
  if (!textSurface) {
    std::cerr << "TTF_RenderText error: " << TTF_GetError() << std::endl;
    return;
  }

  SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rnd, textSurface);
  SDL_FreeSurface(textSurface);
  int textWidth, textHeight;
  SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);

  SDL_Rect destRect;
  destRect.y = y - textHeight / 2;

  switch (align) {
  case ALIGN_LEFT:
    destRect.x = x;
    break;
  case ALIGN_CENTER:
    destRect.x = x - textWidth / 2;
    break;
  case ALIGN_RIGHT:
    destRect.x = x - textWidth;
    break;
  }

  destRect.w = textWidth;
  destRect.h = textHeight;

  SDL_RenderCopy(rnd, textTexture, nullptr, &destRect);
}

void gameOfLifeSettings(SDL_Renderer *renderer, TTF_Font *font, int screenWidth,
                        int screenHeight) {
  mystd::array<uint16_t, 2> tempRules = currentRules;

  const int buttonSize = 40;
  const int buttonSpacing = 5;
  const int rowSpacing = 60;

  int totalWidth = 9 * buttonSize + 8 * buttonSpacing;
  int startX = (screenWidth - totalWidth) / 2;
  int row1Y = screenHeight / 2 - rowSpacing;
  int row2Y = screenHeight / 2 + rowSpacing;

  const int okButtonWidth = 100;
  const int okButtonHeight = 50;
  int okButtonX = (screenWidth - okButtonWidth) / 2;
  int okButtonY = row2Y + rowSpacing * 2;

  bool settingsRunning = true;
  bool redrawNeeded = true;

  while (settingsRunning) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        settingsRunning = false;
        break;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN ||
            event.key.keysym.sym == SDLK_ESCAPE) {
          currentRules = tempRules;
          settingsRunning = false;
        }
      } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
          int mouseX = event.button.x;
          int mouseY = event.button.y;

          for (int i = 0; i < 9; ++i) {
            int buttonX = startX + i * (buttonSize + buttonSpacing);
            SDL_Rect buttonRect = {buttonX, row1Y, buttonSize, buttonSize};

            if (mouseX >= buttonRect.x &&
                mouseX <= buttonRect.x + buttonRect.w &&
                mouseY >= buttonRect.y &&
                mouseY <= buttonRect.y + buttonRect.h) {
              tempRules[0] ^= (1 << i);
              redrawNeeded = true;
            }
          }

          for (int i = 0; i < 9; ++i) {
            int buttonX = startX + i * (buttonSize + buttonSpacing);
            SDL_Rect buttonRect = {buttonX, row2Y, buttonSize, buttonSize};

            if (mouseX >= buttonRect.x &&
                mouseX <= buttonRect.x + buttonRect.w &&
                mouseY >= buttonRect.y &&
                mouseY <= buttonRect.y + buttonRect.h) {
              tempRules[1] ^= (1 << i);
              redrawNeeded = true;
            }
          }

          SDL_Rect okRect = {okButtonX, okButtonY, okButtonWidth,
                             okButtonHeight};
          if (mouseX >= okRect.x && mouseX <= okRect.x + okRect.w &&
              mouseY >= okRect.y && mouseY <= okRect.y + okRect.h) {
            currentRules = tempRules;
            settingsRunning = false;
          }
        }
      }
    }

    if (redrawNeeded) {
      SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
      SDL_RenderClear(renderer);

      for (int i = 0; i < 9; ++i) {
        int buttonX = startX + i * (buttonSize + buttonSpacing);
        SDL_Rect buttonRect = {buttonX, row1Y, buttonSize, buttonSize};

        bool enabled = (tempRules[0] >> i) & 0b1;

        if (enabled) {
          SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        } else {
          SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        }
        SDL_RenderFillRect(renderer, &buttonRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &buttonRect);

        SDL_Color numColor = {255, 255, 255, 255};
        drawText(renderer, std::to_string(i), buttonX + buttonSize / 2,
                 row1Y + buttonSize / 2, font, numColor, ALIGN_CENTER);
      }

      for (int i = 0; i < 9; ++i) {
        int buttonX = startX + i * (buttonSize + buttonSpacing);
        SDL_Rect buttonRect = {buttonX, row2Y, buttonSize, buttonSize};

        bool enabled = (tempRules[1] >> i) & 0b1;

        if (enabled) {
          SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        } else {
          SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        }
        SDL_RenderFillRect(renderer, &buttonRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &buttonRect);
        SDL_Color numColor = {255, 255, 255, 255};
        drawText(renderer, std::to_string(i), buttonX + buttonSize / 2,
                 row2Y + buttonSize / 2, font, numColor, ALIGN_CENTER);
      }

      SDL_Rect okRect = {okButtonX, okButtonY, okButtonWidth, okButtonHeight};
      SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
      SDL_RenderFillRect(renderer, &okRect);
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderDrawRect(renderer, &okRect);

      SDL_Color textColor = {255, 255, 255, 255};
      std::string surviveText = "Survive with neighbors:";
      std::string reviveText = "Revive with neighbors:";

      int surviveTextX = screenWidth / 2;
      int surviveTextY = row1Y - 40;
      drawText(renderer, surviveText, surviveTextX, surviveTextY, font,
               textColor, ALIGN_CENTER);

      int reviveTextX = screenWidth / 2;
      int reviveTextY = row2Y - 40;
      drawText(renderer, reviveText, reviveTextX, reviveTextY, font, textColor,
               ALIGN_CENTER);

      std::string okText = "OK";
      int okTextX = okButtonX + okButtonWidth / 2;
      int okTextY = okButtonY + okButtonHeight / 2;
      drawText(renderer, okText, okTextX, okTextY, font, textColor,
               ALIGN_CENTER);

      SDL_RenderPresent(renderer);
      redrawNeeded = false;
    }
  }
}

} // namespace gameOfLife

// Self-register with settings function
namespace {
const bool registered = [] {
  registerMod("Game of Life (hold notes counted as alive cell, before "
              "new fragments load)",
              &gameOfLife::gameOfLifeHoldAlive, nullptr,
              gameOfLife::gameOfLifeSettings);

  registerMod("Game of Life (hold notes counted as alive cell, after new "
              "fragments load)",
              nullptr, &gameOfLife::gameOfLifeHoldAlive,
              gameOfLife::gameOfLifeSettings);

  registerMod("Game of Life (hold notes counted as dead cell, before new "
              "fragments load)",
              &gameOfLife::gameOfLifeHoldDead, nullptr,
              gameOfLife::gameOfLifeSettings);

  registerMod("Game of Life (hold notes counted as dead cell, after new "
              "fragments load)",
              nullptr, &gameOfLife::gameOfLifeHoldDead,
              gameOfLife::gameOfLifeSettings);

  return true;
}();
} // namespace
