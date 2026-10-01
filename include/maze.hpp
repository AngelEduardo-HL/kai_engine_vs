#pragma once

#include "raylib.h"

#include <array>
#include <string>
#include <vector>

namespace kai {

class Maze {
public:
    static constexpr int TileSize = 32;
    static constexpr int Columns = 25;
    static constexpr int Rows = 17;
    static constexpr int HudY = Rows * TileSize;

    void reset();
    void draw(int collectedCoins) const;

    bool isWall(int column, int row) const;
    bool collectCoinAt(Vector2 position);
    bool isExitAt(Vector2 position) const;

    Vector2 getStartPosition() const;
    Vector2 getExitPosition() const;
    int getTotalCoins() const;

    Vector2 getRandomPatrolPosition(
        Vector2 origin,
        int minimumTileDistance) const;
    std::vector<Vector2> findPath(
        Vector2 start,
        Vector2 destination) const;

private:
    std::array<std::string, Rows> tiles{};
    Vector2 startPosition{};
    Vector2 exitPosition{};
    int totalCoins = 0;

    int columnAt(Vector2 position) const;
    int rowAt(Vector2 position) const;
};

} // namespace kai
