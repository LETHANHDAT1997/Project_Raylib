#include "snake_entities.h"
#include "snake_audio.h"
#include "font_vn.h"
#include "raymath.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define DrawText DrawTextVN
#define MeasureText MeasureTextVN

Vector2 GridToScreen(int gridX, int gridY)
{
    return (Vector2){
        (float)(BOARD_OFFSET_X + gridX * CELL_SIZE),
        (float)(BOARD_OFFSET_Y + gridY * CELL_SIZE)
    };
}

Vector2 GridToScreenCenter(int gridX, int gridY)
{
    return (Vector2){
        (float)(BOARD_OFFSET_X + gridX * CELL_SIZE + CELL_SIZE / 2),
        (float)(BOARD_OFFSET_Y + gridY * CELL_SIZE + CELL_SIZE / 2)
    };
}

bool IsGridOccupiedBySnake(const Snake *snake, int x, int y)
{
    for (int i = 0; i < snake->length; i++) {
        if (snake->segments[i].pos.x == x && snake->segments[i].pos.y == y) {
            return true;
        }
    }
    return false;
}

static bool AreOppositeDirs(SnakeDirection a, SnakeDirection b)
{
    if (a == DIR_UP && b == DIR_DOWN) return true;
    if (a == DIR_DOWN && b == DIR_UP) return true;
    if (a == DIR_LEFT && b == DIR_RIGHT) return true;
    if (a == DIR_RIGHT && b == DIR_LEFT) return true;
    return false;
}

static float GetBaseInterval(SnakeDifficulty diff)
{
    switch (diff) {
        case DIFF_FAST:   return 0.080f;
        case DIFF_INSANE: return 0.052f;
        case DIFF_NORMAL:
        default:          return 0.115f;
    }
}

void InitSnake(Snake *snake, SnakeDifficulty diff)
{
    snake->length = 4;
    snake->dir = DIR_RIGHT;
    snake->nextDir = DIR_RIGHT;
    snake->queuedDir = DIR_NONE;
    snake->moveInterval = GetBaseInterval(diff);
    snake->moveTimer = 0.0f;
    snake->stepProgress = 0.0f;
    snake->alive = true;
    snake->growPending = 0;
    snake->eyeBlinkTimer = 0.0f;
    snake->tongueTimer = 0.0f;
    snake->tongueLength = 0.0f;

    // Vị trí xuất phát ở giữa bàn cờ
    int startX = GRID_COLS / 2;
    int startY = GRID_ROWS / 2;

    for (int i = 0; i < snake->length; i++) {
        snake->segments[i].pos.x = startX - i;
        snake->segments[i].pos.y = startY;
        snake->segments[i].visualPos = GridToScreen(snake->segments[i].pos.x, snake->segments[i].pos.y);
    }
}

void HandleSnakeInput(Snake *snake, bool soundEnabled)
{
    if (!snake->alive) return;

    SnakeDirection reqDir = DIR_NONE;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_K)) {
        reqDir = DIR_UP;
    } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_J)) {
        reqDir = DIR_DOWN;
    } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_H)) {
        reqDir = DIR_LEFT;
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_L)) {
        reqDir = DIR_RIGHT;
    }

    if (reqDir == DIR_NONE) return;

    // Nếu chưa có lệnh rẽ nào đang chờ: cập nhật nextDir nếu không bị quay ngược 180 độ
    if (snake->nextDir == snake->dir) {
        if (!AreOppositeDirs(snake->dir, reqDir) && reqDir != snake->dir) {
            snake->nextDir = reqDir;
            PlaySoundSnakeTurn(soundEnabled);
        }
    } else {
        // Đã có nextDir chờ thực thi: đệm lệnh thứ 2 vào queuedDir
        if (!AreOppositeDirs(snake->nextDir, reqDir) && reqDir != snake->nextDir) {
            snake->queuedDir = reqDir;
        }
    }
}

