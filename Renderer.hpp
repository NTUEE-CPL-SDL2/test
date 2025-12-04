#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "include/tuple.hpp"

#include "Game.hpp"

struct TupleHash {
  size_t operator()(const mystd::tuple<int8_t, bool, uint64_t> &t) const {
    auto hash1 = std::hash<int8_t>{}(mystd::get<0>(t));
    auto hash2 = std::hash<bool>{}(mystd::get<1>(t));
    auto hash3 = std::hash<uint64_t>{}(mystd::get<2>(t));
    return hash1 ^ (hash2 << 1) ^ (hash3 << 2);
  }
};

class Renderer {
private:
  Game &game;
  std::unordered_map<mystd::tuple<int8_t, bool, uint64_t>, SDL_Texture *,
                     TupleHash>
      notesTextureCache;
  std::unordered_map<std::string, SDL_Texture *> textTextureCache;

  int screenW;
  int screenH;
  int laneWidth;
  int fragmentHeight;

  TTF_Font *large_font;
  TTF_Font *medium_font;
  TTF_Font *small_font;

  enum TextAlignment { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

public:
  float fps;

  Renderer(Game &game_, int screenW_, int screenH_, TTF_Font *large_font_,
           TTF_Font *medium_font_, TTF_Font *small_font_)
      : game(game_), screenW(screenW_), screenH(screenH_),
        large_font(large_font_), medium_font(medium_font_),
        small_font(small_font_), fps(0) {
    laneWidth = screenW / game.lanes;
    fragmentHeight = screenH / game.fragments;
  }

  ~Renderer() { clearCache(); }

  void render(SDL_Renderer *rnd) {
    SDL_SetRenderDrawColor(rnd, 0, 0, 0, 255);
    SDL_RenderClear(rnd);

    SDL_SetRenderDrawColor(rnd, 100, 100, 100, 255);
    for (size_t lane = 1; lane < game.lanes; ++lane) {
      int x = lane * laneWidth;
      SDL_RenderDrawLine(rnd, x, 0, x, screenH);
    }

    SDL_SetRenderDrawColor(rnd, 60, 60, 60, 255);
    for (size_t fragment = 1; fragment < game.fragments; ++fragment) {
      int y = fragment * fragmentHeight;
      SDL_RenderDrawLine(rnd, 0, y, screenW, y);
    }

    SDL_SetRenderDrawColor(rnd, 255, 255, 255, 255);
    int judgmentLineY = screenH - fragmentHeight;
    SDL_Rect judgmentLine = {0, judgmentLineY, screenW, 3};
    SDL_RenderFillRect(rnd, &judgmentLine);

    for (size_t lane = 0; lane < game.lanes; ++lane) {
      bool lanePressed = game.lanePressed[lane];

      for (size_t fragmentIdx = 0; fragmentIdx < game.fragments;
           ++fragmentIdx) {
        int8_t fragmentValue = game.highway[lane][fragmentIdx];
        uint64_t holdTime = 0;

        if (fragmentIdx == game.fragments - 1 && fragmentValue > 0 &&
            lanePressed) {
          holdTime = game.holdPressedTime[lane];
        }

        auto key = mystd::make_tuple(
            fragmentValue, lanePressed && fragmentIdx == game.fragments - 1,
            holdTime);

        auto it = notesTextureCache.find(key);
        SDL_Texture *texture;

        if (it == notesTextureCache.end()) {
          texture = createFragmentTexture(
              rnd, fragmentValue,
              lanePressed && fragmentIdx == game.fragments - 1, holdTime);
          notesTextureCache[key] = texture;
        } else {
          texture = it->second;
        }

        SDL_Rect destRect;
        destRect.x = lane * laneWidth;
        destRect.y = fragmentIdx * fragmentHeight;
        destRect.w = laneWidth;
        destRect.h = fragmentHeight;

        SDL_RenderCopy(rnd, texture, nullptr, &destRect);
      }

      int laneCenterX = lane * laneWidth + laneWidth / 2;

      std::string keyHint;
      switch (lane) {
      case 0:
        keyHint = "A";
        break;
      case 1:
        keyHint = "S";
        break;
      case 2:
        keyHint = "D";
        break;
      case 3:
        keyHint = "F";
        break;
      case 4:
        keyHint = "G";
        break;
      case 5:
        keyHint = "H";
        break;
      case 6:
        keyHint = "J";
        break;
      case 7:
        keyHint = "K";
        break;
      case 8:
        keyHint = "L";
        break;
      default:
        keyHint = std::to_string(lane + 1);
        break;
      }
      drawText(rnd, keyHint, laneCenterX, screenH - 30, small_font,
               {200, 200, 200, 255}, ALIGN_CENTER);
    }

    drawText(rnd, "Score: " + std::to_string(game.score), screenW / 2, 30,
             medium_font, {255, 255, 255, 255}, ALIGN_CENTER);

    int statsX = 20;
    int statsY = 30;
    int lineHeight = 30;

    drawText(rnd, "PERFECT: " + std::to_string(game.perfectCount), statsX,
             statsY, small_font, {0, 255, 0, 255});
    drawText(rnd, "GREAT: " + std::to_string(game.greatCount), statsX,
             statsY + lineHeight, small_font, {0, 200, 100, 255});
    drawText(rnd, "GOOD: " + std::to_string(game.goodCount), statsX,
             statsY + lineHeight * 2, small_font, {200, 200, 0, 255});
    drawText(rnd, "BAD: " + std::to_string(game.badCount), statsX,
             statsY + lineHeight * 3, small_font, {255, 100, 0, 255});
    drawText(rnd, "MISS: " + std::to_string(game.missCount), statsX,
             statsY + lineHeight * 4, small_font, {255, 0, 0, 255});
    drawText(rnd, "COMBO: " + std::to_string(game.combo), statsX,
             statsY + lineHeight * 5, small_font, {255, 0, 0, 255});
    drawText(rnd, "MAX COMBO: " + std::to_string(game.maxCombo), statsX,
             statsY + lineHeight * 6, small_font, {255, 255, 255, 255});
    drawText(rnd, "HELD TIME: " + std::to_string(game.heldTime) + " ms", statsX,
             statsY + lineHeight * 7, small_font, {100, 255, 100, 255});

    drawText(rnd, "Fragment: " + std::to_string(game.nowFragment), screenW - 20,
             30, small_font, {200, 200, 200, 255}, ALIGN_RIGHT);
    drawText(rnd, "FPS: " + std::to_string(fps), screenW - 20, 60, small_font,
             {200, 200, 200, 255}, ALIGN_RIGHT);
  }

