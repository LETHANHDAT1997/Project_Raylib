#include "snake_game.h"
#include "snake_entities.h"
#include "snake_audio.h"
#include "font_vn.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

#define DrawText DrawTextVN
#define MeasureText MeasureTextVN

static const char *HIGH_SCORE_FILE = "snake_highscore.dat";

static int LoadHighScoreFromFile(void)
{
    FILE *f = fopen(HIGH_SCORE_FILE, "rb");
    if (!f) return 0;
    int score = 0;
    if (fread(&score, sizeof(int), 1, f) != 1) {
        score = 0;
    }
    fclose(f);
    return score;
}

static void SaveHighScoreToFile(int score)
{
    FILE *f = fopen(HIGH_SCORE_FILE, "wb");
    if (f) {
        fwrite(&score, sizeof(int), 1, f);
        fclose(f);
    }
}

void InitSnakeGame(SnakeGame *game)
{
    game->highScore = LoadHighScoreFromFile();
    game->score = 0;
    game->applesEaten = 0;
    game->goldenEaten = 0;
    game->combo = 0;
    game->comboTimer = 0.0f;
    game->state = SNAKE_STATE_MENU;
    game->mode = MODE_CLASSIC;
    game->difficulty = DIFF_NORMAL;
    game->soundEnabled = true;
    game->frostTimer = 0.0f;
    game->shakeTimer = 0.0f;
    game->shakeMagnitude = 0.0f;
    game->globalTime = 0.0f;
    game->stateTime = 0.0f;

    InitParticles(game);
    InitSnake(&game->snake, game->difficulty);
    SpawnNormalFood(game);
    game->bonusFood.active = false;
}

void ResetSnakeGame(SnakeGame *game)
{
    game->score = 0;
    game->applesEaten = 0;
    game->goldenEaten = 0;
    game->combo = 0;
    game->comboTimer = 0.0f;
    game->frostTimer = 0.0f;
    game->shakeTimer = 0.0f;
    game->shakeMagnitude = 0.0f;
    game->stateTime = 0.0f;

    InitParticles(game);
    InitSnake(&game->snake, game->difficulty);
    SpawnNormalFood(game);
    game->bonusFood.active = false;

    game->state = SNAKE_STATE_PLAYING;
}

void CloseSnakeGame(SnakeGame *game)
{
    if (game->score > game->highScore) {
        game->highScore = game->score;
    }
    SaveHighScoreToFile(game->highScore);
}