void SpawnNormalFood(SnakeGame *game)
{
    int maxAttempts = 500;
    int x = 0, y = 0;
    bool found = false;

    while (maxAttempts-- > 0) {
        x = GetRandomValue(0, GRID_COLS - 1);
        y = GetRandomValue(0, GRID_ROWS - 1);

        if (!IsGridOccupiedBySnake(&game->snake, x, y)) {
            if (!game->bonusFood.active || (game->bonusFood.pos.x != x || game->bonusFood.pos.y != y)) {
                found = true;
                break;
            }
        }
    }

    // Nếu bàn cờ quá đầy, duyệt tuần tự
    if (!found) {
        for (int r = 0; r < GRID_ROWS && !found; r++) {
            for (int c = 0; c < GRID_COLS && !found; c++) {
                if (!IsGridOccupiedBySnake(&game->snake, c, r)) {
                    x = c;
                    y = r;
                    found = true;
                }
            }
        }
    }

    game->normalFood.pos.x = x;
    game->normalFood.pos.y = y;
    game->normalFood.type = FOOD_NORMAL;
    game->normalFood.active = true;
    game->normalFood.timer = 0.0f;
    game->normalFood.maxTime = 0.0f;
    game->normalFood.pulse = 0.0f;
}

void SpawnBonusFood(SnakeGame *game)
{
    int maxAttempts = 300;
    int x = 0, y = 0;
    bool found = false;

    while (maxAttempts-- > 0) {
        x = GetRandomValue(0, GRID_COLS - 1);
        y = GetRandomValue(0, GRID_ROWS - 1);

        if (!IsGridOccupiedBySnake(&game->snake, x, y)) {
            if (!game->normalFood.active || (game->normalFood.pos.x != x || game->normalFood.pos.y != y)) {
                found = true;
                break;
            }
        }
    }

    if (!found) return;

    game->bonusFood.pos.x = x;
    game->bonusFood.pos.y = y;
    // 75% cơ hội ra táo vàng, 25% ra quả băng làm chậm
    game->bonusFood.type = (GetRandomValue(0, 100) < 75) ? FOOD_GOLDEN : FOOD_FROST;
    game->bonusFood.active = true;
    game->bonusFood.timer = 9.0f; // Tồn tại 9 giây
    game->bonusFood.maxTime = 9.0f;
    game->bonusFood.pulse = 0.0f;

    // Hiệu ứng hạt xuất hiện
    Vector2 center = GridToScreenCenter(x, y);
    Color spawnCol = (game->bonusFood.type == FOOD_GOLDEN) ? (Color){255, 215, 0, 255} : (Color){56, 189, 248, 255};
    EmitFoodParticles(game, center, spawnCol, 15);
}

void UpdateFood(SnakeGame *game, float dt)
{
    game->normalFood.pulse += dt;

    if (game->bonusFood.active) {
        game->bonusFood.pulse += dt;
        game->bonusFood.timer -= dt;
        if (game->bonusFood.timer <= 0.0f) {
            // Hết giờ thưởng: tạo khói tan biến
            Vector2 center = GridToScreenCenter(game->bonusFood.pos.x, game->bonusFood.pos.y);
            EmitFoodParticles(game, center, (Color){150, 150, 150, 180}, 10);
            game->bonusFood.active = false;
        }
    }
}

