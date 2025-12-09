#pragma once

#include <functional>
#include <map>
#include <string>

#include "include/tuple.hpp"
#include "include/vector.hpp"

#include "Game.hpp"

using ModFunc = void (*)(Game &game);
using SettingsFunc = void (*)(SDL_Renderer *, TTF_Font *, int, int);

inline std::map<std::string, mystd::tuple<ModFunc, ModFunc, SettingsFunc>> &
getModMap() {
  static std::map<std::string, mystd::tuple<ModFunc, ModFunc, SettingsFunc>>
      modMap;
  return modMap;
}

inline void registerMod(const std::string &name, ModFunc foo, ModFunc bar,
                        SettingsFunc set) {
  getModMap()[name] = mystd::make_tuple(foo, bar, set);
}
