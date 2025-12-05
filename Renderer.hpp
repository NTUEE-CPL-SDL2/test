#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>

#include "include/tuple.hpp"

#include "Game.hpp"

struct TupleHash {
  std::size_t operator()(const mystd::tuple<int8_t, bool, uint64_t> &t) const {
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
  std::unordered_map<std::string, SDL_Texture *> imageTextureCache;

  int screenW;
  int screenH;
  int laneWidth;
  int fragmentHeight;

  TTF_Font *large_font;
  TTF_Font *medium_font;
  TTF_Font *small_font;

  SDL_Renderer* sdl_renderer;

  enum Alignment : uint8_t { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

public:
  float fps;

  Renderer(Game &game_, int screenW_, int screenH_, SDL_Renderer* renderer, TTF_Font *large_font_,
           TTF_Font *medium_font_, TTF_Font *small_font_)
      : game(game_), screenW(screenW_), screenH(screenH_),
        sdl_renderer(renderer), large_font(large_font_), medium_font(medium_font_),
        small_font(small_font_), fps(0) {
    laneWidth = screenW / game.lanes;
    fragmentHeight = screenH / game.fragments;

    loadEffectImages();
  }

  ~Renderer() { clearCache(); }

  void render(SDL_Renderer *rnd) {
    SDL_SetRenderDrawColor(rnd, 0, 0, 0, 255);
    SDL_RenderClear(rnd);

    // Draw game field
    SDL_SetRenderDrawColor(rnd, 100, 100, 100, 255);
    for (std::size_t lane = 1; lane < game.lanes; ++lane) {
      int x = lane * laneWidth;
      SDL_RenderDrawLine(rnd, x, 0, x, screenH);
    }

    SDL_SetRenderDrawColor(rnd, 60, 60, 60, 255);
    for (std::size_t fragment = 1; fragment < game.fragments; ++fragment) {
      int y = fragment * fragmentHeight;
      SDL_RenderDrawLine(rnd, 0, y, screenW, y);
    }

    SDL_SetRenderDrawColor(rnd, 255, 255, 255, 255);
    int judgmentLineY = screenH - fragmentHeight;
    SDL_Rect judgmentLine = {0, judgmentLineY, screenW, 3};
    SDL_RenderFillRect(rnd, &judgmentLine);

    // Render notes
    for (std::size_t lane = 0; lane < game.lanes; ++lane) {
      bool lanePressed = game.lanePressed[lane];

      for (std::size_t fragmentIdx = 0; fragmentIdx < game.fragments;
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

      // Draw lane key hints
      int laneCenterX = lane * laneWidth + laneWidth / 2;
      std::string keyHint;
      switch (lane) {
      case 0: keyHint = "A"; break;
      case 1: keyHint = "S"; break;
      case 2: keyHint = "D"; break;
      case 3: keyHint = "F"; break;
      case 4: keyHint = "G"; break;
      case 5: keyHint = "H"; break;
      case 6: keyHint = "J"; break;
      case 7: keyHint = "K"; break;
      case 8: keyHint = "L"; break;
      default: keyHint = std::to_string(lane + 1); break;
      }
      drawText(rnd, keyHint, laneCenterX, screenH - 30, small_font,
               {200, 200, 200, 255}, ALIGN_CENTER);
    }

    // Draw score at top center
    drawText(rnd, "Score: " + std::to_string(game.score), screenW / 2, 30,
             medium_font, {255, 255, 255, 255}, ALIGN_CENTER);

    // Draw stats on left
    const int statsX = 20;
    const int statsY = 30;
    const int lineHeight = 40;

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
             statsY + lineHeight * 5, small_font, {255, 255, 255, 255});
    drawText(rnd, "MAX COMBO: " + std::to_string(game.maxCombo), statsX,
             statsY + lineHeight * 6, small_font, {255, 255, 255, 255});
    drawText(rnd, "HELD TIME: " + std::to_string(game.heldTime) + " ms", statsX,
             statsY + lineHeight * 7, small_font, {100, 255, 100, 255});

    // Draw info on right
    drawText(rnd, "Fragment: " + std::to_string(game.nowFragment), screenW - 20,
             30, small_font, {200, 200, 200, 255}, ALIGN_RIGHT);
    drawText(rnd, "FPS: " + std::to_string(fps), screenW - 20, 70, small_font,
             {200, 200, 200, 255}, ALIGN_RIGHT);

    // Draw lane effects
    for (std::size_t lane = 0; lane < game.lanes; ++lane) {
      uint32_t effect = game.laneEffects[lane];
      if (effect != NO_LANE_EFFECT) {
        drawLaneEffect(rnd, lane, effect);
      }
    }

    // Draw center effects
    if (game.centerEffect != NO_CENTER_EFFECT) {
      drawCenterEffect(rnd, game.centerEffect);
    }
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

    for (auto &pair : imageTextureCache) {
      SDL_DestroyTexture(pair.second);
    }
    imageTextureCache.clear();
  }

  void updateDimension(int screenW_, int screenH_) {
    if (screenW != screenW_ || screenH != screenH_) {
      screenW = screenW_;
      screenH = screenH_;
      laneWidth = screenW / game.lanes;
      fragmentHeight = screenH / game.fragments;

      clearCache();
      loadEffectImages();
    }
  }

private:
  void loadEffectImages() {
    const char* effectImages[] = {
        "res/img/perfect.png",
        "res/img/great.png", 
        "res/img/good.png",
        "res/img/bad.png",
        "res/img/miss.png",
        "res/img/hold_released.png",
        "res/img/combo.png",
        "res/img/score.png"
    };
    
    for (const char* path : effectImages) {
      std::string key = path;
      SDL_Texture* texture = loadImageTexture(path);
      if (texture) {
        imageTextureCache[key] = texture;
      }
    }
  }
  
  SDL_Texture* loadImageTexture(const char* path) {
    SDL_Texture* texture = IMG_LoadTexture(sdl_renderer, path);
    if (!texture) {
      std::cerr << "Failed to load image " << path << ": " << IMG_GetError() << std::endl;
      return nullptr;
    }

    int originalWidth, originalHeight;
    SDL_QueryTexture(texture, nullptr, nullptr, &originalWidth, &originalHeight);

    float scaleFactor = std::min(screenW / 1920.0f, screenH / 1080.0f);
    int targetWidth = static_cast<int>(originalWidth * scaleFactor * 0.5f);
    int targetHeight = static_cast<int>(originalHeight * scaleFactor * 0.5f);

    SDL_Texture* scaledTexture = SDL_CreateTexture(sdl_renderer,
                                                   SDL_PIXELFORMAT_RGBA8888,
                                                   SDL_TEXTUREACCESS_TARGET,
                                                   targetWidth,
                                                   targetHeight);
    if (!scaledTexture) {
      std::cerr << "Failed to create scaled texture: " << SDL_GetError() << std::endl;
      SDL_DestroyTexture(texture);
      return nullptr;
    }
    
    SDL_SetTextureBlendMode(scaledTexture, SDL_BLENDMODE_BLEND);

    SDL_Texture* prevTarget = SDL_GetRenderTarget(sdl_renderer);
    SDL_SetRenderTarget(sdl_renderer, scaledTexture);

    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 0);
    SDL_RenderClear(sdl_renderer);

    SDL_Rect destRect = {0, 0, targetWidth, targetHeight};
    SDL_RenderCopy(sdl_renderer, texture, nullptr, &destRect);

    SDL_SetRenderTarget(sdl_renderer, prevTarget);

    SDL_DestroyTexture(texture);
    
    return scaledTexture;
  }
  
  void drawLaneEffect(SDL_Renderer* rnd, std::size_t lane, uint32_t effect) {
    std::string imagePath;
    std::string effectText;
    SDL_Color textColor = {255, 255, 255, 255};
    
    switch (effect) {
      case PERFECT:
        imagePath = "res/img/perfect.png";
        effectText = "PERFECT!";
        textColor = {0, 255, 0, 255};
        break;
      case GREAT:
        imagePath = "res/img/great.png";
        effectText = "GREAT!";
        textColor = {0, 200, 100, 255};
        break;
      case GOOD:
        imagePath = "res/img/good.png";
        effectText = "GOOD";
        textColor = {200, 200, 0, 255};
        break;
      case BAD:
        imagePath = "res/img/bad.png";
        effectText = "BAD";
        textColor = {255, 100, 0, 255};
        break;
      case MISS:
        imagePath = "res/img/miss.png";
        effectText = "MISS";
        textColor = {255, 0, 0, 255};
        break;
      case HOLD_RELEASED:
        imagePath = "res/img/hold_released.png";
        effectText = "HOLD";
        textColor = {100, 255, 100, 255};
        break;
      default:
        return;
    }

    auto it = imageTextureCache.find(imagePath);
    SDL_Texture* imageTexture = nullptr;
    
    if (it == imageTextureCache.end()) {
      imageTexture = loadImageTexture(imagePath.c_str());
      if (imageTexture) {
        imageTextureCache[imagePath] = imageTexture;
      }
    } else {
      imageTexture = it->second;
    }
    
    if (!imageTexture) return;

    int effectWidth, effectHeight;
    SDL_QueryTexture(imageTexture, nullptr, nullptr, &effectWidth, &effectHeight);

    int laneCenterX = lane * laneWidth + laneWidth / 2;
    int effectY = fragmentHeight * 2;
    
    SDL_Rect destRect = {
      laneCenterX - effectWidth / 2,
      effectY - effectHeight / 2,
      effectWidth,
      effectHeight
    };

    SDL_RenderCopy(rnd, imageTexture, nullptr, &destRect);

    drawText(rnd, effectText, laneCenterX, effectY, small_font, textColor, ALIGN_CENTER);
  }
  
  void drawCenterEffect(SDL_Renderer* rnd, uint32_t effect) {
    if (effect & COMBO) {
      std::string imagePath = "res/img/combo.png";
      std::string comboText = std::to_string(game.combo);

      auto it = imageTextureCache.find(imagePath);
      SDL_Texture* imageTexture = nullptr;
      
      if (it == imageTextureCache.end()) {
        imageTexture = loadImageTexture(imagePath.c_str());
        if (imageTexture) {
          imageTextureCache[imagePath] = imageTexture;
        }
      } else {
        imageTexture = it->second;
      }
      
      if (imageTexture) {
        int effectWidth, effectHeight;
        SDL_QueryTexture(imageTexture, nullptr, nullptr, &effectWidth, &effectHeight);

        effectWidth = static_cast<int>(effectWidth * 1.5f);
        effectHeight = static_cast<int>(effectHeight * 1.5f);

        SDL_Rect destRect = {
          screenW / 2 - effectWidth / 2,
          screenH / 3 - effectHeight / 2,
          effectWidth,
          effectHeight
        };
        
        SDL_RenderCopy(rnd, imageTexture, nullptr, &destRect);

        SDL_Color comboColor = game.combo >= 50 ? SDL_Color{255, 215, 0, 255} : 
                              game.combo >= 20 ? SDL_Color{255, 100, 255, 255} : 
                              SDL_Color{255, 255, 255, 255};
        
        drawText(rnd, comboText, screenW / 2, screenH / 3, large_font, comboColor, ALIGN_CENTER);
        drawText(rnd, "COMBO", screenW / 2, screenH / 3 + 80, medium_font, comboColor, ALIGN_CENTER);
      }
    }
    
    if (effect & SCORE) {
      std::string imagePath = "res/img/score.png";

      auto it = imageTextureCache.find(imagePath);
      SDL_Texture* imageTexture = nullptr;
      
      if (it == imageTextureCache.end()) {
        imageTexture = loadImageTexture(imagePath.c_str());
        if (imageTexture) {
          imageTextureCache[imagePath] = imageTexture;
        }
      } else {
        imageTexture = it->second;
      }
      
      if (imageTexture) {
        int effectWidth, effectHeight;
        SDL_QueryTexture(imageTexture, nullptr, nullptr, &effectWidth, &effectHeight);

        SDL_Rect destRect = {
          screenW / 2 - effectWidth / 4,
          80,
          effectWidth / 2,
          effectHeight / 2
        };
        
        SDL_RenderCopy(rnd, imageTexture, nullptr, &destRect);

        drawText(rnd, "+" + std::to_string(game.score % 1000), 
                screenW / 2, 150, medium_font, {100, 255, 100, 255}, ALIGN_CENTER);
      }
    }
  }

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
    if (fragmentValue == -1) {
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

    if (fragmentValue >= -1) {
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
                TTF_Font *font, SDL_Color color, Alignment align = ALIGN_LEFT) {
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