void UpdateSnake(SnakeGame *game, float dt)
{
    Snake *snake = &game->snake;
    if (!snake->alive) return;

    // Cập nhật hoạt ảnh mắt & lưỡi
    snake->eyeBlinkTimer += dt;
    if (snake->eyeBlinkTimer > 3.5f) {
        if (snake->eyeBlinkTimer > 3.7f) snake->eyeBlinkTimer = 0.0f;
    }

    snake->tongueTimer += dt;
    if (snake->tongueTimer >= 2.2f) {
        snake->tongueLength = sinf((snake->tongueTimer - 2.2f) * 15.0f);
        if (snake->tongueTimer >= 2.6f) {
            snake->tongueTimer = 0.0f;
            snake->tongueLength = 0.0f;
        }
    }

    // Tốc độ hiện tại dựa trên độ khó, số táo đã ăn và hiệu ứng băng
    float baseInterval = GetBaseInterval(game->difficulty);
    // Tăng tốc độ nhẹ theo độ dài rắn
    float speedFactor = powf(0.985f, (float)(game->applesEaten));
    if (speedFactor < 0.60f) speedFactor = 0.60f;
    float currentInterval = baseInterval * speedFactor;

    // Hiệu ứng quả băng làm chậm 1.5 lần
    if (game->frostTimer > 0.0f) {
        currentInterval *= 1.5f;
    }

    snake->moveInterval = currentInterval;
    snake->moveTimer += dt;
    snake->stepProgress = snake->moveTimer / currentInterval;
    if (snake->stepProgress > 1.0f) snake->stepProgress = 1.0f;

    if (snake->moveTimer >= currentInterval) {
        snake->moveTimer -= currentInterval;
        snake->stepProgress = 0.0f;

        // Áp dụng hướng di chuyển tiếp theo từ bộ đệm
        snake->dir = snake->nextDir;
        if (snake->queuedDir != DIR_NONE) {
            snake->nextDir = snake->queuedDir;
            snake->queuedDir = DIR_NONE;
        }

        // Tính vị trí đầu mới
        GridPos newHead = snake->segments[0].pos;
        switch (snake->dir) {
            case DIR_UP:    newHead.y -= 1; break;
            case DIR_DOWN:  newHead.y += 1; break;
            case DIR_LEFT:  newHead.x -= 1; break;
            case DIR_RIGHT: newHead.x += 1; break;
            default: break;
        }

        // Xử lý va chạm biên / tường
        if (game->mode == MODE_CLASSIC) {
            if (newHead.x < 0 || newHead.x >= GRID_COLS || newHead.y < 0 || newHead.y >= GRID_ROWS) {
                // Đâm tường tử vong
                snake->alive = false;
                game->state = SNAKE_STATE_GAME_OVER;
                game->shakeTimer = 0.45f;
                game->shakeMagnitude = 12.0f;
                PlaySoundSnakeDie(game->soundEnabled);
                EmitDeathParticles(game, GridToScreenCenter(snake->segments[0].pos.x, snake->segments[0].pos.y), 40);
                return;
            }
        } else {
            // MODE_NO_WALLS: Xuyên tường (Torus wrap)
            if (newHead.x < 0) newHead.x = GRID_COLS - 1;
            else if (newHead.x >= GRID_COLS) newHead.x = 0;
            if (newHead.y < 0) newHead.y = GRID_ROWS - 1;
            else if (newHead.y >= GRID_ROWS) newHead.y = 0;
        }

        // Xử lý tự đâm vào thân mình (không tính đuôi nếu đuôi sắp dời đi)
        int checkLimit = snake->length - (snake->growPending > 0 ? 0 : 1);
        for (int i = 0; i < checkLimit; i++) {
            if (snake->segments[i].pos.x == newHead.x && snake->segments[i].pos.y == newHead.y) {
                // Tự cắn đuôi
                snake->alive = false;
                game->state = SNAKE_STATE_GAME_OVER;
                game->shakeTimer = 0.5f;
                game->shakeMagnitude = 14.0f;
                PlaySoundSnakeDie(game->soundEnabled);
                EmitDeathParticles(game, GridToScreenCenter(newHead.x, newHead.y), 45);
                return;
            }
        }

        // Kiểm tra ăn mồi thường (Táo đỏ)
        bool ateFood = false;
        if (game->normalFood.active && newHead.x == game->normalFood.pos.x && newHead.y == game->normalFood.pos.y) {
            ateFood = true;
            game->applesEaten++;
            game->snake.growPending += 1;

            // Tính combo & điểm
            int points = 100 * (1 + game->combo);
            game->score += points;
            game->combo++;
            game->comboTimer = 4.0f;

            char scoreStr[32];
            if (game->combo > 1) {
                snprintf(scoreStr, sizeof(scoreStr), "+%d (x%d)", points, game->combo);
            } else {
                snprintf(scoreStr, sizeof(scoreStr), "+%d", points);
            }

            Vector2 foodCenter = GridToScreenCenter(newHead.x, newHead.y);
            AddFloatingText(game, scoreStr, foodCenter, (Color){248, 113, 113, 255});
            EmitFoodParticles(game, foodCenter, (Color){239, 68, 68, 255}, 18);
            PlaySoundSnakeEat(game->soundEnabled);

            SpawnNormalFood(game);

            // Mỗi 5 quả táo thì sinh 1 quả thưởng đặc biệt nếu chưa có
            if (game->applesEaten % 5 == 0 && !game->bonusFood.active) {
                SpawnBonusFood(game);
            }
        }

        // Kiểm tra ăn mồi thưởng (Táo vàng hoặc Quả băng)
        if (game->bonusFood.active && newHead.x == game->bonusFood.pos.x && newHead.y == game->bonusFood.pos.y) {
            Vector2 bonusCenter = GridToScreenCenter(newHead.x, newHead.y);

            if (game->bonusFood.type == FOOD_GOLDEN) {
                game->goldenEaten++;
                game->snake.growPending += 2;
                int points = 300 * (1 + game->combo);
                game->score += points;
                game->combo += 2;
                game->comboTimer = 5.0f;

                char scoreStr[32];
                snprintf(scoreStr, sizeof(scoreStr), "+%d GOLD!", points);
                AddFloatingText(game, scoreStr, bonusCenter, (Color){255, 215, 0, 255});
                EmitFoodParticles(game, bonusCenter, (Color){251, 191, 36, 255}, 28);
                PlaySoundSnakeGolden(game->soundEnabled);
                game->shakeTimer = 0.20f;
                game->shakeMagnitude = 6.0f;
            } else if (game->bonusFood.type == FOOD_FROST) {
                game->frostTimer = 5.5f; // Chậm trong 5.5 giây
                int points = 150;
                game->score += points;

                AddFloatingText(game, "+150 FREEZE!", bonusCenter, (Color){56, 189, 248, 255});
                EmitFoodParticles(game, bonusCenter, (Color){125, 211, 252, 255}, 22);
                PlaySoundSnakeFrost(game->soundEnabled);
            }

            game->bonusFood.active = false;
        }

        // Cập nhật kỷ lục High Score
        if (game->score > game->highScore) {
            game->highScore = game->score;
        }

        // Thêm đoạn thân mới nếu đang trong trạng thái lớn lên
        if (snake->growPending > 0 && snake->length < MAX_SNAKE_LENGTH) {
            snake->length++;
            snake->growPending--;
        }

        // Dịch chuyển các đốt thân từ sau về trước
        for (int i = snake->length - 1; i > 0; i--) {
            snake->segments[i] = snake->segments[i - 1];
        }
        snake->segments[0].pos = newHead;
        snake->segments[0].visualPos = GridToScreen(newHead.x, newHead.y);

        // Tạo hạt bụi nhỏ khi đầu rắn bò
        if (!ateFood && (GetRandomValue(0, 3) == 0)) {
            Vector2 tailPos = GridToScreenCenter(snake->segments[snake->length - 1].pos.x, snake->segments[snake->length - 1].pos.y);
            EmitTrailParticle(game, tailPos, (Color){74, 222, 128, 120});
        }
    }
}

