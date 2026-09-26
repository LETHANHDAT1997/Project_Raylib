#include "boot_menu.h"
#include "font_vn.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

#define CARD_START_X 50.0f
#define CARD_START_Y 150.0f
#define CARD_WIDTH   280.0f
#define CARD_HEIGHT  475.0f
#define CARD_GAP     20.0f

void InitBootMenu(BootApp *app)
{
    app->selectedGameIndex = 0;
    app->fadeAlpha = 0.0f;
    app->isTransitioning = false;
}

void UpdateBootMenu(BootApp *app, float dt)
{
    (void)dt;

    // Phím chuyển chọn game
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        app->selectedGameIndex--;
        if (app->selectedGameIndex < 0) app->selectedGameIndex = 3;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        app->selectedGameIndex++;
        if (app->selectedGameIndex > 3) app->selectedGameIndex = 0;
    }

    // Nhấn phím số 1, 2, 3 để chọn trực tiếp
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        app->selectedGameIndex = 0;
    } else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        app->selectedGameIndex = 1;
    } else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
        app->selectedGameIndex = 2;
    } else if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) {
        app->selectedGameIndex = 3;
    }

    // Nhấn Enter hoặc Space để vào game
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (app->selectedGameIndex == 0) app->targetScreen = BOOT_SCREEN_TETRIS;
        else if (app->selectedGameIndex == 1) app->targetScreen = BOOT_SCREEN_SPACE_INVADER;
        else if (app->selectedGameIndex == 2) app->targetScreen = BOOT_SCREEN_SNAKE;
        else if (app->selectedGameIndex == 3) app->targetScreen = BOOT_SCREEN_FIGHTER;
        app->isTransitioning = true;
    }

    // Tương tác chuột trên 4 thẻ game
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < 4; i++) {
        Rectangle cardRec = {CARD_START_X + i * (CARD_WIDTH + CARD_GAP), CARD_START_Y, CARD_WIDTH, CARD_HEIGHT};
        // Mở rộng hitbox nhẹ lên trên nếu thẻ đang ở trạng thái nâng lên
        if (app->selectedGameIndex == i) {
            cardRec.y -= 8.0f;
            cardRec.height += 8.0f;
        }

        if (CheckCollisionPointRec(mouse, cardRec)) {
            app->selectedGameIndex = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (i == 0) app->targetScreen = BOOT_SCREEN_TETRIS;
                else if (i == 1) app->targetScreen = BOOT_SCREEN_SPACE_INVADER;
                else if (i == 2) app->targetScreen = BOOT_SCREEN_SNAKE;
                else if (i == 3) app->targetScreen = BOOT_SCREEN_FIGHTER;
                app->isTransitioning = true;
            }
        }
    }
}

// Vẽ icon minh họa mini cho Tetris
static void DrawMiniTetrisIcon(Vector2 center)
{
    float sz = 18.0f;
    // Khối T mảnh ghép
    DrawRectangleRec((Rectangle){center.x - sz * 1.5f, center.y, sz - 2, sz - 2}, (Color){168, 85, 247, 255});
    DrawRectangleRec((Rectangle){center.x - sz * 0.5f, center.y, sz - 2, sz - 2}, (Color){168, 85, 247, 255});
    DrawRectangleRec((Rectangle){center.x + sz * 0.5f, center.y, sz - 2, sz - 2}, (Color){168, 85, 247, 255});
    DrawRectangleRec((Rectangle){center.x - sz * 0.5f, center.y - sz, sz - 2, sz - 2}, (Color){168, 85, 247, 255});

    // Khối vuông O bên cạnh
    DrawRectangleRec((Rectangle){center.x + sz * 1.6f, center.y - sz * 0.8f, sz - 2, sz - 2}, (Color){250, 204, 21, 255});
    DrawRectangleRec((Rectangle){center.x + sz * 2.6f, center.y - sz * 0.8f, sz - 2, sz - 2}, (Color){250, 204, 21, 255});
    DrawRectangleRec((Rectangle){center.x + sz * 1.6f, center.y + sz * 0.2f, sz - 2, sz - 2}, (Color){250, 204, 21, 255});
    DrawRectangleRec((Rectangle){center.x + sz * 2.6f, center.y + sz * 0.2f, sz - 2, sz - 2}, (Color){250, 204, 21, 255});
}