  void clearCache() {
    for (auto &pair : notesTextureCache) {
      SDL_DestroyTexture(pair.second);
    }
    notesTextureCache.clear();

    for (auto &pair : textTextureCache) {
      SDL_DestroyTexture(pair.second);
    }
    textTextureCache.clear();
  }

  void updateDimension(int screenW_, int screenH_) {
    if (screenW != screenW_ || screenH != screenH_) {
      screenW = screenW_;
      screenH = screenH_;
      laneWidth = screenW / game.lanes;
      fragmentHeight = screenH / game.fragments;

      clearCache();
    }
  }

private:
  SDL_Texture *createFragmentTexture(SDL_Renderer *rnd, int8_t fragmentValue,
                                     bool pressed, uint64_t holdPressedTime) {
    SDL_Texture *texture =
        SDL_CreateTexture(rnd, SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET, laneWidth, fragmentHeight);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    SDL_Texture *prevTarget = SDL_GetRenderTarget(rnd);
    SDL_SetRenderTarget(rnd, texture);

    SDL_SetRenderDrawColor(rnd, 0, 0, 0, 0);
    SDL_RenderClear(rnd);

    SDL_Color color;
    if (fragmentValue < 0) {
      color = {255, 50, 50, 255};
    } else if (fragmentValue > 0) {
      if (pressed) {
        float pulse = 0.7f + 0.3f * sin(holdPressedTime / 100.0f);
        color = {0, static_cast<Uint8>(150 * pulse), 0, 255};
      } else {
        color = {100, 255, 100, 200};
      }
    } else {
      if (pressed) {
        color = {50, 50, 200, 120};
      } else {
        color = {80, 80, 180, 80};
      }
    }

    SDL_SetRenderDrawColor(rnd, color.r, color.g, color.b, color.a);
    SDL_Rect fillRect = {1, 1, laneWidth - 2, fragmentHeight - 2};
    SDL_RenderFillRect(rnd, &fillRect);

    if (fragmentValue != 0) {
      SDL_Color borderColor = fragmentValue < 0 ? SDL_Color{255, 200, 200, 255}
                                                : SDL_Color{200, 255, 200, 255};

      SDL_SetRenderDrawColor(rnd, borderColor.r, borderColor.g, borderColor.b,
                             borderColor.a);
      SDL_RenderDrawRect(rnd, &fillRect);

      if (fragmentValue > 0) {
        std::string holdText = std::to_string(fragmentValue);
        drawText(rnd, holdText, laneWidth / 2, fragmentHeight / 2, small_font,
                 {255, 255, 255, 255}, ALIGN_CENTER);
      }
    }

    SDL_SetRenderTarget(rnd, prevTarget);

    return texture;
  }

  SDL_Texture *getTextTexture(SDL_Renderer *rnd, const std::string &text,
                              TTF_Font *font, SDL_Color color) {
    std::string cacheKey =
        text + "_" + std::to_string(color.r) + "_" + std::to_string(color.g) +
        "_" + std::to_string(color.b) + "_" + std::to_string(color.a);

    auto it = textTextureCache.find(cacheKey);
    if (it != textTextureCache.end()) {
      return it->second;
    }

    SDL_Surface *textSurface =
        TTF_RenderText_Blended(font, text.c_str(), color);
    if (!textSurface) {
      std::cerr << "TTF_RenderText error: " << TTF_GetError() << std::endl;
      return nullptr;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(rnd, textSurface);
    SDL_FreeSurface(textSurface);

    if (texture) {
      textTextureCache[cacheKey] = texture;
    }

    return texture;
  }

  void drawText(SDL_Renderer *rnd, const std::string &text, int x, int y,
                TTF_Font *font, SDL_Color color,
                TextAlignment align = ALIGN_LEFT) {
    if (text.empty())
      return;

    SDL_Texture *textTexture = getTextTexture(rnd, text, font, color);
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
};