void DrawSnake(const SnakeGame *game)
{
    const Snake *snake = &game->snake;
    if (snake->length <= 0) return;

    bool isFrozen = (game->frostTimer > 0.0f);

    // 1. Vẽ các đốt thân rắn từ đuôi lên đầu để đầu luôn nằm đè lên trên
    for (int i = snake->length - 1; i >= 0; i--) {
        Vector2 screenPos = GridToScreen(snake->segments[i].pos.x, snake->segments[i].pos.y);
        Rectangle rect = {screenPos.x + 2, screenPos.y + 2, (float)(CELL_SIZE - 4), (float)(CELL_SIZE - 4)};

        // Tạo dải màu gradient dọc thân: Đầu màu sáng, đuôi màu lục đậm / xanh lam
        float t = (float)i / (float)(snake->length > 1 ? snake->length : 1);
        Color bodyColor;

        if (isFrozen) {
            // Theme màu tuyết băng lạnh giá
            bodyColor = (Color){
                (unsigned char)(56 + (int)((30 - 56) * t)),
                (unsigned char)(189 + (int)((144 - 189) * t)),
                (unsigned char)(248 + (int)((255 - 248) * t)),
                255
            };
        } else {
            // Theme màu Neon Emerald -> Dark Teal
            bodyColor = (Color){
                (unsigned char)(46 + (int)((13 - 46) * t)),
                (unsigned char)(204 + (int)((148 - 204) * t)),
                (unsigned char)(113 + (int)((136 - 113) * t)),
                255
            };
        }

        // Nối liền giữa đốt i và đốt i+1 để tạo thân mượt mà (nếu liền kề và không bị bọc màn hình)
        if (i < snake->length - 1) {
            GridPos p1 = snake->segments[i].pos;
            GridPos p2 = snake->segments[i + 1].pos;
            int dx = p1.x - p2.x;
            int dy = p1.y - p2.y;

            // Kiểm tra hai đốt có kế bên nhau không (khoảng cách = 1)
            if (abs(dx) + abs(dy) == 1) {
                Vector2 c1 = GridToScreenCenter(p1.x, p1.y);
                Vector2 c2 = GridToScreenCenter(p2.x, p2.y);
                DrawLineEx(c1, c2, (float)(CELL_SIZE - 6), bodyColor);
            }
        }

        // Vẽ đốt hình chữ nhật bo tròn mềm mại
        DrawRectangleRounded(rect, 0.45f, 6, bodyColor);

        // Vẽ vệt sáng bóng (specular highlight) ở nửa trên đốt
        Rectangle hl = {rect.x + 4, rect.y + 3, rect.width - 8, rect.height * 0.35f};
        DrawRectangleRounded(hl, 0.5f, 4, (Color){255, 255, 255, 45});
    }

    // 2. Vẽ chi tiết đầu rắn
    GridPos headPos = snake->segments[0].pos;
    Vector2 headCenter = GridToScreenCenter(headPos.x, headPos.y);

    // Vẽ lưỡi rắn màu đỏ thè ra thụt vào
    if (snake->tongueLength > 0.05f) {
        float tLen = snake->tongueLength * 14.0f;
        Vector2 tStart = headCenter;
        Vector2 tEnd = headCenter;
        Vector2 fork1 = headCenter;
        Vector2 fork2 = headCenter;

        switch (snake->dir) {
            case DIR_UP:
                tStart.y -= (float)(CELL_SIZE / 2 - 2);
                tEnd.y = tStart.y - tLen;
                fork1 = (Vector2){tEnd.x - 3, tEnd.y - 4};
                fork2 = (Vector2){tEnd.x + 3, tEnd.y - 4};
                break;
            case DIR_DOWN:
                tStart.y += (float)(CELL_SIZE / 2 - 2);
                tEnd.y = tStart.y + tLen;
                fork1 = (Vector2){tEnd.x - 3, tEnd.y + 4};
                fork2 = (Vector2){tEnd.x + 3, tEnd.y + 4};
                break;
            case DIR_LEFT:
                tStart.x -= (float)(CELL_SIZE / 2 - 2);
                tEnd.x = tStart.x - tLen;
                fork1 = (Vector2){tEnd.x - 4, tEnd.y - 3};
                fork2 = (Vector2){tEnd.x - 4, tEnd.y + 3};
                break;
            case DIR_RIGHT:
                tStart.x += (float)(CELL_SIZE / 2 - 2);
                tEnd.x = tStart.x + tLen;
                fork1 = (Vector2){tEnd.x + 4, tEnd.y - 3};
                fork2 = (Vector2){tEnd.x + 4, tEnd.y + 3};
                break;
            default: break;
        }

        DrawLineEx(tStart, tEnd, 2.5f, (Color){239, 68, 68, 255});
        DrawLineEx(tEnd, fork1, 2.0f, (Color){239, 68, 68, 255});
        DrawLineEx(tEnd, fork2, 2.0f, (Color){239, 68, 68, 255});
    }

    // Hai con mắt theo hướng nhìn
    Vector2 eye1 = headCenter;
    Vector2 eye2 = headCenter;
    Vector2 pupilOffset = {0, 0};

    switch (snake->dir) {
        case DIR_UP:
            eye1 = (Vector2){headCenter.x - 6, headCenter.y - 4};
            eye2 = (Vector2){headCenter.x + 6, headCenter.y - 4};
            pupilOffset = (Vector2){0, -2};
            break;
        case DIR_DOWN:
            eye1 = (Vector2){headCenter.x - 6, headCenter.y + 4};
            eye2 = (Vector2){headCenter.x + 6, headCenter.y + 4};
            pupilOffset = (Vector2){0, 2};
            break;
        case DIR_LEFT:
            eye1 = (Vector2){headCenter.x - 4, headCenter.y - 6};
            eye2 = (Vector2){headCenter.x - 4, headCenter.y + 6};
            pupilOffset = (Vector2){-2, 0};
            break;
        case DIR_RIGHT:
            eye1 = (Vector2){headCenter.x + 4, headCenter.y - 6};
            eye2 = (Vector2){headCenter.x + 4, headCenter.y + 6};
            pupilOffset = (Vector2){2, 0};
            break;
        default: break;
    }

    bool blinking = (snake->eyeBlinkTimer > 3.5f && snake->eyeBlinkTimer <= 3.7f);

    if (blinking) {
        // Nhắm mắt khi chớp
        DrawLineEx((Vector2){eye1.x - 3, eye1.y}, (Vector2){eye1.x + 3, eye1.y}, 2.0f, (Color){20, 30, 25, 255});
        DrawLineEx((Vector2){eye2.x - 3, eye2.y}, (Vector2){eye2.x + 3, eye2.y}, 2.0f, (Color){20, 30, 25, 255});
    } else {
        // Tròng trắng
        DrawCircleV(eye1, 4.5f, RAYWHITE);
        DrawCircleV(eye2, 4.5f, RAYWHITE);

        // Đồng tử đen liếc theo hướng
        DrawCircleV(Vector2Add(eye1, pupilOffset), 2.2f, (Color){15, 23, 42, 255});
        DrawCircleV(Vector2Add(eye2, pupilOffset), 2.2f, (Color){15, 23, 42, 255});

        // Điểm sáng li ti trong mắt
        DrawCircleV((Vector2){eye1.x - 1, eye1.y - 1}, 1.0f, RAYWHITE);
        DrawCircleV((Vector2){eye2.x - 1, eye2.y - 1}, 1.0f, RAYWHITE);
    }
}

