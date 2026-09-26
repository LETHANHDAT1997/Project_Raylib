#ifndef SNAKE_ENTITIES_H
#define SNAKE_ENTITIES_H

#include "snake_types.h"

// Khởi tạo và thiết lập trạng thái rắn
void InitSnake(Snake *snake, SnakeDifficulty diff);
void HandleSnakeInput(Snake *snake, bool soundEnabled);
void UpdateSnake(SnakeGame *game, float dt);
void DrawSnake(const SnakeGame *game);

// Quản lý thức ăn
void SpawnNormalFood(SnakeGame *game);
void SpawnBonusFood(SnakeGame *game);
void UpdateFood(SnakeGame *game, float dt);
void DrawFood(const FoodItem *food, float globalTime);

// Hệ thống hạt hiệu ứng (Particles)
void InitParticles(SnakeGame *game);
void EmitFoodParticles(SnakeGame *game, Vector2 pos, Color color, int count);
void EmitDeathParticles(SnakeGame *game, Vector2 pos, int count);
void EmitTrailParticle(SnakeGame *game, Vector2 pos, Color color);
void UpdateParticles(SnakeGame *game, float dt);
void DrawParticles(const SnakeGame *game);

// Hiệu ứng chữ số điểm nổi (Floating Text)
void AddFloatingText(SnakeGame *game, const char *text, Vector2 pos, Color color);
void UpdateFloatingTexts(SnakeGame *game, float dt);
void DrawFloatingTexts(const SnakeGame *game);

// Tiện ích chuyển đổi tọa độ ô cờ sang tọa độ màn hình
Vector2 GridToScreen(int gridX, int gridY);
Vector2 GridToScreenCenter(int gridX, int gridY);
bool IsGridOccupiedBySnake(const Snake *snake, int x, int y);

#endif // SNAKE_ENTITIES_H
