#include "play.hpp"

#include <cmath>

namespace {

    constexpr std::array<const char*, 17> LEVEL_TEMPLATE = {
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

    const Color FLOOR_LIGHT = { 29, 34, 48, 255 };
    const Color FLOOR_DARK = { 24, 29, 42, 255 };
    const Color WALL_FILL = { 45, 74, 138, 255 };
    const Color WALL_EDGE = { 92, 139, 230, 255 };
    const Color PLAYER_FILL = { 70, 210, 255, 255 };
    const Color ENEMY_FILL = { 235, 72, 85, 255 };

} // namespace

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
    }

    void Play::resetLevel()
    {
        for (int row = 0; row < MAP_ROWS; ++row) {
            map[row] = LEVEL_TEMPLATE[row];
        }

        collectedCoins = 0;
        totalCoins = 0;
        victory = false;
        enemyActive = false;
        enemyPath.clear();
        enemyPathIndex = 0;
        moveCooldown = 0.0f;
        enemyHitMessageTime = 0.0f;
        exitLockedMessageTime = 0.0f;

        for (int row = 0; row < MAP_ROWS; ++row) {
            for (int column = 0; column < MAP_COLUMNS; ++column) {
                const char tile = map[row][column];
                const Vector2 tilePosition = {
                    static_cast<float>(column * TILE_SIZE),
                    static_cast<float>(row * TILE_SIZE)
                };

                if (tile == 'A') {
                    startPosition = tilePosition;
                }
                else if (tile == 'B') {
                    exitPosition = tilePosition;
                }
                else if (tile == 'C') {
                    ++totalCoins;
                }
            }
        }

        playerPosition = startPosition;
        spawnEnemyRandomly();
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

        handlePlayerMovement(deltaTime);
        collectCoin();
        checkExit();

        if (victory) {
            return;
        }

        updateEnemy(deltaTime);

        if (enemyActive && CheckCollisionRecs(getPlayerBounds(), getEnemyBounds())) {
            playerPosition = startPosition;
            enemyHitMessageTime = 1.5f;
            exitLockedMessageTime = 0.0f;
        }
    }

    void Play::handlePlayerMovement(float deltaTime)
    {
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

        tryMovePlayer(deltaColumn, deltaRow);
        moveCooldown = 0.12f;
    }

    void Play::tryMovePlayer(int deltaColumn, int deltaRow)
    {
        const int currentColumn = static_cast<int>(playerPosition.x) / TILE_SIZE;
        const int currentRow = static_cast<int>(playerPosition.y) / TILE_SIZE;
        const int targetColumn = currentColumn + deltaColumn;
        const int targetRow = currentRow + deltaRow;

        if (isWallTile(targetColumn, targetRow)) {
            return;
        }

        playerPosition = {
            static_cast<float>(targetColumn * TILE_SIZE),
            static_cast<float>(targetRow * TILE_SIZE)
        };
    }

    void Play::collectCoin()
    {
        const int column = static_cast<int>(playerPosition.x) / TILE_SIZE;
        const int row = static_cast<int>(playerPosition.y) / TILE_SIZE;

        if (map[row][column] != 'C') {
            return;
        }

        map[row][column] = '.';

        EventData data;
        data.name = "coin";
        data.intVal = 1;
        EventBus::get().fire("coin_collected", data);
    }

    void Play::checkExit()
    {
        const bool standingOnExit =
            playerPosition.x == exitPosition.x &&
            playerPosition.y == exitPosition.y;

        if (!standingOnExit) {
            return;
        }

        if (collectedCoins == totalCoins) {
            victory = true;
        }
        else {
            exitLockedMessageTime = 0.4f;
        }
    }

    void Play::updateEnemy(float deltaTime)
    {
        if (!enemyActive) {
            return;
        }

        if (enemyPath.empty() || enemyPathIndex >= static_cast<int>(enemyPath.size())) {
            chooseEnemyPatrolTarget();
        }

        if (!enemyActive || enemyPath.empty()) {
            return;
        }

        const Vector2 targetTile = enemyPath[enemyPathIndex];
        const Vector2 targetPosition = {
            targetTile.x * TILE_SIZE,
            targetTile.y * TILE_SIZE
        };

        const float differenceX = targetPosition.x - enemyPosition.x;
        const float differenceY = targetPosition.y - enemyPosition.y;
        const float distance = std::fabs(differenceX) + std::fabs(differenceY);
        const float movement = enemySpeed * deltaTime;

        if (distance <= movement) {
            enemyPosition = targetPosition;
            ++enemyPathIndex;

            if (enemyPathIndex >= static_cast<int>(enemyPath.size())) {
                chooseEnemyPatrolTarget();
            }
            return;
        }

        if (differenceX != 0.0f) {
            enemyPosition.x += (differenceX > 0.0f) ? movement : -movement;
        }
        else if (differenceY != 0.0f) {
            enemyPosition.y += (differenceY > 0.0f) ? movement : -movement;
        }
    }

    void Play::spawnEnemyRandomly()
    {
        constexpr int TILE_COUNT = MAP_COLUMNS * MAP_ROWS;
        constexpr int MINIMUM_DISTANCE_FROM_A = 60;
        constexpr int DIRECTIONS[4][2] = {
            { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
        };

        std::array<int, TILE_COUNT> distances;
        std::array<int, TILE_COUNT> pendingTiles{};
        distances.fill(-1);

        const int startColumn = static_cast<int>(startPosition.x) / TILE_SIZE;
        const int startRow = static_cast<int>(startPosition.y) / TILE_SIZE;
        const int startIndex = startRow * MAP_COLUMNS + startColumn;

        int queueStart = 0;
        int queueEnd = 0;
        pendingTiles[queueEnd++] = startIndex;
        distances[startIndex] = 0;

        while (queueStart < queueEnd) {
            const int currentIndex = pendingTiles[queueStart++];
            const int currentColumn = currentIndex % MAP_COLUMNS;
            const int currentRow = currentIndex / MAP_COLUMNS;

            for (const auto& direction : DIRECTIONS) {
                const int nextColumn = currentColumn + direction[0];
                const int nextRow = currentRow + direction[1];

                if (isWallTile(nextColumn, nextRow)) {
                    continue;
                }

                const int nextIndex = nextRow * MAP_COLUMNS + nextColumn;
                if (distances[nextIndex] != -1) {
                    continue;
                }

                distances[nextIndex] = distances[currentIndex] + 1;
                pendingTiles[queueEnd++] = nextIndex;
            }
        }

        std::vector<Vector2> spawnCandidates;

        for (int row = 0; row < MAP_ROWS; ++row) {
            for (int column = 0; column < MAP_COLUMNS; ++column) {
                const int index = row * MAP_COLUMNS + column;

                if (map[row][column] == '.' &&
                    distances[index] >= MINIMUM_DISTANCE_FROM_A) {
                    spawnCandidates.push_back(Vector2{
                        static_cast<float>(column),
                        static_cast<float>(row)
                        });
                }
            }
        }

        if (spawnCandidates.empty()) {
            enemyActive = false;
            return;
        }

        const int randomIndex = GetRandomValue(
            0,
            static_cast<int>(spawnCandidates.size()) - 1);
        const Vector2 spawnTile = spawnCandidates[randomIndex];

        enemyPosition = {
            spawnTile.x * TILE_SIZE,
            spawnTile.y * TILE_SIZE
        };
        enemyActive = true;
        enemyPath.clear();
        enemyPathIndex = 0;

        chooseEnemyPatrolTarget();
    }

    void Play::chooseEnemyPatrolTarget()
    {
        constexpr int MINIMUM_PATROL_LENGTH = 12;
        constexpr int MAXIMUM_ATTEMPTS = 32;

        std::vector<Vector2> patrolCandidates;

        for (int row = 0; row < MAP_ROWS; ++row) {
            for (int column = 0; column < MAP_COLUMNS; ++column) {
                if (map[row][column] == '.') {
                    patrolCandidates.push_back(Vector2{
                        static_cast<float>(column),
                        static_cast<float>(row)
                        });
                }
            }
        }

        enemyPath.clear();
        enemyPathIndex = 0;

        if (patrolCandidates.empty()) {
            enemyActive = false;
            return;
        }

        for (int attempt = 0; attempt < MAXIMUM_ATTEMPTS; ++attempt) {
            const int randomIndex = GetRandomValue(
                0,
                static_cast<int>(patrolCandidates.size()) - 1);
            const Vector2 target = patrolCandidates[randomIndex];

            if (buildEnemyPath(
                static_cast<int>(target.x),
                static_cast<int>(target.y)) &&
                static_cast<int>(enemyPath.size()) >= MINIMUM_PATROL_LENGTH) {
                return;
            }
        }

        for (const Vector2 target : patrolCandidates) {
            if (buildEnemyPath(
                static_cast<int>(target.x),
                static_cast<int>(target.y))) {
                return;
            }
        }

        enemyActive = false;
    }

    bool Play::buildEnemyPath(int targetColumn, int targetRow)
    {
        constexpr int TILE_COUNT = MAP_COLUMNS * MAP_ROWS;
        constexpr int DIRECTIONS[4][2] = {
            { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
        };

        if (isWallTile(targetColumn, targetRow)) {
            return false;
        }

        const int startColumn = static_cast<int>(enemyPosition.x) / TILE_SIZE;
        const int startRow = static_cast<int>(enemyPosition.y) / TILE_SIZE;
        const int startIndex = startRow * MAP_COLUMNS + startColumn;
        const int targetIndex = targetRow * MAP_COLUMNS + targetColumn;

        if (startIndex == targetIndex) {
            return false;
        }

        std::array<int, TILE_COUNT> previousTile;
        std::array<int, TILE_COUNT> pendingTiles{};
        previousTile.fill(-1);

        int queueStart = 0;
        int queueEnd = 0;
        pendingTiles[queueEnd++] = startIndex;
        previousTile[startIndex] = startIndex;

        while (queueStart < queueEnd && previousTile[targetIndex] == -1) {
            const int currentIndex = pendingTiles[queueStart++];
            const int currentColumn = currentIndex % MAP_COLUMNS;
            const int currentRow = currentIndex / MAP_COLUMNS;

            for (const auto& direction : DIRECTIONS) {
                const int nextColumn = currentColumn + direction[0];
                const int nextRow = currentRow + direction[1];

                if (isWallTile(nextColumn, nextRow)) {
                    continue;
                }

                const int nextIndex = nextRow * MAP_COLUMNS + nextColumn;
                if (previousTile[nextIndex] != -1) {
                    continue;
                }

                previousTile[nextIndex] = currentIndex;
                pendingTiles[queueEnd++] = nextIndex;
            }
        }

        if (previousTile[targetIndex] == -1) {
            return false;
        }

        std::vector<Vector2> pathInReverse;
        int currentIndex = targetIndex;

        while (currentIndex != startIndex) {
            const int column = currentIndex % MAP_COLUMNS;
            const int row = currentIndex / MAP_COLUMNS;
            pathInReverse.push_back(Vector2{
                static_cast<float>(column),
                static_cast<float>(row)
                });
            currentIndex = previousTile[currentIndex];
        }

        enemyPath.assign(pathInReverse.rbegin(), pathInReverse.rend());
        enemyPathIndex = 0;
        return !enemyPath.empty();
    }

    bool Play::isWallTile(int column, int row) const
    {
        if (column < 0 || column >= MAP_COLUMNS || row < 0 || row >= MAP_ROWS) {
            return true;
        }

        return map[row][column] == '#';
    }

    Rectangle Play::getPlayerBounds() const
    {
        return {
            playerPosition.x,
            playerPosition.y,
            static_cast<float>(TILE_SIZE),
            static_cast<float>(TILE_SIZE)
        };
    }

    Rectangle Play::getEnemyBounds() const
    {
        return {
            enemyPosition.x,
            enemyPosition.y,
            static_cast<float>(TILE_SIZE),
            static_cast<float>(TILE_SIZE)
        };
    }

    void Play::draw()
    {
        for (int row = 0; row < MAP_ROWS; ++row) {
            for (int column = 0; column < MAP_COLUMNS; ++column) {
                const Rectangle tileBounds = {
                    static_cast<float>(column * TILE_SIZE),
                    static_cast<float>(row * TILE_SIZE),
                    static_cast<float>(TILE_SIZE),
                    static_cast<float>(TILE_SIZE)
                };

                const Color floorColor = ((row + column) % 2 == 0)
                    ? FLOOR_LIGHT
                    : FLOOR_DARK;
                DrawRectangleRec(tileBounds, floorColor);

                const char tile = map[row][column];

                if (tile == '#') {
                    DrawRectangleRec(tileBounds, WALL_FILL);
                    DrawRectangleLinesEx(tileBounds, 2.0f, WALL_EDGE);
                }
                else if (tile == 'A') {
                    DrawRectangleRec(tileBounds, Fade(GREEN, 0.35f));
                    DrawRectangleLinesEx(tileBounds, 2.0f, GREEN);
                    DrawText("A", column * TILE_SIZE + 9, row * TILE_SIZE + 4, 24, GREEN);
                }
                else if (tile == 'B') {
                    const Color exitColor = (collectedCoins == totalCoins) ? LIME : ORANGE;
                    DrawRectangleRec(tileBounds, Fade(exitColor, 0.35f));
                    DrawRectangleLinesEx(tileBounds, 2.0f, exitColor);
                    DrawText("B", column * TILE_SIZE + 9, row * TILE_SIZE + 4, 24, exitColor);
                }
                else if (tile == 'C') {
                    const Vector2 center = {
                        column * TILE_SIZE + TILE_SIZE / 2.0f,
                        row * TILE_SIZE + TILE_SIZE / 2.0f
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

        if (enemyActive) {
            const Rectangle enemyBounds = getEnemyBounds();
            DrawRectangleRec(enemyBounds, ENEMY_FILL);
            DrawRectangleLinesEx(enemyBounds, 2.0f, MAROON);
            DrawCircle(
                static_cast<int>(enemyPosition.x + 10.0f),
                static_cast<int>(enemyPosition.y + 11.0f),
                3.0f,
                WHITE);
            DrawCircle(
                static_cast<int>(enemyPosition.x + 22.0f),
                static_cast<int>(enemyPosition.y + 11.0f),
                3.0f,
                WHITE);
        }

        const Rectangle playerBounds = getPlayerBounds();
        DrawRectangleRec(playerBounds, PLAYER_FILL);
        DrawRectangleLinesEx(playerBounds, 2.0f, BLUE);

        DrawRectangle(0, HUD_Y, 800, 600 - HUD_Y, Color{ 10, 12, 18, 255 });
        DrawText(TextFormat("COINS: %d", collectedCoins), 12, HUD_Y + 12, 26, GOLD);

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
        else if (collectedCoins == totalCoins) {
            statusText = "SALIDA B DESBLOQUEADA";
            statusColor = LIME;
        }

        DrawText(statusText, 180, HUD_Y + 8, 18, statusColor);
        DrawText("FLECHAS/WASD", 180, HUD_Y + 31, 14, GRAY);
        DrawText("R: REINICIAR", 660, HUD_Y + 31, 14, GRAY);

        if (victory) {
            DrawRectangle(0, 0, 800, HUD_Y, Fade(BLACK, 0.72f));

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
    }

    void Play::onEvent(EventData data)
    {
        if (data.type == "coin_collected") {
            collectedCoins += data.intVal;
        }
    }

} // namespace kai
