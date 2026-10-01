#include "player.hpp"

#include "maze.hpp"

namespace {

const Color PlayerFill = { 70, 210, 255, 255 };

} // namespace

namespace kai {

Player::Player()
{
    name = "Player";
    active = true;
}

void Player::initialize(const Maze& levelMaze)
{
    maze = &levelMaze;
    startPosition = maze->getStartPosition();
    resetToStart();
}

void Player::resetToStart()
{
    setPosition(startPosition);
    moveCooldown = 0.0f;
}

void Player::update()
{
    if (!active || maze == nullptr) {
        return;
    }

    float deltaTime = GetFrameTime();
    if (deltaTime > 0.05f) {
        deltaTime = 0.05f;
    }

    const bool up = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    const bool down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    const bool left = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    const bool right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    const bool movementKeyDown = up || down || left || right;

    if (!movementKeyDown) {
        moveCooldown = 0.0f;
        return;
    }

    const bool newDirectionPressed =
        IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
        IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
        IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) ||
        IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);

    if (newDirectionPressed) {
        moveCooldown = 0.0f;
    }
    else {
        moveCooldown -= deltaTime;
    }

    if (moveCooldown > 0.0f) {
        return;
    }

    int deltaColumn = 0;
    int deltaRow = 0;

    if (up) {
        deltaRow = -1;
    }
    else if (down) {
        deltaRow = 1;
    }
    else if (left) {
        deltaColumn = -1;
    }
    else if (right) {
        deltaColumn = 1;
    }

    tryMove(deltaColumn, deltaRow);
    moveCooldown = 0.12f;
}

void Player::draw()
{
    if (!active) {
        return;
    }

    const Rectangle bounds = getBounds();
    DrawRectangleRec(bounds, PlayerFill);
    DrawRectangleLinesEx(bounds, 2.0f, BLUE);
}

Rectangle Player::getBounds() const
{
    return {
        position.x,
        position.y,
        static_cast<float>(Maze::TileSize),
        static_cast<float>(Maze::TileSize)
    };
}

Vector2 Player::getPosition() const
{
    return position;
}

void Player::tryMove(int deltaColumn, int deltaRow)
{
    const int currentColumn = static_cast<int>(position.x) / Maze::TileSize;
    const int currentRow = static_cast<int>(position.y) / Maze::TileSize;
    const int targetColumn = currentColumn + deltaColumn;
    const int targetRow = currentRow + deltaRow;

    if (maze->isWall(targetColumn, targetRow)) {
        return;
    }

    setPosition(
        static_cast<float>(targetColumn * Maze::TileSize),
        static_cast<float>(targetRow * Maze::TileSize));
}

} // namespace kai
