#ifndef FIGHTER_RUNNER_H
#define FIGHTER_RUNNER_H

// API kiểu C để Arcade Hub (viết bằng C) gọi được game viết bằng C++.
#ifdef __cplusplus
extern "C" {
#endif

void InitFighterApp(void);
void UpdateFighterApp(float dt);
void DrawFighterApp(void);
void CloseFighterApp(void);

#ifdef __cplusplus
}
#endif

#endif // FIGHTER_RUNNER_H
