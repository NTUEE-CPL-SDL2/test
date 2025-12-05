#include <string>

#include "Game.hpp"
#include "Mods.hpp"

template <uint8_t survive, uint8_t revive, bool hold_alive>
void gameOfLife(Game &game) {
  mystd::vector<mystd::vector<int8_t>> oldHighway(
      game.lanes, mystd::vector<int8_t>(game.fragments, 0));
  for (std::size_t lane = 0; lane < game.lanes; ++lane) {
    for (std::size_t f = 0; f < game.fragments; ++f) {
      oldHighway[lane][f] = game.highway[lane][f];
    }
  }

  auto isAlive = [](int8_t val) -> bool { return val == -1; };

  auto isAliveCountingHold = [](int8_t val) -> bool {
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
            if (isAliveCountingHold(oldHighway[nl][nf]))
              aliveCount++;
          }
        }
      }

      int8_t &cell = game.highway[lane][f];

      if (cell == -1) { // alive
        if (aliveCount != survive) {
          cell = 0; // die
        }
      } else if (cell == 0) { // dead
        if (aliveCount == revive) {
          cell = -1; // born
        }
      }
      // >0 (hold) stays unchanged
    }
  }
}

template <uint8_t survive, uint8_t revive, bool hold_alive>
void registerGameOfLife() {
  mods::registerMod("Game of Life, Survive: " + std::to_string(survive) +
                        ", Revive: " + std::to_string(revive) + ", Hold " +
                        (hold_alive ? "Alive" : "Dead") + ", Before",
                    &gameOfLife<survive, revive, hold_alive>, true);

  mods::registerMod("Game of Life, Survive: " + std::to_string(survive) +
                        ", Revive: " + std::to_string(revive) + ", Hold " +
                        (hold_alive ? "Alive" : "Dead") + ", After",
                    &gameOfLife<survive, revive, hold_alive>, false);
}

template <unsigned char i = 0, unsigned char j = 0> struct RegisterAll {
  static void run() {
    if constexpr (i < 8) {
      if constexpr (j < 8) {
        registerGameOfLife<i, j, true>();
        registerGameOfLife<i, j, false>();
        RegisterAll<i, j + 1>::run();
      } else {
        RegisterAll<i + 1, 0>::run();
      }
    }
  }
};

// Self-register
namespace {
const bool registered = [] {
  RegisterAll<>::run();
  return true;
}();
} // namespace