void UpdateSnakeGame(SnakeGame *game, float dt)
{
    game->globalTime += dt;
    game->stateTime += dt;

    // Rung màn hình (Screen shake)
    if (game->shakeTimer > 0.0f) {
        game->shakeTimer -= dt;
    }

    // Hiệu ứng làm chậm bằng băng
    if (game->frostTimer > 0.0f) {
        game->frostTimer -= dt;
    }

    // Bộ đếm Combo ăn táo liên tiếp
    if (game->comboTimer > 0.0f) {
        game->comboTimer -= dt;
        if (game->comboTimer <= 0.0f) {
            game->combo = 0;
        }
    }

    // Xử lý theo từng trạng thái game
    switch (game->state) {
        case SNAKE_STATE_MENU: {
            // Nhấn Space hoặc Enter để bắt đầu chơi
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                ResetSnakeGame(game);
                PlaySoundSnakeClick(game->soundEnabled);
            }
            // Tab đổi chế độ tường
            if (IsKeyPressed(KEY_TAB)) {
                game->mode = (game->mode == MODE_CLASSIC) ? MODE_NO_WALLS : MODE_CLASSIC;
                PlaySoundSnakeClick(game->soundEnabled);
            }
            // Phím D đổi xoay vòng độ khó (không cần phím số)
            if (IsKeyPressed(KEY_D)) {
                game->difficulty = (game->difficulty + 1) % 3;
                PlaySoundSnakeClick(game->soundEnabled);
            }
            // 1, 2, 3 (cả hàng số lẫn numpad) đổi độ khó
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
                game->difficulty = DIFF_NORMAL;
                PlaySoundSnakeClick(game->soundEnabled);
            } else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
                game->difficulty = DIFF_FAST;
                PlaySoundSnakeClick(game->soundEnabled);
            } else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
                game->difficulty = DIFF_INSANE;
                PlaySoundSnakeClick(game->soundEnabled);
            }
            // M bật/tắt âm thanh
            if (IsKeyPressed(KEY_M)) {
                game->soundEnabled = !game->soundEnabled;
            }
            break;
        }

        case SNAKE_STATE_PLAYING: {
            // Tạm dừng
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                game->state = SNAKE_STATE_PAUSED;
                PlaySoundSnakePause(game->soundEnabled);
                break;
            }

            // Tab đổi chế độ
            if (IsKeyPressed(KEY_TAB)) {
                game->mode = (game->mode == MODE_CLASSIC) ? MODE_NO_WALLS : MODE_CLASSIC;
                PlaySoundSnakeClick(game->soundEnabled);
            }

            // M bật/tắt âm thanh
            if (IsKeyPressed(KEY_M)) {
                game->soundEnabled = !game->soundEnabled;
            }

            // Xử lý điều khiển, di chuyển, thức ăn và hạt
            HandleSnakeInput(&game->snake, game->soundEnabled);
            UpdateSnake(game, dt);
            UpdateFood(game, dt);
            UpdateParticles(game, dt);
            UpdateFloatingTexts(game, dt);

            // Tự động lưu High score nếu lập kỷ lục mới
            if (game->score > game->highScore) {
                game->highScore = game->score;
                SaveHighScoreToFile(game->highScore);
            }
            break;
        }

        case SNAKE_STATE_PAUSED: {
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
                game->state = SNAKE_STATE_PLAYING;
                PlaySoundSnakePause(game->soundEnabled);
            } else if (IsKeyPressed(KEY_R)) {
                ResetSnakeGame(game);
                PlaySoundSnakeClick(game->soundEnabled);
            } else if (IsKeyPressed(KEY_M)) {
                game->soundEnabled = !game->soundEnabled;
            }
            break;
        }

        case SNAKE_STATE_GAME_OVER: {
            UpdateParticles(game, dt);
            UpdateFloatingTexts(game, dt);

            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
                ResetSnakeGame(game);
                PlaySoundSnakeClick(game->soundEnabled);
            } else if (IsKeyPressed(KEY_ESCAPE)) {
                game->state = SNAKE_STATE_MENU;
                PlaySoundSnakeClick(game->soundEnabled);
            } else if (IsKeyPressed(KEY_M)) {
                game->soundEnabled = !game->soundEnabled;
            }
            break;
        }
    }
}