void DrawFood(const FoodItem *food, float globalTime)
{
    if (!food->active) return;

    Vector2 center = GridToScreenCenter(food->pos.x, food->pos.y);
    float pulseScale = 1.0f + 0.08f * sinf(food->pulse * 6.0f);

    if (food->type == FOOD_NORMAL) {
        // Táo đỏ mọng nước
        float radius = (CELL_SIZE * 0.38f) * pulseScale;

        // Bóng đổ nhẹ dưới quả
        DrawEllipse((int)center.x, (int)(center.y + radius * 0.9f), radius * 0.85f, radius * 0.35f, (Color){0, 0, 0, 60});

        // Thân quả táo (2 hình tròn khẽ lồng nhau)
        DrawCircleV((Vector2){center.x - radius * 0.28f, center.y}, radius * 0.88f, (Color){239, 68, 68, 255});
        DrawCircleV((Vector2){center.x + radius * 0.28f, center.y}, radius * 0.88f, (Color){220, 38, 38, 255});

        // Vệt phản chiếu ánh sáng
        DrawCircleV((Vector2){center.x - radius * 0.35f, center.y - radius * 0.35f}, radius * 0.28f, (Color){254, 202, 202, 200});

        // Cuống táo màu nâu
        Vector2 stemStart = {center.x, center.y - radius * 0.75f};
        Vector2 stemEnd = {center.x + 3.0f, center.y - radius * 1.35f};
        DrawLineEx(stemStart, stemEnd, 2.2f, (Color){120, 53, 15, 255});

        // Chiếc lá xanh bé xinh
        Vector2 leafPos = {center.x + 5.0f, center.y - radius * 1.25f};
        DrawCircleV(leafPos, 3.2f, (Color){34, 197, 94, 255});
    }
    else if (food->type == FOOD_GOLDEN) {
        // Táo vàng hào quang rực rỡ
        float radius = (CELL_SIZE * 0.42f) * pulseScale;

        // Vòng phát sáng vàng kim
        float haloAlpha = 0.25f + 0.15f * sinf(globalTime * 8.0f);
        DrawCircleV(center, radius * 1.6f, Fade((Color){255, 215, 0, 255}, haloAlpha));

        // Quả táo vàng óng
        DrawCircleV((Vector2){center.x - radius * 0.25f, center.y}, radius * 0.9f, (Color){251, 191, 36, 255});
        DrawCircleV((Vector2){center.x + radius * 0.25f, center.y}, radius * 0.9f, (Color){245, 158, 11, 255});

        // Ánh kim phản chiếu
        DrawCircleV((Vector2){center.x - radius * 0.35f, center.y - radius * 0.35f}, radius * 0.32f, (Color){254, 240, 138, 240});

        // Cuống táo
        Vector2 stemStart = {center.x, center.y - radius * 0.75f};
        Vector2 stemEnd = {center.x + 2.5f, center.y - radius * 1.3f};
        DrawLineEx(stemStart, stemEnd, 2.5f, (Color){180, 83, 9, 255});

        // Lấp lánh 4 cánh xoay
        float sparkRot = globalTime * 3.0f;
        float sparkLen = 5.0f;
        Vector2 spOffset = {cosf(sparkRot) * sparkLen, sinf(sparkRot) * sparkLen};
        Vector2 spCenter = {center.x + radius * 0.5f, center.y - radius * 0.5f};
        DrawLineEx(Vector2Subtract(spCenter, spOffset), Vector2Add(spCenter, spOffset), 1.8f, RAYWHITE);
        Vector2 spOffsetCross = {-spOffset.y, spOffset.x};
        DrawLineEx(Vector2Subtract(spCenter, spOffsetCross), Vector2Add(spCenter, spOffsetCross), 1.8f, RAYWHITE);

        // Vòng tròn thời gian đếm ngược
        float timeRatio = food->timer / food->maxTime;
        if (timeRatio < 0.0f) timeRatio = 0.0f;
        DrawRing(center, radius * 1.35f, radius * 1.55f, -90.0f, -90.0f + 360.0f * timeRatio, 32, (Color){255, 215, 0, 220});
    }
    else if (food->type == FOOD_FROST) {
        // Tinh thể băng tuyết (Diamond Gem)
        float size = (CELL_SIZE * 0.40f) * pulseScale;

        // Vòng phát sáng xanh băng
        DrawCircleV(center, size * 1.5f, Fade((Color){56, 189, 248, 255}, 0.25f));

        // Hình thoi tinh thể băng
        Vector2 pTop = {center.x, center.y - size};
        Vector2 pBottom = {center.x, center.y + size};
        Vector2 pLeft = {center.x - size, center.y};
        Vector2 pRight = {center.x + size, center.y};

        DrawTriangle(pTop, pLeft, center, (Color){125, 211, 252, 255});
        DrawTriangle(pTop, center, pRight, (Color){56, 189, 248, 255});
        DrawTriangle(center, pLeft, pBottom, (Color){2, 132, 199, 255});
        DrawTriangle(pRight, center, pBottom, (Color){14, 165, 233, 255});

        // Điểm sáng trắng trung tâm
        DrawCircleV(center, 2.5f, RAYWHITE);

        // Vòng đếm ngược
        float timeRatio = food->timer / food->maxTime;
        if (timeRatio < 0.0f) timeRatio = 0.0f;
        DrawRing(center, size * 1.35f, size * 1.55f, -90.0f, -90.0f + 360.0f * timeRatio, 32, (Color){56, 189, 248, 220});
    }
}

