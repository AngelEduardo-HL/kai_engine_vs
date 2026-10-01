#include "enemy.h"

#include "maze.hpp"

#include <cmath>
#include <utility>

namespace {

const Color EnemyFill = { 235, 72, 85, 255 };

} // namespace

namespace kai {

Enemy::Enemy()
{
    name = "Enemy";
    active = true;
}

void Enemy::initialize(const Maze& levelMaze)
{
    maze = &levelMaze;
    setPosition(maze->getRandomPatrolPosition(
        maze->getStartPosition(),
        10));
    active = true;
    chooseNextPatrolRoute();
}

void Enemy::update()
{
    if (!active || maze == nullptr) {
        return;
    }

    float deltaTime = GetFrameTime();
    if (deltaTime > 0.05f) {
        deltaTime = 0.05f;
    }

    if (nextWaypoint >= patrolPath.size()) {
        chooseNextPatrolRoute();
    }

    if (nextWaypoint >= patrolPath.size()) {
        return;
    }

    const Vector2 target = patrolPath[nextWaypoint];
    const float differenceX = target.x - position.x;
    const float differenceY = target.y - position.y;
    const float distance = std::abs(differenceX) + std::abs(differenceY);
    const float movement = speed * deltaTime;

    if (distance <= movement) {
        setPosition(target);
        ++nextWaypoint;
        return;
    }

    Vector2 nextPosition = position;
    if (std::abs(differenceX) > 0.01f) {
        nextPosition.x += (differenceX > 0.0f) ? movement : -movement;
    }
    else {
        nextPosition.y += (differenceY > 0.0f) ? movement : -movement;
    }

    setPosition(nextPosition);
}

void Enemy::chooseNextPatrolRoute()
{
    patrolPath.clear();
    nextWaypoint = 0;

    // Destinations stay far apart, making the enemy travel through the maze
    // instead of pacing repeatedly over the same two or three tiles.
    for (int attempt = 0; attempt < 12; ++attempt) {
        const Vector2 destination =
            maze->getRandomPatrolPosition(position, 8);
        std::vector<Vector2> route = maze->findPath(position, destination);

        if (route.size() < 2) {
            continue;
        }

        patrolPath = std::move(route);
        nextWaypoint = 1;
        return;
    }
}

void Enemy::draw()
{
    if (!active) {
        return;
    }

    const Rectangle bounds = getBounds();
    DrawRectangleRec(bounds, EnemyFill);
    DrawRectangleLinesEx(bounds, 2.0f, MAROON);
    DrawCircle(
        static_cast<int>(position.x + 10.0f),
        static_cast<int>(position.y + 11.0f),
        3.0f,
        WHITE);
    DrawCircle(
        static_cast<int>(position.x + 22.0f),
        static_cast<int>(position.y + 11.0f),
        3.0f,
        WHITE);
}

Rectangle Enemy::getBounds() const
{
    return {
        position.x,
        position.y,
        static_cast<float>(Maze::TileSize),
        static_cast<float>(Maze::TileSize)
    };
}

} // namespace kai
