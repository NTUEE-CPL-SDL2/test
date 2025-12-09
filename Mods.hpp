#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "include/tuple.hpp"
#include "include/vector.hpp"

#include "Game.hpp"

using ModFunc = void (*)(const void *, Game &game);
using SettingsFunc = bool (*)();

inline std::unordered_map<std::string, mystd::tuple<ModFunc, ModFunc, SettingsFunc>> &
getModMap() {
  static std::unordered_map<std::string, mystd::tuple<ModFunc, ModFunc, SettingsFunc>> modMap;
  return modMap;
}

inline void registerMod(const std::string &name, ModFunc foo, ModFunc bar, SettingsFunc set) {
  getModMap()[name] = mystd::make_tuple(foo, bar, set);
}
