#ifndef SNAKE_RUNNER_H
#define SNAKE_RUNNER_H

void InitSnakeApp(void);
void UpdateSnakeApp(float dt);
void DrawSnakeApp(void);
void CloseSnakeApp(void);

// Chuyển file kỷ lục kiểu cũ (snake_highscore.dat) sang thư mục dữ liệu chung.
void MigrateSnakeSave(void);

#endif // SNAKE_RUNNER_H
