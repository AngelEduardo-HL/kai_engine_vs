#ifndef PLAY_HPP
#define PLAY_HPP

#include "scene.hpp"
#include "even_bus.hpp"
#include "entity_manager.h"
#include "enemy.h"
#include "maze.hpp"
#include "player.hpp"

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
    Maze maze;
    Player player;
    Enemy enemy;
    EntityManager entityManager;

    int collectedCoins = 0;
    bool victory = false;
    float enemyHitMessageTime = 0.0f;
    float exitLockedMessageTime = 0.0f;

    void resetLevel();
    void collectCoin();
    void checkExit();
    void checkEnemyCollision();
    void drawHud() const;
    void drawVictory() const;
};

} // namespace kai

#endif // PLAY_HPP
