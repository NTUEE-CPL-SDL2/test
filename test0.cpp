#include <iostream>
#include <cassert>
#include "game.hpp"

void testTapScoring() {
    std::cout << "=== Testing Tap Scoring ===" << std::endl;

    Game game(1, 4, 100);

    game.notes = {
        {0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
    };

    game.loadFragment();
    assert(game.highway[0][0] == -1);

    game.loadFragment();
    assert(game.highway[0][1] == -1);
    assert(game.highway[0][0] == -1);

    game.loadFragment();

    game.loadFragment();
    assert(game.highway[0].back() == -1);

    game.keyPressed(0, 410);
    assert(game.score == 1000);
    assert(game.perfectCount == 1);
    assert(game.combo == 1);
    std::cout << "✓ Perfect tap: OK" << std::endl;

    game.loadFragment();
    assert(game.highway[0].back() == -1);

    game.keyPressed(0, 530);
    assert(game.score == 1000 + 700);
    assert(game.greatCount == 1);
    assert(game.combo == 2);
    std::cout << "✓ Great tap: OK" << std::endl;

    game.loadFragment();
    assert(game.highway[0].back() == -1);

    game.keyPressed(0, 680);
    assert(game.score == 1000 + 700 + 100);
    assert(game.badCount == 1);
    assert(game.combo == 0);
    std::cout << "✓ Bad tap: OK" << std::endl;

    std::cout << "All tap tests passed!" << std::endl << std::endl;
}

void testHoldScenarios() {
    std::cout << "=== Testing Hold Scenarios ===" << std::endl;

    {
        std::cout << "Scenario 1: 3-fragment hold" << std::endl;
        Game game(1, 5, 100);
        game.notes = {{0, 0, 3}};

        game.loadFragment();
        assert(game.highway[0][0] == 3);

        for (int i = 0; i < 4; i++) {
            game.loadFragment();
        }
        assert(game.highway[0].back() == 3);

        game.keyPressed(0, 510);

        game.loadFragment();
        assert(game.highway[0].back() == 2);

        game.loadFragment();
        assert(game.highway[0].back() == 1);

        game.loadFragment();
        assert(game.highway[0].back() == 0);

        assert(game.heldTime > 0);
        std::cout << "✓ Scenario 1 passed" << std::endl;
    }

    {
        std::cout << "Scenario 2: Quick press/release" << std::endl;
        Game game(1, 4, 100);
        game.notes = {{0, 0, 2}};

        game.loadFragment();
        for (int i = 0; i < 3; i++) game.loadFragment();

        assert(game.highway[0].back() == 2);

        game.keyPressed(0, 410);
        uint64_t initialScore = game.score;
        game.keyReleased(0, 460);

        assert(game.score > initialScore);
        assert(game.highway[0].back() == 0);

        std::cout << "✓ Scenario 2 passed" << std::endl;
    }

    {
        std::cout << "Scenario 3: Hold until end" << std::endl;
        Game game(1, 3, 100);
        game.notes = {{0, 0, 2}};

        game.loadFragment();
        game.loadFragment();
        game.loadFragment();

        assert(game.highway[0].back() == 2);

        game.keyPressed(0, 310);
        uint64_t score1 = game.score;

        game.loadFragment();
        assert(game.highway[0].back() == 1);
        uint64_t score2 = game.score;
        assert(score2 > score1);

        game.loadFragment();
        assert(game.highway[0].back() == 0);
        uint64_t score3 = game.score;
        assert(score3 > score2);

        game.keyReleased(0, 510);
        assert(game.score == score3);

        std::cout << "✓ Scenario 3 passed" << std::endl;
    }

    std::cout << "All hold tests passed!" << std::endl << std::endl;
}

void testMixedNotes() {
    std::cout << "=== Testing Mixed Notes ===" << std::endl;

    Game game(2, 4, 100);
    game.notes = {
        {0, 0, -1},
        {0, 1, 2},
        {4, 0, 3},
    };

    uint64_t initialScore = game.score;

    game.loadFragment();
    assert(game.highway[0][0] == -1);
    assert(game.highway[1][0] == 2);

    for (int i = 0; i < 3; i++) game.loadFragment();

    assert(game.highway[0].back() == -1);
    assert(game.highway[1].back() == 2);

    game.keyPressed(0, 410);
    assert(game.perfectCount == 1);
    assert(game.combo == 1);

    game.keyPressed(1, 410);

    game.loadFragment();
    assert(game.highway[1].back() == 1);

    assert(game.highway[0][0] == 3);
    game.loadFragment();
    assert(game.highway[0][0] == 2);
    game.loadFragment();
    assert(game.highway[0][0] == 1);
    game.loadFragment();
    assert(game.highway[0][0] == 0);

    std::cout << "✓ Mixed notes test passed!" << std::endl << std::endl;
}

void testComboTracking() {
    std::cout << "=== Testing Combo Tracking ===" << std::endl;

    Game game(1, 4, 100);
    game.notes = {
        {0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
    };

    for (int i = 0; i < 4; i++) game.loadFragment();
    game.keyPressed(0, 410);
    assert(game.combo == 1);
    assert(game.maxCombo == 1);

    game.loadFragment();
    game.keyPressed(0, 510);
    assert(game.combo == 2);
    assert(game.maxCombo == 2);

    game.loadFragment();
    game.loadFragment();
    assert(game.combo == 0);
    game.keyPressed(0, 710);
    assert(game.combo == 1);
    assert(game.maxCombo == 2);

    game.loadFragment();
    game.keyPressed(0, 810);
    assert(game.combo == 2);
    assert(game.maxCombo == 2);

    std::cout << "✓ Combo tracking test passed!" << std::endl << std::endl;
}

int main() {
    try {
        std::cout << "Starting Game tests..." << std::endl;

        testTapScoring();
        testHoldScenarios();
        testMixedNotes();
        testComboTracking();

        std::cout << "=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown test failure" << std::endl;
        return 1;
    }
}