// Vẽ icon minh họa mini cho Space Invader
static void DrawMiniSpaceIcon(Vector2 center)
{
    float pSize = 5.0f;
    // Pixel bitmap của alien 11x8
    const char *alienMap[8] = {
        "  x     x  ",
        "   x   x   ",
        "  xxxxxxx  ",
        " xx xxx xx ",
        "xxxxxxxxxxx",
        "x xxxxxxx x",
        "x x     x x",
        "   xx xx   "
    };

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 11; c++) {
            if (alienMap[r][c] == 'x') {
                DrawRectangle((int)(center.x - 5.5f * pSize + c * pSize),
                              (int)(center.y - 4.0f * pSize + r * pSize),
                              (int)(pSize - 1), (int)(pSize - 1), (Color){34, 197, 94, 255});
            }
        }
    }
}

// Vẽ icon minh họa mini cho Snake
static void DrawMiniSnakeIcon(Vector2 center)
{
    float sz = 16.0f;
    // Thân rắn
    DrawRectangleRounded((Rectangle){center.x - sz * 2.0f, center.y, sz - 2, sz - 2}, 0.3f, 4, (Color){16, 185, 129, 255});
    DrawRectangleRounded((Rectangle){center.x - sz * 1.0f, center.y, sz - 2, sz - 2}, 0.3f, 4, (Color){34, 197, 94, 255});
    DrawRectangleRounded((Rectangle){center.x, center.y, sz - 2, sz - 2}, 0.3f, 4, (Color){74, 222, 128, 255});

    // Mắt
    DrawCircle((int)(center.x + sz * 0.4f), (int)(center.y + sz * 0.25f), 2.5f, RAYWHITE);
    DrawCircle((int)(center.x + sz * 0.4f), (int)(center.y + sz * 0.65f), 2.5f, RAYWHITE);

    // Táo đỏ
    DrawCircle((int)(center.x + sz * 2.0f), (int)(center.y + sz * 0.45f), 7.0f, (Color){239, 68, 68, 255});
    DrawCircle((int)(center.x + sz * 2.0f + 2), (int)(center.y + sz * 0.45f - 6), 2.0f, (Color){34, 197, 94, 255});
}

// Vẽ icon minh họa mini cho Fighter
static void DrawMiniFighterIcon(Vector2 center)
{
    // Nhân vật 1 (Ryu - gi trắng)
    DrawRectangle((int)(center.x - 30), (int)(center.y - 5), 16, 25, (Color){240, 235, 220, 255});
    DrawCircle((int)(center.x - 22), (int)(center.y - 12), 8, (Color){230, 190, 150, 255});
    DrawCircle((int)(center.x - 22), (int)(center.y - 18), 6, (Color){60, 40, 20, 255});
    DrawRectangle((int)(center.x - 30), (int)(center.y - 15), 16, 4, RED); // Bandana
    // Nắm đấm
    DrawRectangle((int)(center.x - 14), (int)(center.y - 2), 12, 6, (Color){230, 190, 150, 255});

    // Hadouken giữa 2 người
    DrawCircle((int)(center.x), (int)(center.y + 2), 8, (Color){100, 180, 255, 150});
    DrawCircle((int)(center.x), (int)(center.y + 2), 5, (Color){150, 210, 255, 200});
    DrawCircle((int)(center.x), (int)(center.y + 2), 3, WHITE);

    // Nhân vật 2 (Ken - gi đỏ)
    DrawRectangle((int)(center.x + 14), (int)(center.y - 5), 16, 25, (Color){200, 50, 40, 255});
    DrawCircle((int)(center.x + 22), (int)(center.y - 12), 8, (Color){235, 195, 155, 255});
    DrawEllipse((int)(center.x + 22), (int)(center.y - 18), 8, 6, (Color){230, 200, 50, 255});
}