// Vẽ bàn cờ ô vuông kiểu Checkerboard hiện đại
static void DrawBoardGrid(const SnakeGame *game)
{
    // Nền bàn cờ
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            Vector2 p = GridToScreen(c, r);
            Rectangle tileRect = {p.x, p.y, (float)CELL_SIZE, (float)CELL_SIZE};

            Color tileCol;
            if ((r + c) % 2 == 0) {
                tileCol = (Color){26, 36, 52, 255}; // Ô màu slate tối A
            } else {
                tileCol = (Color){21, 29, 43, 255}; // Ô màu slate tối B
            }

            // Ánh xanh băng nếu quả tuyết đang hoạt động
            if (game->frostTimer > 0.0f) {
                float frostRatio = game->frostTimer / 5.5f;
                tileCol = (Color){
                    (unsigned char)(tileCol.r + 15 * frostRatio),
                    (unsigned char)(tileCol.g + 35 * frostRatio),
                    (unsigned char)(tileCol.b + 55 * frostRatio),
                    255
                };
            }

            DrawRectangleRec(tileRect, tileCol);

            // Điểm giao chấm nhỏ giữa các ô (Grid dot pattern)
            DrawCircle((int)p.x, (int)p.y, 1.0f, (Color){50, 65, 85, 90});
        }
    }

    // Khung viền bàn cờ
    Rectangle boardRect = {
        (float)BOARD_OFFSET_X,
        (float)BOARD_OFFSET_Y,
        (float)BOARD_WIDTH,
        (float)BOARD_HEIGHT
    };

    if (game->mode == MODE_CLASSIC) {
        // Viền đỏ báo hiệu chạm vào sẽ chết (Deadly walls)
        DrawRectangleLinesEx(boardRect, 3.0f, (Color){239, 68, 68, 220});
        // Ánh hào quang nhẹ bên ngoài viền
        Rectangle outerGlow = {boardRect.x - 2, boardRect.y - 2, boardRect.width + 4, boardRect.height + 4};
        DrawRectangleLinesEx(outerGlow, 1.0f, (Color){239, 68, 68, 80});
    } else {
        // Viền xanh ngọc báo hiệu cổng xuyên không gian (Portal wrap)
        DrawRectangleLinesEx(boardRect, 3.0f, (Color){14, 165, 233, 220});
        Rectangle outerGlow = {boardRect.x - 2, boardRect.y - 2, boardRect.width + 4, boardRect.height + 4};
        DrawRectangleLinesEx(outerGlow, 1.0f, (Color){14, 165, 233, 80});
    }
}

