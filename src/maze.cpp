#include "maze.hpp"

#include <algorithm>
#include <cstdlib>
#include <queue>

namespace {

constexpr std::array<const char*, kai::Maze::Rows> LevelTemplate = {
    "#########################",
    "#A..#.....#.............#",
    "###.#.###.###.#######.###",
    "#...#...#...#.C.....#...#",
    "#.###C#.###.#####.#####.#",
    "#.#...#...#.#B....#...#.#",
    "#.#####.#.#C#######.#.#.#",
    "#.....#.#.#.......#.#...#",
    "#####.###.#######.#.###.#",
    "#.....#C..#.......#...#.#",
    "#.#####.###C#########.#.#",
    "#.......#.#...#.....#.#.#",
    "#########.###.#.###.#C###",
    "#C......#...#...#.#.#...#",
    "#.#####.#.#.#####.#.###.#",
    "#....C#C..#.........C...#",
    "#########################"
};

const Color FloorLight = { 29, 34, 48, 255 };
const Color FloorDark = { 24, 29, 42, 255 };
const Color WallFill = { 45, 74, 138, 255 };
const Color WallEdge = { 92, 139, 230, 255 };

} // namespace

namespace kai {

void Maze::reset()
{
    totalCoins = 0;
    int startCount = 0;
    int exitCount = 0;

    for (int row = 0; row < Rows; ++row) {
        tiles[row] = LevelTemplate[row];

        if (static_cast<int>(tiles[row].size()) != Columns) {
            TraceLog(LOG_ERROR, "Maze row %i does not contain %i tiles", row, Columns);
        }

        for (int column = 0; column < Columns; ++column) {
            const char tile = tiles[row][column];
            const Vector2 tilePosition = {
                static_cast<float>(column * TileSize),
                static_cast<float>(row * TileSize)
            };

            if (tile == 'A') {
                startPosition = tilePosition;
                ++startCount;
            }
            else if (tile == 'B') {
                exitPosition = tilePosition;
                ++exitCount;
            }
            else if (tile == 'C') {
                ++totalCoins;
            }
        }
    }

    if (startCount != 1 || exitCount != 1) {
        TraceLog(LOG_ERROR, "Maze requires exactly one A and one B tile");
    }
}

void Maze::draw(int collectedCoins) const
{
    for (int row = 0; row < Rows; ++row) {
        for (int column = 0; column < Columns; ++column) {
            const Rectangle tileBounds = {
                static_cast<float>(column * TileSize),
                static_cast<float>(row * TileSize),
                static_cast<float>(TileSize),
                static_cast<float>(TileSize)
            };

            const Color floorColor = ((row + column) % 2 == 0)
                ? FloorLight
                : FloorDark;
            DrawRectangleRec(tileBounds, floorColor);

            const char tile = tiles[row][column];

            if (tile == '#') {
                DrawRectangleRec(tileBounds, WallFill);
                DrawRectangleLinesEx(tileBounds, 2.0f, WallEdge);
            }
            else if (tile == 'A') {
                DrawRectangleRec(tileBounds, Fade(GREEN, 0.35f));
                DrawRectangleLinesEx(tileBounds, 2.0f, GREEN);
                DrawText("A", column * TileSize + 9, row * TileSize + 4, 24, GREEN);
            }
            else if (tile == 'B') {
                const Color exitColor = (collectedCoins == totalCoins) ? LIME : ORANGE;
                DrawRectangleRec(tileBounds, Fade(exitColor, 0.35f));
                DrawRectangleLinesEx(tileBounds, 2.0f, exitColor);
                DrawText("B", column * TileSize + 9, row * TileSize + 4, 24, exitColor);
            }
            else if (tile == 'C') {
                const Vector2 center = {
                    column * TileSize + TileSize / 2.0f,
                    row * TileSize + TileSize / 2.0f
                };
                DrawCircleV(center, 9.0f, GOLD);
                DrawCircleLines(
                    static_cast<int>(center.x),
                    static_cast<int>(center.y),
                    9.0f,
                    YELLOW);
            }
        }
    }
}

bool Maze::isWall(int column, int row) const
{
    if (column < 0 || column >= Columns || row < 0 || row >= Rows) {
        return true;
    }

    return tiles[row][column] == '#';
}

bool Maze::collectCoinAt(Vector2 position)
{
    const int column = columnAt(position);
    const int row = rowAt(position);

    if (column < 0 || column >= Columns || row < 0 || row >= Rows) {
        return false;
    }

    if (tiles[row][column] != 'C') {
        return false;
    }

    tiles[row][column] = '.';
    return true;
}

bool Maze::isExitAt(Vector2 position) const
{
    return position.x == exitPosition.x && position.y == exitPosition.y;
}

Vector2 Maze::getStartPosition() const
{
    return startPosition;
}

Vector2 Maze::getExitPosition() const
{
    return exitPosition;
}

int Maze::getTotalCoins() const
{
    return totalCoins;
}

Vector2 Maze::getRandomPatrolPosition(
    Vector2 origin,
    int minimumTileDistance) const
{
    std::array<Vector2, Rows * Columns> candidates{};
    int candidateCount = 0;

    const int originColumn = columnAt(origin);
    const int originRow = rowAt(origin);

    for (int row = 0; row < Rows; ++row) {
        for (int column = 0; column < Columns; ++column) {
            // Empty floor prevents spawning directly on A, B or a coin.
            if (tiles[row][column] != '.') {
                continue;
            }

            const int tileDistance =
                std::abs(column - originColumn) +
                std::abs(row - originRow);

            if (tileDistance < minimumTileDistance) {
                continue;
            }

            candidates[candidateCount++] = {
                static_cast<float>(column * TileSize),
                static_cast<float>(row * TileSize)
            };
        }
    }

    if (candidateCount == 0) {
        return origin;
    }

    return candidates[GetRandomValue(0, candidateCount - 1)];
}

std::vector<Vector2> Maze::findPath(
    Vector2 start,
    Vector2 destination) const
{
    constexpr int CellCount = Rows * Columns;
    std::array<int, CellCount> parent{};
    parent.fill(-1);

    const int startColumn = columnAt(start);
    const int startRow = rowAt(start);
    const int destinationColumn = columnAt(destination);
    const int destinationRow = rowAt(destination);

    if (isWall(startColumn, startRow) ||
        isWall(destinationColumn, destinationRow)) {
        return {};
    }

    const int startIndex = startRow * Columns + startColumn;
    const int destinationIndex =
        destinationRow * Columns + destinationColumn;

    std::queue<int> pending;
    pending.push(startIndex);
    parent[startIndex] = startIndex;

    constexpr std::array<int, 4> ColumnOffsets = { 1, -1, 0, 0 };
    constexpr std::array<int, 4> RowOffsets = { 0, 0, 1, -1 };

    while (!pending.empty() && parent[destinationIndex] == -1) {
        const int currentIndex = pending.front();
        pending.pop();

        const int currentColumn = currentIndex % Columns;
        const int currentRow = currentIndex / Columns;

        for (int directionIndex = 0; directionIndex < 4; ++directionIndex) {
            const int nextColumn =
                currentColumn + ColumnOffsets[directionIndex];
            const int nextRow = currentRow + RowOffsets[directionIndex];

            if (isWall(nextColumn, nextRow)) {
                continue;
            }

            const int nextIndex = nextRow * Columns + nextColumn;
            if (parent[nextIndex] != -1) {
                continue;
            }

            parent[nextIndex] = currentIndex;
            pending.push(nextIndex);
        }
    }

    if (parent[destinationIndex] == -1) {
        return {};
    }

    std::vector<Vector2> reversedPath;
    int currentIndex = destinationIndex;

    while (true) {
        const int column = currentIndex % Columns;
        const int row = currentIndex / Columns;
        reversedPath.push_back({
            static_cast<float>(column * TileSize),
            static_cast<float>(row * TileSize)
        });

        if (currentIndex == startIndex) {
            break;
        }

        currentIndex = parent[currentIndex];
    }

    std::reverse(reversedPath.begin(), reversedPath.end());
    return reversedPath;
}

int Maze::columnAt(Vector2 position) const
{
    return static_cast<int>(position.x) / TileSize;
}

int Maze::rowAt(Vector2 position) const
{
    return static_cast<int>(position.y) / TileSize;
}

} // namespace kai