void DrawBootMenu(const BootApp *app)
{
    ClearBackground((Color){11, 15, 25, 255}); // Midnight Dark Blue

    // 1. Nền hoa văn lưới điện tử nhẹ (Cyber grid)
    for (int x = 0; x < HUB_VIRTUAL_WIDTH; x += 40) {
        DrawLine(x, 0, x, HUB_VIRTUAL_HEIGHT, (Color){20, 28, 45, 100});
    }
    for (int y = 0; y < HUB_VIRTUAL_HEIGHT; y += 40) {
        DrawLine(0, y, HUB_VIRTUAL_WIDTH, y, (Color){20, 28, 45, 100});
    }

    // 2. Header Tiêu đề chính
    const char *mainTitle = "RAYLIB ARCADE MASTER HUB";
    int titleW = MeasureTextVNBold(mainTitle, 36);
    DrawTextVNBold(mainTitle, (HUB_VIRTUAL_WIDTH - titleW) / 2, 45, 36, (Color){56, 189, 248, 255});

    const char *subTitle = "- BỘ SƯU TẬP TRÒ CHƠI CỔ ĐIỂN HỖ TRỢ VIRTUAL CANVAS ĐA ĐỘ PHÂN GIẢI -";
    int subW = MeasureTextVN(subTitle, 15);
    DrawTextVN(subTitle, (HUB_VIRTUAL_WIDTH - subW) / 2, 95, 15, (Color){148, 163, 184, 255});

    // 3. 4 Thẻ bài Game Cards (đã căn giữa màn hình)

    struct {
        const char *name;
        const char *sub;
        const char *res;
        const char *features[4];
        Color theme;
    } games[4] = {
        {
            "TETRIS",
            "Xếp Gạch Cổ Điển Hiện Đại",
            "Độ phân giải gốc: 780 x 740",
            {"Thuật toán 7-Bag tiêu chuẩn", "Bóng Ghost piece & Hold piece", "Hiệu ứng ăn 4 dòng Tetris", "Âm thanh 8-bit Procedural"},
            (Color){6, 182, 212, 255}
        },
        {
            "SPACE INVADER",
            "Bắn Ruồi Không Gian Arcade",
            "Độ phân giải gốc: 800 x 880",
            {"55 Quái vật 3 chủng loại", "4 Boongke phòng thủ phá hủy", "Đĩa bay bí ẩn UFO đỏ", "Nhịp hành quân 4 nốt dồn dập"},
            (Color){34, 197, 94, 255}
        },
        {
            "SNAKE",
            "Rắn Săn Mồi Retro Edition",
            "Độ phân giải gốc: 960 x 720",
            {"Bộ đệm phím chống chết ngược", "Táo vàng thưởng & Quả băng", "2 chế độ: Cổ Điển & Xuyên Tường", "Chuột & Phím chọn độ khó"},
            (Color){16, 185, 129, 255}
        },
        {
            "FIGHTER",
            "Game Đối Kháng Street Fighter",
            "Độ phân giải gốc: 1024 x 600",
            {"Ryu vs Ken - 2 nhân vật", "Hadouken, Combo & Super Meter", "AI đối thủ thông minh", "Best of 3 Rounds - K.O System"},
            (Color){239, 68, 68, 255}
        }
    };

    for (int i = 0; i < 4; i++) {
        Rectangle cardRec = {CARD_START_X + i * (CARD_WIDTH + CARD_GAP), CARD_START_Y, CARD_WIDTH, CARD_HEIGHT};
        bool isSelected = (app->selectedGameIndex == i);

        // Hiệu ứng nâng thẻ khi chọn
        if (isSelected) {
            cardRec.y -= 8.0f;
            cardRec.height += 8.0f;
        }

        // Nền thẻ
        Color cardBg = isSelected ? (Color){24, 34, 53, 250} : (Color){18, 24, 38, 230};
        DrawRectangleRounded(cardRec, 0.06f, 8, cardBg);

        // Viền thẻ
        Color borderCol = isSelected ? games[i].theme : (Color){45, 58, 80, 255};
        float borderThick = isSelected ? 2.5f : 1.2f;
        DrawRectangleRoundedLinesEx(cardRec, 0.06f, 8, borderThick, borderCol);

        // Hào quang sáng ngoài viền khi chọn
        if (isSelected) {
            Rectangle glowRec = {cardRec.x - 3, cardRec.y - 3, cardRec.width + 6, cardRec.height + 6};
            float pulse = 0.5f + 0.5f * sinf(app->globalTime * 6.0f);
            DrawRectangleRoundedLinesEx(glowRec, 0.06f, 8, 1.5f, Fade(games[i].theme, 0.3f + 0.3f * pulse));
        }

        // Phần Header thẻ: Tên Game
        int nameW = MeasureTextVNBold(games[i].name, 26);
        DrawTextVNBold(games[i].name, (int)(cardRec.x + (cardRec.width - nameW) / 2), (int)(cardRec.y + 22), 26, isSelected ? games[i].theme : RAYWHITE);

        int subW = MeasureTextVN(games[i].sub, 13);
        DrawTextVN(games[i].sub, (int)(cardRec.x + (cardRec.width - subW) / 2), (int)(cardRec.y + 55), 13, (Color){148, 163, 184, 255});

        // Khung Icon minh họa
        Rectangle iconBox = {cardRec.x + 25, cardRec.y + 82, cardRec.width - 50, 85};
        DrawRectangleRounded(iconBox, 0.12f, 6, (Color){12, 16, 26, 255});
        DrawRectangleRoundedLinesEx(iconBox, 0.12f, 6, 1.0f, (Color){38, 48, 68, 255});

        Vector2 iconCenter = {iconBox.x + iconBox.width / 2, iconBox.y + iconBox.height / 2};
        if (i == 0) DrawMiniTetrisIcon(iconCenter);
        else if (i == 1) DrawMiniSpaceIcon(iconCenter);
        else if (i == 2) DrawMiniSnakeIcon(iconCenter);
        else if (i == 3) DrawMiniFighterIcon(iconCenter);

        // Độ phân giải gốc
        DrawTextVN(games[i].res, (int)(cardRec.x + 25), (int)(cardRec.y + 180), 12, (Color){100, 116, 139, 255});
        DrawLine((int)(cardRec.x + 25), (int)(cardRec.y + 200), (int)(cardRec.x + cardRec.width - 25), (int)(cardRec.y + 200), (Color){35, 45, 65, 255});

        // Danh sách tính năng
        DrawTextVNBold("TÍNH NĂNG NỔI BẬT:", (int)(cardRec.x + 25), (int)(cardRec.y + 212), 12, games[i].theme);
        for (int f = 0; f < 4; f++) {
            DrawTextVN("•", (int)(cardRec.x + 25), (int)(cardRec.y + 235 + f * 26), 14, games[i].theme);
            DrawTextVN(games[i].features[f], (int)(cardRec.x + 38), (int)(cardRec.y + 235 + f * 26), 13, (Color){203, 213, 225, 255});
        }

        // Nút CHƠI NGAY
        Rectangle btnPlay = {cardRec.x + 25, cardRec.y + cardRec.height - 65, cardRec.width - 50, 44};
        Color btnCol = isSelected ? games[i].theme : (Color){30, 41, 59, 255};
        Color textCol = isSelected ? (Color){15, 23, 42, 255} : (Color){203, 213, 225, 255};

        DrawRectangleRounded(btnPlay, 0.25f, 6, btnCol);
        DrawRectangleRoundedLinesEx(btnPlay, 0.25f, 6, 1.2f, isSelected ? RAYWHITE : (Color){51, 65, 85, 255});

        const char *playLabel = isSelected ? "▶  CHƠI NGAY (ENTER)" : "CHỌN GAME";
        int playW = MeasureTextVNBold(playLabel, 15);
        DrawTextVNBold(playLabel, (int)(btnPlay.x + (btnPlay.width - playW) / 2), (int)(btnPlay.y + 14), 15, textCol);
    }

    // 4. Footer hướng dẫn phím tắt toàn cục
    Rectangle footer = {40, (float)(HUB_VIRTUAL_HEIGHT - 65), (float)(HUB_VIRTUAL_WIDTH - 80), 45};
    DrawRectangleRounded(footer, 0.3f, 6, (Color){18, 25, 39, 220});
    DrawRectangleRoundedLinesEx(footer, 0.3f, 6, 1.0f, (Color){45, 58, 80, 255});

    const char *hints = "[ ← / → / A / D ] Chọn Game   |   [ ENTER / SPACE / Click ] Vào Chơi   |   [ F11 ] Toàn Màn Hình   |   [ F1 ] Về Hub";
    int hintW = MeasureTextVN(hints, 14);
    DrawTextVN(hints, (int)(footer.x + (footer.width - hintW) / 2), (int)(footer.y + 15), 14, (Color){148, 163, 184, 255});
}