void InitParticles(SnakeGame *game)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        game->particles[i].active = false;
    }
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        game->floatingTexts[i].active = false;
    }
}

void EmitFoodParticles(SnakeGame *game, Vector2 pos, Color color, int count)
{
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (!game->particles[i].active) {
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(50, 180);
            game->particles[i].pos = pos;
            game->particles[i].vel = (Vector2){cosf(angle) * speed, sinf(angle) * speed};
            game->particles[i].color = color;
            game->particles[i].size = (float)GetRandomValue(3, 7);
            game->particles[i].life = (float)GetRandomValue(35, 65) / 100.0f;
            game->particles[i].maxLife = game->particles[i].life;
            game->particles[i].active = true;
            spawned++;
        }
    }
}

void EmitDeathParticles(SnakeGame *game, Vector2 pos, int count)
{
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (!game->particles[i].active) {
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(70, 260);
            game->particles[i].pos = pos;
            game->particles[i].vel = (Vector2){cosf(angle) * speed, sinf(angle) * speed};

            // Hạt văng đa màu từ thân rắn
            Color pColors[] = {
                (Color){46, 204, 113, 255},
                (Color){39, 174, 96, 255},
                (Color){241, 196, 15, 255},
                (Color){231, 76, 60, 255}
            };
            game->particles[i].color = pColors[GetRandomValue(0, 3)];
            game->particles[i].size = (float)GetRandomValue(4, 10);
            game->particles[i].life = (float)GetRandomValue(50, 110) / 100.0f;
            game->particles[i].maxLife = game->particles[i].life;
            game->particles[i].active = true;
            spawned++;
        }
    }
}

