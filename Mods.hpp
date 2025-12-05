#pragma once

#include <unordered_map>
#include <functional>
#include <string>

#include "include/vector.hpp"
#include "include/tuple.hpp"

#include "Game.hpp"

namespace mods {

using ModFunc = void(*)(Game&);

struct ModHash {
  std::size_t operator()(const mystd::tuple<ModFunc, bool> &t) const {
    auto hash1 = std::hash<ModFunc>{}(mystd::get<0>(t));
    auto hash2 = std::hash<bool>{}(mystd::get<1>(t));
    return hash1 ^ (hash2 << 1);
  }
};

inline std::unordered_map<std::string, mystd::tuple<ModFunc, bool>>& getModMap() {
  static std::unordered_map<std::string, mystd::tuple<ModFunc, bool>> modMap;
  return modMap;
}

inline void registerMod(const std::string& name, ModFunc foo, bool before) {
  getModMap()[name] = mystd::make_tuple(foo, before);
}

} // namespace mods