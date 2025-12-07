#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "include/tuple.hpp"
#include "include/vector.hpp"

#include "Game.hpp"

namespace mods {

using ModFunc = void (*)(const void *, Game &game);

inline std::unordered_map<std::string, mystd::tuple<ModFunc, bool>> &
getModMap() {
  static std::unordered_map<std::string, mystd::tuple<ModFunc, bool>> modMap;
  return modMap;
}

inline void registerMod(const std::string &name, ModFunc foo, bool before) {
  getModMap()[name] = mystd::make_tuple(foo, before);
}

} // namespace mods
