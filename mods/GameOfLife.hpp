#include <string>

#include "../include/tuple.hpp"

#include "../Game.hpp"
#include "../Mods.hpp"

template <bool hold_alive>
void gameOfLife(const mystd::tuple<uint8_t, uint8_t> &t,
                Game &game) { // tuple: survive, alive
  auto oldHighway = game.highway;

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
        if (!((mystd::get<0>(t) >> aliveCount) & 0b1)) {
          cell = 0; // die
        }
      } else if (cell == 0) { // dead
        if (!((mystd::get<1>(t) >> aliveCount) & 0b1)) {
          cell = -1; // born
        }
      }
      // >0 (hold) stays unchanged
    }
  }
}

void gameOfLifeHoldAlive(const void *pt, Game &game) {
  return gameOfLife<true>(
      *static_cast<const mystd::tuple<uint8_t, uint8_t> *>(pt), game);
}

void gameOfLifeHoldDead(const void *pt, Game &game) {
  return gameOfLife<false>(
      *static_cast<const mystd::tuple<uint8_t, uint8_t> *>(pt), game);
}

// Self-register
namespace {
const bool registered = [] {
  mods::registerMod("Game of Life (hold notes counted as alive cell, before "
                    "new fragments load)",
                    &gameOfLifeHoldAlive, true);
  mods::registerMod("Game of Life (hold notes counted as alive cell, after new "
                    "fragments load)",
                    &gameOfLifeHoldAlive, false);
  mods::registerMod("Game of Life (hold notes counted as dead cell, before new "
                    "fragments load)",
                    &gameOfLifeHoldDead, true);
  mods::registerMod("Game of Life (hold notes counted as dead cell, after new "
                    "fragments load)",
                    &gameOfLifeHoldDead, false);
  return true;
}();
} // namespace
