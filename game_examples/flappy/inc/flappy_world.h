#ifndef FLAPPY_WORLD_H
#define FLAPPY_WORLD_H

#include "flappy_types.h"

const DifficultyDef *FlappyDifficultyGet(FlappyDifficulty diff);

// Máy bay
void ResetPlane(Plane *plane);
void FlapPlane(FlappyGame *game);
void UpdatePlaneFlight(FlappyGame *game, float dt);   // Trọng lực + góc nghiêng
void UpdatePlaneHover(FlappyGame *game, float dt);    // Lơ lửng ở màn chờ
bool UpdatePlaneFalling(FlappyGame *game, float dt);  // Rơi xoay sau va chạm; true khi chạm đất
void AnimatePlane(Plane *plane, float dt, float rate);

// Địa hình: đá, sao, mặt đất
void ResetWorld(FlappyGame *game);
void ScrollWorld(FlappyGame *game, float dt);          // Cuộn nền + đất (dùng cả ở menu)
void UpdateObstacles(FlappyGame *game, float dt);      // Sinh/di chuyển đá và sao, tính điểm
float GroundTopUnder(const FlappyGame *game, float worldX);

// Va chạm - trả về true nếu máy bay chạm đá / đất
bool PlaneHitsRock(const FlappyGame *game);
bool PlaneHitsGround(const FlappyGame *game);

// Hiệu ứng
void ClearEffects(FlappyGame *game);
FlappyParticle *EmitPuff(FlappyGame *game, Vector2 pos, Vector2 vel, bool large);
void EmitSparkles(FlappyGame *game, Vector2 pos, Color color, int count);
void EmitCrash(FlappyGame *game, Vector2 pos);
void AddFloatText(FlappyGame *game, const char *text, Vector2 pos, Color color);
void UpdateEffects(FlappyGame *game, float dt, float worldScroll);

// Tiện ích dùng chung với phần vẽ
Vector2 PlaneExhaustPos(const Plane *plane);
int StarValue(StarKind kind);
Color StarColor(StarKind kind);
MedalKind MedalForScore(int score);

#endif // FLAPPY_WORLD_H