// Vẽ thanh trạng thái / Bảng điều khiển bên phải (HUD Sidebar)
static void DrawSidebar(const SnakeGame *game)
{
    int startX = BOARD_OFFSET_X + BOARD_WIDTH + 30; // 40 + 640 + 30 = 710
    int width = 210;

    // Panel 1: ĐIỂM SỐ & KỶ LỤC (Score & Highscore)
    Rectangle p1 = {(float)startX, 40, (float)width, 140};
    DrawRectangleRounded(p1, 0.12f, 8, (Color){26, 36, 52, 220});
    DrawRectangleRoundedLinesEx(p1, 0.12f, 8, 1.5f, (Color){51, 65, 85, 255});

    DrawText("SCORE", startX + 16, 52, 13, (Color){148, 163, 184, 255});
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "%05d", game->score);
    DrawText(scoreStr, startX + 16, 68, 30, (Color){52, 211, 153, 255});

    DrawLine(startX + 16, 108, startX + width - 16, 108, (Color){45, 58, 77, 255});

    DrawText("HIGH SCORE", startX + 16, 116, 12, (Color){251, 191, 36, 255});
    char hsStr[32];
    snprintf(hsStr, sizeof(hsStr), "%05d", game->highScore);
    DrawText(hsStr, startX + 16, 134, 24, (Color){254, 240, 138, 255});

    // Panel 2: THỐNG KÊ CHI TIẾT (Stats)
    Rectangle p2 = {(float)startX, 195, (float)width, 175};
    DrawRectangleRounded(p2, 0.12f, 8, (Color){26, 36, 52, 220});
    DrawRectangleRoundedLinesEx(p2, 0.12f, 8, 1.5f, (Color){51, 65, 85, 255});

    DrawText("CHỈ SỐ CHƠI", startX + 16, 207, 13, (Color){148, 163, 184, 255});

    // Độ dài thân
    DrawText("Độ dài rắn:", startX + 16, 230, 14, (Color){203, 213, 225, 255});
    char lenStr[16];
    snprintf(lenStr, sizeof(lenStr), "%d", game->snake.length);
    DrawText(lenStr, startX + width - 16 - MeasureText(lenStr, 14), 230, 14, (Color){52, 211, 153, 255});

    // Số táo đỏ
    DrawText("Táo đỏ:", startX + 16, 255, 14, (Color){203, 213, 225, 255});
    char appleStr[16];
    snprintf(appleStr, sizeof(appleStr), "%d", game->applesEaten);
    DrawText(appleStr, startX + width - 16 - MeasureText(appleStr, 14), 255, 14, (Color){248, 113, 113, 255});

    // Số táo vàng
    DrawText("Táo vàng:", startX + 16, 280, 14, (Color){203, 213, 225, 255});
    char goldStr[16];
    snprintf(goldStr, sizeof(goldStr), "%d", game->goldenEaten);
    DrawText(goldStr, startX + width - 16 - MeasureText(goldStr, 14), 280, 14, (Color){251, 191, 36, 255});

    // Combo liên hoàn
    DrawText("Combo:", startX + 16, 305, 14, (Color){203, 213, 225, 255});
    char comboStr[16];
    if (game->combo > 0) {
        snprintf(comboStr, sizeof(comboStr), "x%d", game->combo);
        DrawText(comboStr, startX + width - 16 - MeasureText(comboStr, 14), 305, 14, (Color){244, 114, 182, 255});

        // Thanh thời gian combo
        float comboRatio = game->comboTimer / 4.0f;
        if (comboRatio > 1.0f) comboRatio = 1.0f;
        Rectangle cBar = {(float)(startX + 16), 332, (float)(width - 32) * comboRatio, 6};
        DrawRectangleRounded(cBar, 0.5f, 4, (Color){244, 114, 182, 230});
    } else {
        DrawText("-", startX + width - 16 - MeasureText("-", 14), 305, 14, (Color){100, 116, 139, 255});
    }

    if (game->frostTimer > 0.0f) {
        DrawText("ĐANG ĐÓNG BĂNG!", startX + 16, 345, 12, (Color){56, 189, 248, 255});
    }

    // Panel 3: CHẾ ĐỘ & CÀI ĐẶT (Mode & Options)
    Rectangle p3 = {(float)startX, 385, (float)width, 105};
    DrawRectangleRounded(p3, 0.12f, 8, (Color){26, 36, 52, 220});
    DrawRectangleRoundedLinesEx(p3, 0.12f, 8, 1.5f, (Color){51, 65, 85, 255});

    DrawText("CHẾ ĐỘ CHƠI", startX + 16, 396, 13, (Color){148, 163, 184, 255});

    const char *modeStr = (game->mode == MODE_CLASSIC) ? "Cổ Điển (Đâm tường)" : "Xuyên Tường (Wrap)";
    Color modeCol = (game->mode == MODE_CLASSIC) ? (Color){239, 68, 68, 255} : (Color){14, 165, 233, 255};
    DrawText(modeStr, startX + 16, 417, 13, modeCol);

    const char *diffStr = "Bình thường";
    if (game->difficulty == DIFF_FAST) diffStr = "Nhanh";
    else if (game->difficulty == DIFF_INSANE) diffStr = "Điên cuồng";

    DrawText("Độ khó: ", startX + 16, 440, 13, (Color){203, 213, 225, 255});
    DrawText(diffStr, startX + 70, 440, 13, (Color){251, 191, 36, 255});

    DrawText("Âm thanh: ", startX + 16, 463, 13, (Color){203, 213, 225, 255});
    DrawText(game->soundEnabled ? "BẬT" : "TẮT", startX + 85, 463, 13, game->soundEnabled ? (Color){52, 211, 153, 255} : (Color){239, 68, 68, 255});

    // Panel 4: PHÍM ĐIỀU KHIỂN (Controls Guide)
    Rectangle p4 = {(float)startX, 505, (float)width, 175};
    DrawRectangleRounded(p4, 0.12f, 8, (Color){26, 36, 52, 220});
    DrawRectangleRoundedLinesEx(p4, 0.12f, 8, 1.5f, (Color){51, 65, 85, 255});

    DrawText("HƯỚNG DẪN PHÍM", startX + 16, 516, 13, (Color){148, 163, 184, 255});

    DrawText("Mũi tên / WASD", startX + 16, 538, 12, (Color){203, 213, 225, 255});
    DrawText("Di chuyển", startX + width - 16 - MeasureText("Di chuyển", 12), 538, 12, (Color){148, 163, 184, 255});

    DrawText("P / ESC", startX + 16, 560, 12, (Color){203, 213, 225, 255});
    DrawText("Tạm dừng", startX + width - 16 - MeasureText("Tạm dừng", 12), 560, 12, (Color){148, 163, 184, 255});

    DrawText("TAB", startX + 16, 582, 12, (Color){203, 213, 225, 255});
    DrawText("Đổi tường", startX + width - 16 - MeasureText("Đổi tường", 12), 582, 12, (Color){148, 163, 184, 255});

    DrawText("D / 1-3", startX + 16, 604, 12, (Color){203, 213, 225, 255});
    DrawText("Độ khó", startX + width - 16 - MeasureText("Độ khó", 12), 604, 12, (Color){148, 163, 184, 255});

    DrawText("M", startX + 16, 626, 12, (Color){203, 213, 225, 255});
    DrawText("Bật/Tắt tiếng", startX + width - 16 - MeasureText("Bật/Tắt tiếng", 12), 626, 12, (Color){148, 163, 184, 255});

    DrawText("R", startX + 16, 648, 12, (Color){203, 213, 225, 255});
    DrawText("Chơi lại", startX + width - 16 - MeasureText("Chơi lại", 12), 648, 12, (Color){148, 163, 184, 255});
}

