#include "play.hpp"

#include "raylib.h"

namespace kai {

void Play::init()
{
    stopListening();
    listen("coin_collected");
    resetLevel();
}

void Play::exit()
{
    stopListening();
    entityManager.clear();
}

void Play::resetLevel()
{
    maze.reset();
    player.initialize(maze);
    enemy.initialize(maze);

    entityManager.clear();
    entityManager.add(&player);
    entityManager.add(&enemy);

    collectedCoins = 0;
    victory = false;
    enemyHitMessageTime = 0.0f;
    exitLockedMessageTime = 0.0f;
}

void Play::update()
{
    float deltaTime = GetFrameTime();
    if (deltaTime > 0.05f) {
        deltaTime = 0.05f;
    }

    if (IsKeyPressed(KEY_R)) {
        resetLevel();
        return;
    }

    if (enemyHitMessageTime > 0.0f) {
        enemyHitMessageTime -= deltaTime;
    }
    if (exitLockedMessageTime > 0.0f) {
        exitLockedMessageTime -= deltaTime;
    }

    if (victory) {
        return;
    }

    entityManager.update();
    collectCoin();
    checkExit();

    if (!victory) {
        checkEnemyCollision();
    }
}

void Play::collectCoin()
{
    if (!maze.collectCoinAt(player.getPosition())) {
        return;
    }

    EventData data;
    data.name = "coin";
    data.intVal = 1;
    EventBus::get().fire("coin_collected", data);
}

void Play::checkExit()
{
    if (!maze.isExitAt(player.getPosition())) {
        return;
    }

    if (collectedCoins == maze.getTotalCoins()) {
        victory = true;
    }
    else {
        exitLockedMessageTime = 0.4f;
    }
}

void Play::checkEnemyCollision()
{
    if (!enemy.isActive()) {
        return;
    }

    if (CheckCollisionRecs(player.getBounds(), enemy.getBounds())) {
        player.resetToStart();
        enemyHitMessageTime = 1.5f;
        exitLockedMessageTime = 0.0f;
    }
}

void Play::draw()
{
    maze.draw(collectedCoins);
    entityManager.draw();
    drawHud();

    if (victory) {
        drawVictory();
    }
}

void Play::drawHud() const
{
    DrawRectangle(
        0,
        Maze::HudY,
        800,
        600 - Maze::HudY,
        Color{ 10, 12, 18, 255 });
    DrawText(
        TextFormat("COINS: %d", collectedCoins),
        12,
        Maze::HudY + 12,
        26,
        GOLD);

    const char* statusText = "RECOGE TODAS LAS MONEDAS Y VE A B";
    Color statusColor = LIGHTGRAY;

    if (enemyHitMessageTime > 0.0f) {
        statusText = "ENEMIGO: REGRESAS AL PUNTO A";
        statusColor = RED;
    }
    else if (exitLockedMessageTime > 0.0f) {
        statusText = "SALIDA BLOQUEADA: FALTAN MONEDAS";
        statusColor = ORANGE;
    }
    else if (collectedCoins == maze.getTotalCoins()) {
        statusText = "SALIDA B DESBLOQUEADA";
        statusColor = LIME;
    }

    DrawText(statusText, 180, Maze::HudY + 8, 18, statusColor);
    DrawText("FLECHAS/WASD", 180, Maze::HudY + 31, 14, GRAY);
    DrawText("R: REINICIAR", 660, Maze::HudY + 31, 14, GRAY);
}

void Play::drawVictory() const
{
    DrawRectangle(0, 0, 800, Maze::HudY, Fade(BLACK, 0.72f));

    const char* title = "VICTORIA";
    const int titleSize = 56;
    const int titleWidth = MeasureText(title, titleSize);
    DrawText(title, (800 - titleWidth) / 2, 185, titleSize, GOLD);

    const char* subtitle = "RECOLECTASTE TODAS LAS MONEDAS";
    const int subtitleWidth = MeasureText(subtitle, 24);
    DrawText(subtitle, (800 - subtitleWidth) / 2, 260, 24, WHITE);

    const char* restart = "PRESIONA R PARA REINICIAR";
    const int restartWidth = MeasureText(restart, 20);
    DrawText(restart, (800 - restartWidth) / 2, 305, 20, LIGHTGRAY);
}

void Play::onEvent(EventData data)
{
    if (data.type == "coin_collected") {
        collectedCoins += data.intVal;
    }
}

} // namespace kai