void EmitTrailParticle(SnakeGame *game, Vector2 pos, Color color)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!game->particles[i].active) {
            game->particles[i].pos = (Vector2){
                pos.x + (float)GetRandomValue(-4, 4),
                pos.y + (float)GetRandomValue(-4, 4)
            };
            game->particles[i].vel = (Vector2){
                (float)GetRandomValue(-15, 15),
                (float)GetRandomValue(-15, 15)
            };
            game->particles[i].color = color;
            game->particles[i].size = (float)GetRandomValue(2, 4);
            game->particles[i].life = 0.35f;
            game->particles[i].maxLife = 0.35f;
            game->particles[i].active = true;
            break;
        }
    }
}

void UpdateParticles(SnakeGame *game, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (game->particles[i].active) {
            game->particles[i].life -= dt;
            if (game->particles[i].life <= 0.0f) {
                game->particles[i].active = false;
            } else {
                game->particles[i].pos = Vector2Add(game->particles[i].pos, Vector2Scale(game->particles[i].vel, dt));
                game->particles[i].vel = Vector2Scale(game->particles[i].vel, 0.94f); // Ma sát không khí
            }
        }
    }
}

void DrawParticles(const SnakeGame *game)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (game->particles[i].active) {
            float progress = game->particles[i].life / game->particles[i].maxLife;
            Color c = Fade(game->particles[i].color, progress);
            float curSize = game->particles[i].size * progress;
            DrawRectangleV((Vector2){game->particles[i].pos.x - curSize / 2, game->particles[i].pos.y - curSize / 2}, (Vector2){curSize, curSize}, c);
        }
    }
}