// Hàm vẽ nút bấm tương tác (hỗ trợ hover và click chuột)
static bool DrawButton(Rectangle rect, const char *label, const char *hint, int fontSize, Color baseColor, Color hoverColor, Color textColor)
{
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    Color bg = hovered ? hoverColor : baseColor;

    DrawRectangleRounded(rect, 0.22f, 6, bg);
    DrawRectangleRoundedLinesEx(rect, 0.22f, 6, hovered ? 2.0f : 1.2f, hovered ? RAYWHITE : (Color){71, 85, 105, 255});

    if (hint && hint[0] != '\0') {
        DrawText(label, (int)(rect.x + 18), (int)(rect.y + (rect.height - fontSize) / 2), fontSize, textColor);
        int hintW = MeasureText(hint, fontSize - 2);
        DrawText(hint, (int)(rect.x + rect.width - 18 - hintW), (int)(rect.y + (rect.height - (fontSize - 2)) / 2), fontSize - 2, (Color){148, 163, 184, 255});
    } else {
        int labelW = MeasureText(label, fontSize);
        DrawText(label, (int)(rect.x + (rect.width - labelW) / 2), (int)(rect.y + (rect.height - fontSize) / 2), fontSize, textColor);
    }

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Menu chính khi vào game
static void DrawMenuOverlay(SnakeGame *game)
{
    Rectangle modal = {
        (float)(BOARD_OFFSET_X + 45),
        (float)(BOARD_OFFSET_Y + 70),
        (float)(BOARD_WIDTH - 90),
        500
    };

    DrawRectangleRounded(modal, 0.08f, 8, (Color){15, 23, 42, 245});
    DrawRectangleRoundedLinesEx(modal, 0.08f, 8, 2.0f, (Color){52, 211, 153, 255});

    const char *title = "RẮN SĂN MỒI";
    int titleW = MeasureText(title, 38);
    DrawText(title, (int)(modal.x + (modal.width - titleW) / 2), (int)(modal.y + 24), 38, (Color){52, 211, 153, 255});

    const char *subTitle = "- RETRO ARCADE EDITION -";
    int subW = MeasureText(subTitle, 15);
    DrawText(subTitle, (int)(modal.x + (modal.width - subW) / 2), (int)(modal.y + 68), 15, (Color){251, 191, 36, 255});

    // Nút 1: Bắt đầu chơi
    Rectangle btnStart = {modal.x + 35, modal.y + 102, modal.width - 70, 52};
    if (DrawButton(btnStart, "▶  BẮT ĐẦU CHƠI", "[ Space / Enter ]", 18, (Color){16, 185, 129, 230}, (Color){5, 150, 105, 255}, RAYWHITE)) {
        ResetSnakeGame(game);
        PlaySoundSnakeClick(game->soundEnabled);
    }

    // Nút 2: Đổi chế độ chơi
    Rectangle btnMode = {modal.x + 35, modal.y + 168, modal.width - 70, 48};
    const char *modeLabel = (game->mode == MODE_CLASSIC) ? "Chế độ: CỔ ĐIỂN (Đâm tường)" : "Chế độ: XUYÊN TƯỜNG (Wrap)";
    Color modeCol = (game->mode == MODE_CLASSIC) ? (Color){185, 28, 28, 200} : (Color){2, 132, 199, 200};
    Color modeHover = (game->mode == MODE_CLASSIC) ? (Color){220, 38, 38, 255} : (Color){14, 165, 233, 255};
    if (DrawButton(btnMode, modeLabel, "[ Phím TAB ]", 15, modeCol, modeHover, RAYWHITE)) {
        game->mode = (game->mode == MODE_CLASSIC) ? MODE_NO_WALLS : MODE_CLASSIC;
        PlaySoundSnakeClick(game->soundEnabled);
    }

    // Nút 3: Đổi độ khó
    Rectangle btnDiff = {modal.x + 35, modal.y + 230, modal.width - 70, 48};
    const char *diffLabel = "Độ khó: BÌNH THƯỜNG";
    if (game->difficulty == DIFF_FAST) diffLabel = "Độ khó: NHANH (Fast)";
    else if (game->difficulty == DIFF_INSANE) diffLabel = "Độ khó: ĐIÊN CUỒNG (Insane)";

    if (DrawButton(btnDiff, diffLabel, "[ Phím D / 1,2,3 ]", 15, (Color){30, 41, 59, 230}, (Color){51, 65, 85, 255}, (Color){251, 191, 36, 255})) {
        game->difficulty = (game->difficulty + 1) % 3;
        PlaySoundSnakeClick(game->soundEnabled);
    }

    // Nút 4: Bật tắt âm thanh
    Rectangle btnSound = {modal.x + 35, modal.y + 292, modal.width - 70, 48};
    const char *soundLabel = game->soundEnabled ? "Âm thanh: BẬT (Sound ON)" : "Âm thanh: TẮT (Muted)";
    if (DrawButton(btnSound, soundLabel, "[ Phím M ]", 15, (Color){30, 41, 59, 230}, (Color){51, 65, 85, 255}, game->soundEnabled ? (Color){52, 211, 153, 255} : (Color){248, 113, 113, 255})) {
        game->soundEnabled = !game->soundEnabled;
    }

    // Kỷ lục cao nhất
    char highStr[64];
    snprintf(highStr, sizeof(highStr), "★ KỶ LỤC HIỆN TẠI: %05d ★", game->highScore);
    int highW = MeasureText(highStr, 17);
    DrawText(highStr, (int)(modal.x + (modal.width - highW) / 2), (int)(modal.y + 365), 17, (Color){251, 191, 36, 255});

    // Mẹo hướng dẫn
    const char *tip1 = "💡 Có thể dùng chuột nhấp trực tiếp vào các nút trên!";
    int tip1W = MeasureText(tip1, 13);
    DrawText(tip1, (int)(modal.x + (modal.width - tip1W) / 2), (int)(modal.y + 410), 13, (Color){148, 163, 184, 255});

    const char *tip2 = "(Nếu gõ số bị hiện ô chữ nổi, hãy bấm phím D hoặc đổi bộ gõ sang Tiếng Anh)";
    int tip2W = MeasureText(tip2, 12);
    DrawText(tip2, (int)(modal.x + (modal.width - tip2W) / 2), (int)(modal.y + 438), 12, (Color){100, 116, 139, 255});
}

// Màn hình Tạm dừng
static void DrawPauseOverlay(SnakeGame *game)
{
    Rectangle modal = {
        (float)(BOARD_OFFSET_X + 80),
        (float)(BOARD_OFFSET_Y + 140),
        (float)(BOARD_WIDTH - 160),
        320
    };

    DrawRectangleRounded(modal, 0.08f, 8, (Color){15, 23, 42, 245});
    DrawRectangleRoundedLinesEx(modal, 0.08f, 8, 2.0f, (Color){56, 189, 248, 255});

    const char *pauseTitle = "TẠM DỪNG";
    int titleW = MeasureText(pauseTitle, 36);
    DrawText(pauseTitle, (int)(modal.x + (modal.width - titleW) / 2), (int)(modal.y + 26), 36, (Color){56, 189, 248, 255});

    // Nút Tiếp tục
    Rectangle btnResume = {modal.x + 35, modal.y + 85, modal.width - 70, 46};
    if (DrawButton(btnResume, "▶  TIẾP TỤC", "[ P / Space ]", 16, (Color){14, 165, 233, 200}, (Color){2, 132, 199, 255}, RAYWHITE)) {
        game->state = SNAKE_STATE_PLAYING;
        PlaySoundSnakePause(game->soundEnabled);
    }

    // Nút Chơi lại
    Rectangle btnRestart = {modal.x + 35, modal.y + 145, modal.width - 70, 46};
    if (DrawButton(btnRestart, "🔄  CHƠI LẠI TỪ ĐẦU", "[ Phím R ]", 16, (Color){185, 28, 28, 180}, (Color){220, 38, 38, 240}, RAYWHITE)) {
        ResetSnakeGame(game);
        PlaySoundSnakeClick(game->soundEnabled);
    }

    // Nút Âm thanh
    Rectangle btnSound = {modal.x + 35, modal.y + 205, modal.width - 70, 46};
    const char *soundLabel = game->soundEnabled ? "🔊  Âm thanh: BẬT" : "🔇  Âm thanh: TẮT";
    if (DrawButton(btnSound, soundLabel, "[ Phím M ]", 15, (Color){30, 41, 59, 220}, (Color){51, 65, 85, 255}, game->soundEnabled ? (Color){52, 211, 153, 255} : (Color){248, 113, 113, 255})) {
        game->soundEnabled = !game->soundEnabled;
    }
}

// Màn hình Game Over
static void DrawGameOverOverlay(SnakeGame *game)
{
    Rectangle modal = {
        (float)(BOARD_OFFSET_X + 60),
        (float)(BOARD_OFFSET_Y + 90),
        (float)(BOARD_WIDTH - 120),
        470
    };

    DrawRectangleRounded(modal, 0.08f, 8, (Color){15, 23, 42, 250});
    DrawRectangleRoundedLinesEx(modal, 0.08f, 8, 2.0f, (Color){239, 68, 68, 255});

    const char *goTitle = "GAME OVER";
    int titleW = MeasureText(goTitle, 40);
    DrawText(goTitle, (int)(modal.x + (modal.width - titleW) / 2), (int)(modal.y + 26), 40, (Color){239, 68, 68, 255});

    // Kiểm tra có đạt kỷ lục mới không
    if (game->score >= game->highScore && game->score > 0) {
        float pulse = 0.5f + 0.5f * sinf(game->globalTime * 8.0f);
        const char *newRecord = "★ KỶ LỤC MỚI ĐƯỢC THIẾT LẬP! ★";
        int recW = MeasureText(newRecord, 18);
        DrawText(newRecord, (int)(modal.x + (modal.width - recW) / 2), (int)(modal.y + 76), 18, Fade((Color){251, 191, 36, 255}, 0.6f + 0.4f * pulse));
    }

    // Kết quả điểm số
    char resScore[64];
    snprintf(resScore, sizeof(resScore), "Điểm số đạt được:  %d", game->score);
    int sW = MeasureText(resScore, 18);
    DrawText(resScore, (int)(modal.x + (modal.width - sW) / 2), (int)(modal.y + 115), 18, RAYWHITE);

    char resApples[64];
    snprintf(resApples, sizeof(resApples), "Số táo đã thu thập:  %d", game->applesEaten);
    int aW = MeasureText(resApples, 16);
    DrawText(resApples, (int)(modal.x + (modal.width - aW) / 2), (int)(modal.y + 150), 16, (Color){203, 213, 225, 255});

    char resLen[64];
    snprintf(resLen, sizeof(resLen), "Chiều dài tối đa của rắn:  %d đốt", game->snake.length);
    int lW = MeasureText(resLen, 16);
    DrawText(resLen, (int)(modal.x + (modal.width - lW) / 2), (int)(modal.y + 180), 16, (Color){203, 213, 225, 255});

    // Nút 1: Chơi lại
    Rectangle btnReplay = {modal.x + 40, modal.y + 235, modal.width - 80, 50};
    if (DrawButton(btnReplay, "🔄  CHƠI LẠI NGAY", "[ Space / Enter / R ]", 17, (Color){16, 185, 129, 230}, (Color){5, 150, 105, 255}, RAYWHITE)) {
        ResetSnakeGame(game);
        PlaySoundSnakeClick(game->soundEnabled);
    }

    // Nút 2: Về Menu chính
    Rectangle btnMenu = {modal.x + 40, modal.y + 300, modal.width - 80, 46};
    if (DrawButton(btnMenu, "⌂  VỀ MENU CHÍNH", "[ Phím ESC ]", 16, (Color){30, 41, 59, 230}, (Color){51, 65, 85, 255}, (Color){203, 213, 225, 255})) {
        game->state = SNAKE_STATE_MENU;
        PlaySoundSnakeClick(game->soundEnabled);
    }

    const char *tip = "💡 Dùng chuột click nút hoặc bấm phím để chọn";
    int tipW = MeasureText(tip, 13);
    DrawText(tip, (int)(modal.x + (modal.width - tipW) / 2), (int)(modal.y + 380), 13, (Color){148, 163, 184, 255});
}

void DrawSnakeGame(SnakeGame *game)
{
    // Màu nền toàn màn hình
    ClearBackground((Color){15, 23, 42, 255}); // Slate 900

    // Hiệu ứng rung camera khi có va chạm hoặc nổ
    Camera2D camera = {0};
    camera.zoom = 1.0f;
    if (game->shakeTimer > 0.0f) {
        float factor = game->shakeTimer * game->shakeMagnitude;
        camera.offset.x = ((float)GetRandomValue(-100, 100) / 100.0f) * factor;
        camera.offset.y = ((float)GetRandomValue(-100, 100) / 100.0f) * factor;
    }

    BeginMode2D(camera);

        // 1. Vẽ bàn cờ
        DrawBoardGrid(game);

        // 2. Vẽ mồi (Thường & Thưởng)
        DrawFood(&game->normalFood, game->globalTime);
        DrawFood(&game->bonusFood, game->globalTime);

        // 3. Vẽ rắn
        DrawSnake(game);

        // 4. Vẽ các hiệu ứng hạt vụn
        DrawParticles(game);

        // 5. Vẽ chữ số điểm nổi (Floating Text)
        DrawFloatingTexts(game);

        // 6. Vẽ thanh Dashboard bên phải
        DrawSidebar(game);

        // 7. Vẽ các màn hình đè (Overlay)
        if (game->state == SNAKE_STATE_MENU) {
            DrawMenuOverlay(game);
        } else if (game->state == SNAKE_STATE_PAUSED) {
            DrawPauseOverlay(game);
        } else if (game->state == SNAKE_STATE_GAME_OVER) {
            DrawGameOverOverlay(game);
        }

    EndMode2D();
}

