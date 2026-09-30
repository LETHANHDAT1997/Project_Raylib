#ifndef FLAPPY_RUNNER_H
#define FLAPPY_RUNNER_H

void InitFlappyApp(void);
void UpdateFlappyApp(float dt);
void DrawFlappyApp(void);
void CloseFlappyApp(void);

// Chuyển file kỷ lục kiểu cũ (flappy_highscore.dat) sang thư mục dữ liệu chung.
void MigrateFlappySave(void);

#endif // FLAPPY_RUNNER_H
