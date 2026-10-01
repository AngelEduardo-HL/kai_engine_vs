#ifndef PLAY_HPP
#define PLAY_HPP

#include "scene.hpp"
#include "even_bus.hpp"
#include "raylib.h"

#include <array>
#include <string>
#include <vector>

namespace kai {

    class Play : public Scene, public EventListener {
    public:
        Play() = default;
        ~Play() override = default;

        void init() override;
        void exit() override;
        void update() override;
        void draw() override;
        void onEvent(EventData data) override;

    private:
        static constexpr int TILE_SIZE = 32;
        static constexpr int MAP_COLUMNS = 25;
        static constexpr int MAP_ROWS = 17;
        static constexpr int HUD_Y = MAP_ROWS * TILE_SIZE;

        std::array<std::string, MAP_ROWS> map{};

        Vector2 playerPosition{};
        Vector2 startPosition{};
        Vector2 exitPosition{};

        Vector2 enemyPosition{};
        bool enemyActive = false;
        float enemySpeed = 72.0f;
        std::vector<Vector2> enemyPath;
        int enemyPathIndex = 0;

        int collectedCoins = 0;
        int totalCoins = 0;
        bool victory = false;

        float moveCooldown = 0.0f;
        float enemyHitMessageTime = 0.0f;
        float exitLockedMessageTime = 0.0f;

        void resetLevel();
        void handlePlayerMovement(float deltaTime);
        void tryMovePlayer(int deltaColumn, int deltaRow);
        void collectCoin();
        void checkExit();
        void updateEnemy(float deltaTime);
        void spawnEnemyRandomly();
        void chooseEnemyPatrolTarget();
        bool buildEnemyPath(int targetColumn, int targetRow);

        bool isWallTile(int column, int row) const;

        Rectangle getPlayerBounds() const;
        Rectangle getEnemyBounds() const;
    };

} // namespace kai

#endif // PLAY_HPP
