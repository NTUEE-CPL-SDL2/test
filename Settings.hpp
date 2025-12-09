#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdint>
#include <iostream>
#include <string>

#include <include/vector.hpp>

#include "Renderer.hpp"
#include "generate-notes.hpp"
#include <Mods.hpp>

extern TTF_Font *large_font;
extern TTF_Font *medium_font;
extern TTF_Font *small_font;
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern std::size_t LANES;
extern std::size_t FRAGMENTS;
extern uint32_t MS_PER_FRAGMENT;
extern std::string MOD;
extern Game *game;
extern Renderer *gameRenderer;

void renderText(SDL_Renderer *rnd, TTF_Font *font, const std::string &text,
                int x, int y, SDL_Color color, Alignment align = ALIGN_CENTER) {
  if (text.empty())
    return;
  SDL_Surface *textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
  if (!textSurface) {
    std::cerr << "TTF_RenderText error: " << TTF_GetError() << std::endl;
    return;
  }

  SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rnd, textSurface);
  SDL_FreeSurface(textSurface);

  if (!textTexture)
    return;

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

void renderRoundedRect(SDL_Renderer *renderer, SDL_Rect rect, int radius,
                       SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_Rect center = {rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
  SDL_RenderFillRect(renderer, &center);
  SDL_Rect vert = {rect.x, rect.y + radius, rect.w, rect.h - 2 * radius};
  SDL_RenderFillRect(renderer, &vert);
  for (int w = 0; w < radius; ++w) {
    for (int h = 0; h < radius; ++h) {
      if ((w - radius) * (w - radius) + (h - radius) * (h - radius) <=
          radius * radius) {
        SDL_RenderDrawPoint(renderer, rect.x + w, rect.y + h);
        SDL_RenderDrawPoint(renderer, rect.x + rect.w - 1 - w, rect.y + h);
        SDL_RenderDrawPoint(renderer, rect.x + w, rect.y + rect.h - 1 - h);
        SDL_RenderDrawPoint(renderer, rect.x + rect.w - 1 - w,
                            rect.y + rect.h - 1 - h);
      }
    }
  }
}

bool pointInRect(int x, int y, SDL_Rect rect) {
  return x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
         y < rect.y + rect.h;
}

void showSettings(SDL_Renderer *renderer) {
  bool running = true;
  SDL_Event e;

  SDL_Color white = {255, 255, 255, 255};
  SDL_Color grey = {100, 100, 100, 255};
  SDL_Color blue = {0, 128, 255, 255};

  SDL_Rect lanesMinus = {310, 80, 40, 40};
  SDL_Rect lanesPlus = {410, 80, 40, 40};
  SDL_Rect fragmentsMinus = {310, 140, 40, 40};
  SDL_Rect fragmentsPlus = {410, 140, 40, 40};
  SDL_Rect modDropdown = {310, 200, 200, 40};
  SDL_Rect okButton = {SCREEN_WIDTH / 2 - 50, 280, 100, 50};

  std::vector<std::string> modKeys;
  for (auto &p : getModMap())
    modKeys.push_back(p.first);
  int selectedModIndex = 0;

  bool dropdownOpen = false;

  while (running) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        return;

      if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        if (pointInRect(mx, my, lanesMinus)) {
          if (LANES > 1)
            --LANES;
        } else if (pointInRect(mx, my, lanesPlus)) {
          if (LANES < 9)
            ++LANES;
        } else if (pointInRect(mx, my, fragmentsMinus)) {
          if (FRAGMENTS > 2)
            --FRAGMENTS;
        } else if (pointInRect(mx, my, fragmentsPlus)) {
          if (FRAGMENTS < 100)
            ++FRAGMENTS;
        } else if (pointInRect(mx, my, modDropdown)) {
          dropdownOpen = !dropdownOpen;
        } else if (pointInRect(mx, my, okButton)) {
          running = false;
        }

        if (dropdownOpen) {
          for (int i = 0; i < (int)modKeys.size(); ++i) {
            SDL_Rect itemRect = {modDropdown.x, modDropdown.y + 40 * (i + 1),
                                 modDropdown.w, 40};
            if (pointInRect(mx, my, itemRect)) {
              selectedModIndex = i;
              MOD = modKeys[i];
              dropdownOpen = false;
            }
          }
        }
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderText(renderer, medium_font, "LANES:", 50, 100, white, ALIGN_LEFT);
    renderText(renderer, medium_font, "FRAGMENTS:", 50, 160, white, ALIGN_LEFT);
    renderText(renderer, medium_font, "MOD:", 50, 220, white, ALIGN_LEFT);

    renderText(renderer, medium_font, std::to_string(LANES), 380, 105, white);
    renderText(renderer, medium_font, std::to_string(FRAGMENTS), 380, 165,
               white);
    renderText(renderer, medium_font, MOD, 310, 220, white, ALIGN_LEFT);

    renderRoundedRect(renderer, lanesMinus, 10, blue);
    renderRoundedRect(renderer, lanesPlus, 10, blue);
    renderRoundedRect(renderer, fragmentsMinus, 10, blue);
    renderRoundedRect(renderer, fragmentsPlus, 10, blue);
    renderRoundedRect(renderer, modDropdown, 10, grey);
    renderRoundedRect(renderer, okButton, 15, blue);

    renderText(renderer, medium_font, "-", lanesMinus.x + 20, lanesMinus.y + 20,
               white);
    renderText(renderer, medium_font, "+", lanesPlus.x + 20, lanesPlus.y + 20,
               white);
    renderText(renderer, medium_font, "-", fragmentsMinus.x + 20,
               fragmentsMinus.y + 20, white);
    renderText(renderer, medium_font, "+", fragmentsPlus.x + 20,
               fragmentsPlus.y + 20, white);
    renderText(renderer, medium_font, "OK", okButton.x + 49, okButton.y + 27, white);

    if (dropdownOpen) {
      for (int i = 0; i < (int)modKeys.size(); ++i) {
        SDL_Rect itemRect = {modDropdown.x, modDropdown.y + 40 * (i + 1),
                             modDropdown.w, 40};
        renderText(renderer, small_font, modKeys[i], itemRect.x + 10,
                   itemRect.y, white, ALIGN_LEFT);
      }
    }

    SDL_RenderPresent(renderer);
  }

  SettingsFunc modSettingsFunc = mystd::get<2>(getModMap()[MOD]);
  new (game) Game(LANES, FRAGMENTS, MS_PER_FRAGMENT);
  game->notes = generateRandomNotes(LANES, 500, 500, 70);
  new (gameRenderer) Renderer(*game, SCREEN_WIDTH, SCREEN_HEIGHT, renderer,
                              large_font, medium_font, small_font);

  if (modSettingsFunc) {
    modSettingsFunc(renderer, small_font, SCREEN_WIDTH, SCREEN_HEIGHT);
  }
}
