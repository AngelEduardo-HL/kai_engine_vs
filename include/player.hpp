#pragma once

#include "entity.h"

namespace kai {

class Maze;

class Player : public Entity {
public:
    Player();

    void initialize(const Maze& maze);
    void resetToStart();

    void update() override;
    void draw() override;

    Vector2 getPosition() const;
    Rectangle getBounds() const;

private:
    const Maze* maze = nullptr;
    Vector2 startPosition{};
    float moveCooldown = 0.0f;

    void tryMove(int deltaColumn, int deltaRow);
};

} // namespace kai