void AddFloatingText(SnakeGame *game, const char *text, Vector2 pos, Color color)
{
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (!game->floatingTexts[i].active) {
            strncpy(game->floatingTexts[i].text, text, sizeof(game->floatingTexts[i].text) - 1);
            game->floatingTexts[i].text[sizeof(game->floatingTexts[i].text) - 1] = '\0';
            game->floatingTexts[i].pos = (Vector2){pos.x, pos.y - 10.0f};
            game->floatingTexts[i].color = color;
            game->floatingTexts[i].life = 0.85f;
            game->floatingTexts[i].maxLife = 0.85f;
            game->floatingTexts[i].active = true;
            break;
        }
    }
}

void UpdateFloatingTexts(SnakeGame *game, float dt)
{
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (game->floatingTexts[i].active) {
            game->floatingTexts[i].life -= dt;
            if (game->floatingTexts[i].life <= 0.0f) {
                game->floatingTexts[i].active = false;
            } else {
                game->floatingTexts[i].pos.y -= 38.0f * dt; // Trôi lên phía trên
            }
        }
    }
}

void DrawFloatingTexts(const SnakeGame *game)
{
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (game->floatingTexts[i].active) {
            float alpha = game->floatingTexts[i].life / game->floatingTexts[i].maxLife;
            Color textColor = Fade(game->floatingTexts[i].color, alpha);
            int textW = MeasureText(game->floatingTexts[i].text, 18);
            int posX = (int)(game->floatingTexts[i].pos.x - textW / 2);
            int posY = (int)(game->floatingTexts[i].pos.y);

            // Viền đổ bóng đen nổi bật
            DrawText(game->floatingTexts[i].text, posX + 1, posY + 1, 18, Fade(BLACK, alpha * 0.7f));
            DrawText(game->floatingTexts[i].text, posX, posY, 18, textColor);
        }
    }
}
