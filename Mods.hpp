#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "include/tuple.hpp"
#include "include/vector.hpp"

#include "Game.hpp"

namespace mods {

using ModFunc = void (*)(const void *, Game &game);

inline std::unordered_map<std::string, mystd::tuple<ModFunc, ModFunc>> &
getModMap() {
  static std::unordered_map<std::string, mystd::tuple<ModFunc, ModFunc>> modMap;
  return modMap;
}

inline void registerMod(const std::string &name, ModFunc foo, ModFunc bar) {
  getModMap()[name] = mystd::make_tuple(foo, bar);
}

} // namespace mods
