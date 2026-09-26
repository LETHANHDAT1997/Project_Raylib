// ============================================================================
// FIGHTER DRAW - Vẽ background, HUD và các màn hình chuyển cảnh
// ============================================================================
#include "raylib.h"
#include "raymath.h"
#include "fighter_types.h"
#include "fighter_draw.h"
#include "fighter_entities.h"
#include <math.h>
#include <stdio.h>

// ============================================================================
// BACKGROUND - Sân đấu phong cách Nhật Bản (như hình ảnh Street Fighter)
// ============================================================================
void DrawBackground(FighterGame *game)
{
    // === BẦU TRỜI ĐÊM ===
    // Gradient từ xanh đậm xuống tím
    for (int y = 0; y < GROUND_Y + 20; y++) {
        float t = (float)y / (GROUND_Y + 20);
        Color skyCol = ColorAlpha(
            (Color){
                (unsigned char)(10 + t * 30),
                (unsigned char)(15 + t * 40),
                (unsigned char)(50 + t * 60),
                255
            }, 1.0f);
        DrawRectangle(0, y, SCREEN_WIDTH, 1, skyCol);
    }

    // === MẶT TRĂNG ===
    float moonX = 800 + sinf(game->stateTimer * 0.1f) * 5;
    float moonY = 60;
    DrawCircle((int)moonX, (int)moonY, 45, (Color){220, 220, 200, 60});
    DrawCircle((int)moonX, (int)moonY, 38, (Color){230, 230, 210, 100});
    DrawCircle((int)moonX, (int)moonY, 32, (Color){240, 240, 225, 180});
    // Crater
    DrawCircle((int)(moonX - 8), (int)(moonY - 5), 6, (Color){210, 210, 200, 80});
    DrawCircle((int)(moonX + 10), (int)(moonY + 8), 4, (Color){210, 210, 200, 80});

    // === NGÔI SAO ===
    for (int i = 0; i < 30; i++) {
        int sx = (i * 137 + 53) % SCREEN_WIDTH;
        int sy = (i * 89 + 17) % (int)(GROUND_Y * 0.4f);
        float twinkle = sinf(game->stateTimer * (2.0f + i * 0.3f) + i) * 0.5f + 0.5f;
        DrawCircle(sx, sy, 1.0f + twinkle, ColorAlpha(WHITE, 0.3f + twinkle * 0.7f));
    }

    // === MÂY ===
    float cloudOff = game->cloudOffset;
    for (int i = 0; i < 4; i++) {
        float cx = fmodf(200 + i * 300 + cloudOff * (0.3f + i * 0.1f), SCREEN_WIDTH + 200) - 100;
        float cy = 40 + i * 25;
        Color cloudCol = (Color){60, 70, 100, (unsigned char)(30 + i * 10)};
        DrawEllipse((int)cx, (int)cy, 80 - i * 10, 20 - i * 3, cloudCol);
        DrawEllipse((int)(cx + 40), (int)(cy - 5), 50, 15, cloudCol);
        DrawEllipse((int)(cx - 30), (int)(cy + 3), 60, 18, cloudCol);
    }

    // === LÂU ĐÀI NHẬT BẢN (phía sau) ===
    Color castleDark = {40, 35, 50, 255};
    Color castleMid = {55, 48, 65, 255};
    Color castleLight = {70, 60, 80, 255};
    Color roofColor = {30, 25, 40, 255};

    // Tầng chính
    DrawRectangle(350, 100, 300, 180, castleDark);
    // Mái cong (tầng trên)
    for (int layer = 0; layer < 4; layer++) {
        float ly = 100 - layer * 40;
        float lw = 200 - layer * 40;
        float lx = 500 - lw / 2;
        float lh = 35;
        DrawRectangle((int)lx, (int)ly, (int)lw, (int)lh, castleMid);
        // Mái cong
        Vector2 roofPts[4];
        roofPts[0] = (Vector2){lx - 20, ly + 5};
        roofPts[1] = (Vector2){lx + lw/2, ly - 15};
        roofPts[2] = (Vector2){lx + lw + 20, ly + 5};
        DrawTriangle(roofPts[1], roofPts[0], roofPts[2], roofColor);

        // Viền mái
        DrawLineEx(roofPts[0], roofPts[1], 2.0f, castleLight);
        DrawLineEx(roofPts[1], roofPts[2], 2.0f, castleLight);
    }

    // Cửa sổ lâu đài
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 5; col++) {
            int wx = 380 + col * 55;
            int wy = 130 + row * 45;
            DrawRectangle(wx, wy, 20, 25, (Color){80, 70, 50, 200});
            DrawRectangle(wx + 2, wy + 2, 16, 21, (Color){40, 50, 30, 150});
            // Ánh sáng vàng nhạt
            float glow = sinf(game->stateTimer * 1.5f + row + col) * 0.3f + 0.3f;
            DrawRectangle(wx + 3, wy + 3, 14, 19, ColorAlpha(YELLOW, glow * 0.3f));
        }
    }

    // === CỔNG TORII nhỏ ở xa ===
    Color toriiRed = {150, 30, 20, 180};
    DrawRectangle(150, 150, 6, 80, toriiRed);
    DrawRectangle(200, 150, 6, 80, toriiRed);
    DrawRectangle(140, 148, 76, 8, toriiRed);
    DrawRectangle(145, 165, 66, 5, toriiRed);

    // === SÀN ĐẤU (sàn gỗ) ===
    // Nền sàn gỗ
    Color woodDark = {100, 65, 35, 255};
    Color woodLight = {130, 85, 50, 255};
    Color woodVarnish = {110, 75, 42, 255};

    DrawRectangle(0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_Y, woodDark);

    // Vân gỗ
    for (int i = 0; i < 20; i++) {
        int plankX = i * (SCREEN_WIDTH / 20);
        int plankW = SCREEN_WIDTH / 20;
        Color plankCol = (i % 2 == 0) ? woodLight : woodVarnish;
        DrawRectangle(plankX, GROUND_Y, plankW, SCREEN_HEIGHT - GROUND_Y, plankCol);
        // Đường kẻ giữa các tấm ván
        DrawLine(plankX, GROUND_Y, plankX, SCREEN_HEIGHT, (Color){70, 45, 25, 150});
    }

    // === ỐNG THÉP / THANH CHẮN PHÍA TRƯỚC ===
    Color pipeColor = {60, 55, 50, 255};
    Color pipeHighlight = {90, 85, 80, 255};

    // Ống ngang lớn
    DrawRectangle(0, GROUND_Y - 30, SCREEN_WIDTH, 30, pipeColor);
    DrawRectangle(0, GROUND_Y - 30, SCREEN_WIDTH, 5, pipeHighlight);
    DrawRectangle(0, GROUND_Y - 5, SCREEN_WIDTH, 3, pipeHighlight);

    // Đĩa tròn trang trí trên ống (kiểu bulông)
    for (int i = 0; i < 6; i++) {
        int bx = 50 + i * 180;
        int by = GROUND_Y - 15;
        DrawCircle(bx, by, 18, (Color){80, 75, 70, 255});
        DrawCircle(bx, by, 15, (Color){70, 65, 60, 255});
        DrawCircleLines(bx, by, 18, pipeHighlight);
        // Các đinh ốc
        for (int j = 0; j < 8; j++) {
            float angle = j * 45.0f * DEG2RAD;
            int dx = (int)(cosf(angle) * 10);
            int dy = (int)(sinf(angle) * 10);
            DrawCircle(bx + dx, by + dy, 2, (Color){90, 85, 80, 255});
        }
    }

    // === ỐNG DỌC (trụ cột hai bên) ===
    DrawRectangle(20, GROUND_Y - 200, 25, 200, pipeColor);
    DrawRectangle(20, GROUND_Y - 200, 5, 200, pipeHighlight);
    DrawRectangle(SCREEN_WIDTH - 45, GROUND_Y - 200, 25, 200, pipeColor);
    DrawRectangle(SCREEN_WIDTH - 45, GROUND_Y - 200, 5, 200, pipeHighlight);

    // Đèn lồng treo (kiểu Nhật)
    for (int side = 0; side < 2; side++) {
        int lx = (side == 0) ? 32 : SCREEN_WIDTH - 33;
        int ly = GROUND_Y - 170;
        // Dây treo
        DrawLine(lx, ly, lx, ly + 20, (Color){50, 50, 50, 200});
        // Đèn lồng
        float lanternPulse = sinf(game->stateTimer * 2.0f + side * 3.14f) * 2.0f;
        DrawEllipse(lx, ly + 30 + (int)lanternPulse, 10, 14,
                    (Color){200, 50, 30, 200});
        DrawEllipse(lx, ly + 30 + (int)lanternPulse, 8, 11,
                    (Color){230, 100, 50, 180});
        // Ánh sáng
        DrawCircle(lx, ly + 30 + (int)lanternPulse, 20,
                   ColorAlpha(ORANGE, 0.1f + sinf(game->stateTimer * 3.0f) * 0.05f));
    }
}

// ============================================================================
// HUD - Thanh máu, tên, timer, round indicators
// ============================================================================
void DrawHUD(FighterGame *game)
{
    Fighter *p1 = &game->player1;
    Fighter *p2 = &game->player2;

    int barWidth = 380;
    int barHeight = 22;
    int barY = 28;
    int p1BarX = 55;
    int p2BarX = SCREEN_WIDTH - 55 - barWidth;
    int timerX = SCREEN_WIDTH / 2;

    // === THANH MÁU P1 (từ trái sang) ===
    // Viền ngoài
    DrawRectangle(p1BarX - 3, barY - 3, barWidth + 6, barHeight + 6, (Color){50, 50, 50, 200});
    DrawRectangle(p1BarX - 2, barY - 2, barWidth + 4, barHeight + 4, (Color){30, 30, 30, 230});

    // Nền thanh máu
    DrawRectangle(p1BarX, barY, barWidth, barHeight, (Color){80, 10, 10, 255});

    // Thanh máu (màu vàng -> đỏ khi máu thấp)
    float hp1Ratio = (float)p1->health / p1->maxHealth;
    int hp1W = (int)(barWidth * hp1Ratio);
    Color hp1Col;
    if (hp1Ratio > 0.5f) hp1Col = (Color){230, 210, 50, 255};      // Vàng
    else if (hp1Ratio > 0.25f) hp1Col = (Color){230, 150, 30, 255}; // Cam
    else hp1Col = (Color){220, 50, 30, 255};                         // Đỏ

    // Gradient effect
    for (int i = 0; i < hp1W; i++) {
        float t = (float)i / barWidth;
        Color c = ColorAlpha(hp1Col, 0.8f + t * 0.2f);
        DrawRectangle(p1BarX + i, barY, 1, barHeight, c);
    }
    // Highlight trên cùng
    DrawRectangle(p1BarX, barY, hp1W, 4, ColorAlpha(WHITE, 0.3f));

    // === THANH MÁU P2 (từ phải sang) ===
    DrawRectangle(p2BarX - 3, barY - 3, barWidth + 6, barHeight + 6, (Color){50, 50, 50, 200});
    DrawRectangle(p2BarX - 2, barY - 2, barWidth + 4, barHeight + 4, (Color){30, 30, 30, 230});
    DrawRectangle(p2BarX, barY, barWidth, barHeight, (Color){80, 10, 10, 255});

    float hp2Ratio = (float)p2->health / p2->maxHealth;
    int hp2W = (int)(barWidth * hp2Ratio);
    Color hp2Col;
    if (hp2Ratio > 0.5f) hp2Col = (Color){230, 210, 50, 255};
    else if (hp2Ratio > 0.25f) hp2Col = (Color){230, 150, 30, 255};
    else hp2Col = (Color){220, 50, 30, 255};

    // P2 bar fills from right to left
    for (int i = 0; i < hp2W; i++) {
        float t = (float)i / barWidth;
        Color c = ColorAlpha(hp2Col, 0.8f + (1.0f - t) * 0.2f);
        DrawRectangle(p2BarX + barWidth - hp2W + i, barY, 1, barHeight, c);
    }
    DrawRectangle(p2BarX + barWidth - hp2W, barY, hp2W, 4, ColorAlpha(WHITE, 0.3f));

    // === THANH SUPER METER ===
    int superBarW = 200;
    int superBarH = 8;
    int superY = barY + barHeight + 8;

    // P1 Super
    DrawRectangle(p1BarX, superY, superBarW, superBarH, (Color){20, 20, 60, 200});
    int p1SuperW = (int)(superBarW * (p1->superMeter / 100.0f));
    for (int i = 0; i < p1SuperW; i++) {
        float pulse = sinf(game->stateTimer * 5.0f + i * 0.1f) * 0.2f + 0.8f;
        DrawRectangle(p1BarX + i, superY, 1, superBarH,
                      ColorAlpha((Color){50, 150, 255, 255}, pulse));
    }

    // P2 Super
    DrawRectangle(p2BarX + barWidth - superBarW, superY, superBarW, superBarH, (Color){20, 20, 60, 200});
    int p2SuperW = (int)(superBarW * (p2->superMeter / 100.0f));
    for (int i = 0; i < p2SuperW; i++) {
        float pulse = sinf(game->stateTimer * 5.0f + i * 0.1f) * 0.2f + 0.8f;
        DrawRectangle(p2BarX + barWidth - p2SuperW + i, superY, 1, superBarH,
                      ColorAlpha((Color){50, 150, 255, 255}, pulse));
    }

    // === KO TEXT ===
    int koX = timerX;
    int koY = barY - 5;
    DrawText("K.O", koX - MeasureText("K.O", 20) / 2, koY, 20, (Color){200, 50, 50, 255});

    // === TIMER ===
    char timerStr[8];
    snprintf(timerStr, sizeof(timerStr), "%d", (int)game->roundTimer);
    int timerFontSize = 48;
    int tw = MeasureText(timerStr, timerFontSize);

    // Timer background
    DrawRectangle(timerX - 35, barY + 15, 70, 55, (Color){20, 20, 20, 200});
    DrawRectangleLines(timerX - 35, barY + 15, 70, 55, (Color){150, 130, 50, 200});

    // Nhấp nháy khi còn ít thời gian
    Color timerCol = WHITE;
    if (game->roundTimer <= 10) {
        float blink = sinf(game->stateTimer * 8.0f);
        timerCol = (blink > 0) ? RED : WHITE;
    }
    DrawText(timerStr, timerX - tw / 2, barY + 22, timerFontSize, timerCol);

    // === TÊN NHÂN VẬT ===
    const char *p1Name = (p1->character == CHAR_RYU) ? "Ryu" : "Ken";
    const char *p2Name = (p2->character == CHAR_RYU) ? "Ryu" : "Ken";

    // Avatar frame P1
    DrawRectangle(12, barY - 5, 40, 40, (Color){40, 40, 40, 220});
    DrawRectangleLines(12, barY - 5, 40, 40, (Color){200, 50, 50, 255});
    if (p1->character == CHAR_RYU) {
        DrawCircle(32, barY + 10, 12, (Color){230, 190, 150, 255});
        DrawCircle(32, barY + 3, 10, (Color){60, 40, 20, 255});
        DrawRectangle(18, barY + 1, 28, 5, RED); // Bandana
    } else {
        DrawCircle(32, barY + 10, 12, (Color){235, 195, 155, 255});
        DrawEllipse(32, barY + 2, 12, 10, (Color){230, 200, 50, 255});
    }
    DrawText(p1Name, p1BarX, barY + barHeight + 2, 16, WHITE);

    // Avatar frame P2
    int p2AvatarX = SCREEN_WIDTH - 52;
    DrawRectangle(p2AvatarX, barY - 5, 40, 40, (Color){40, 40, 40, 220});
    DrawRectangleLines(p2AvatarX, barY - 5, 40, 40, (Color){50, 50, 200, 255});
    if (p2->character == CHAR_RYU) {
        DrawCircle(p2AvatarX + 20, barY + 10, 12, (Color){230, 190, 150, 255});
        DrawCircle(p2AvatarX + 20, barY + 3, 10, (Color){60, 40, 20, 255});
        DrawRectangle(p2AvatarX + 6, barY + 1, 28, 5, RED);
    } else {
        DrawCircle(p2AvatarX + 20, barY + 10, 12, (Color){235, 195, 155, 255});
        DrawEllipse(p2AvatarX + 20, barY + 2, 12, 10, (Color){230, 200, 50, 255});
    }
    int p2NameW = MeasureText(p2Name, 16);
    DrawText(p2Name, p2BarX + barWidth - p2NameW, barY + barHeight + 2, 16, WHITE);

    // === ROUND INDICATORS (chấm tròn thắng) ===
    for (int i = 0; i < WINS_NEEDED; i++) {
        Color dotCol1 = (i < p1->roundsWon) ? (Color){255, 200, 50, 255} : (Color){60, 60, 60, 200};
        Color dotCol2 = (i < p2->roundsWon) ? (Color){255, 200, 50, 255} : (Color){60, 60, 60, 200};
        DrawCircle(p1BarX + 10 + i * 20, superY + 18, 6, dotCol1);
        DrawCircle(p2BarX + barWidth - 10 - i * 20, superY + 18, 6, dotCol2);
        if (i < p1->roundsWon) DrawCircle(p1BarX + 10 + i * 20, superY + 18, 3, WHITE);
        if (i < p2->roundsWon) DrawCircle(p2BarX + barWidth - 10 - i * 20, superY + 18, 3, WHITE);
    }

    // === COMBO COUNTER ===
    if (p1->comboCount > 1) {
        char comboStr[32];
        snprintf(comboStr, sizeof(comboStr), "%d HIT COMBO!", p1->comboCount);
        float comboScale = 1.0f + sinf(game->stateTimer * 10.0f) * 0.1f;
        int comboSize = (int)(24 * comboScale);
        DrawText(comboStr, 80, SCREEN_HEIGHT - 80, comboSize, YELLOW);
    }
    if (p2->comboCount > 1) {
        char comboStr[32];
        snprintf(comboStr, sizeof(comboStr), "%d HIT COMBO!", p2->comboCount);
        float comboScale = 1.0f + sinf(game->stateTimer * 10.0f) * 0.1f;
        int comboSize = (int)(24 * comboScale);
        int cw = MeasureText(comboStr, comboSize);
        DrawText(comboStr, SCREEN_WIDTH - 80 - cw, SCREEN_HEIGHT - 80, comboSize, YELLOW);
    }
}

// ============================================================================
// TITLE SCREEN
// ============================================================================
void DrawTitleScreen(FighterGame *game)
{
    // Background
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float t = (float)y / SCREEN_HEIGHT;
        Color c = {
            (unsigned char)(10 + t * 20),
            (unsigned char)(5 + t * 15),
            (unsigned char)(30 + t * 40),
            255
        };
        DrawRectangle(0, y, SCREEN_WIDTH, 1, c);
    }

    // Title
    float bounce = sinf(game->stateTimer * 3.0f) * 8.0f;
    const char *title = "STREET FIGHTER";
    int titleSize = 64;
    int tw = MeasureText(title, titleSize);

    // Shadow
    DrawText(title, SCREEN_WIDTH/2 - tw/2 + 3, (int)(150 + bounce + 3), titleSize, (Color){0, 0, 0, 150});
    // Glow
    DrawText(title, SCREEN_WIDTH/2 - tw/2, (int)(150 + bounce), titleSize, (Color){255, 200, 50, 255});

    // Subtitle
    const char *subtitle = "RAYLIB EDITION";
    int stw = MeasureText(subtitle, 30);
    DrawText(subtitle, SCREEN_WIDTH/2 - stw/2, 220, 30, (Color){200, 100, 50, 255});

    // VS display
    const char *vs = "RYU  vs  KEN";
    int vsw = MeasureText(vs, 36);
    float vsPulse = sinf(game->stateTimer * 2.0f) * 0.3f + 0.7f;
    DrawText(vs, SCREEN_WIDTH/2 - vsw/2, 300, 36, ColorAlpha(WHITE, vsPulse));

    // Controls info
    int infoY = 380;
    DrawText("--- PLAYER 1 (Ryu) ---", 100, infoY, 18, (Color){100, 180, 255, 255});
    DrawText("A/D: Di chuyen  |  W: Nhay  |  S: Ngoi", 100, infoY + 25, 16, (Color){180, 180, 180, 255});
    DrawText("J: Dam  |  K: Da  |  L: Hadouken", 100, infoY + 48, 16, (Color){180, 180, 180, 255});
    DrawText("Space: Block", 100, infoY + 71, 16, (Color){180, 180, 180, 255});

    DrawText("--- PLAYER 2 (Ken) ---", 580, infoY, 18, (Color){255, 100, 100, 255});
    DrawText("Arrow Keys: Di chuyen", 580, infoY + 25, 16, (Color){180, 180, 180, 255});
    DrawText("Num1: Dam  |  Num2: Da  |  Num3: Hadouken", 580, infoY + 48, 16, (Color){180, 180, 180, 255});
    DrawText("Num0: Block", 580, infoY + 71, 16, (Color){180, 180, 180, 255});

    // Press START
    float blink = sinf(game->stateTimer * 4.0f);
    if (blink > 0) {
        const char *startText = "PRESS ENTER TO START";
        int stWidth = MeasureText(startText, 28);
        DrawText(startText, SCREEN_WIDTH/2 - stWidth/2, 530, 28, (Color){255, 255, 100, 255});
    }

    // Tab for AI
    const char *aiText = "Press TAB: Toggle AI for Player 2";
    int aiW = MeasureText(aiText, 16);
    DrawText(aiText, SCREEN_WIDTH/2 - aiW/2, 560, 16,
             game->p2IsAI ? GREEN : (Color){150, 150, 150, 200});
    if (game->p2IsAI) {
        DrawText("[AI: ON]", SCREEN_WIDTH/2 + aiW/2 + 10, 560, 16, GREEN);
    }
}

// ============================================================================
// INTRO SCREEN ("Round X - FIGHT!")
// ============================================================================
void DrawIntroScreen(FighterGame *game)
{
    DrawBackground(game);
    DrawFighter(&game->player1, 0, 0);
    DrawFighter(&game->player2, 0, 0);
    DrawHUD(game);

    // Overlay
    float alpha = 0;
    if (game->stateTimer < 0.5f) {
        alpha = 1.0f - game->stateTimer / 0.5f;
    }
    if (alpha > 0) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, alpha));
    }

    // Round text
    if (game->stateTimer < 1.5f) {
        char roundStr[32];
        snprintf(roundStr, sizeof(roundStr), "ROUND %d", game->currentRound);
        int rw = MeasureText(roundStr, 60);
        float scale = 1.0f;
        if (game->stateTimer < 0.3f) scale = game->stateTimer / 0.3f * 1.2f;
        else if (game->stateTimer < 0.5f) scale = 1.2f - (game->stateTimer - 0.3f) / 0.2f * 0.2f;
        int size = (int)(60 * scale);
        rw = MeasureText(roundStr, size);
        DrawText(roundStr, SCREEN_WIDTH/2 - rw/2 + 3, SCREEN_HEIGHT/2 - 60 + 3, size, (Color){0, 0, 0, 200});
        DrawText(roundStr, SCREEN_WIDTH/2 - rw/2, SCREEN_HEIGHT/2 - 60, size, (Color){255, 220, 50, 255});
    }

    // FIGHT!
    if (game->stateTimer > 1.2f && game->stateTimer < 2.5f) {
        float ft = game->stateTimer - 1.2f;
        float fscale = 1.0f;
        if (ft < 0.2f) fscale = ft / 0.2f * 1.5f;
        else if (ft < 0.4f) fscale = 1.5f - (ft - 0.2f) / 0.2f * 0.5f;
        int fsize = (int)(72 * fscale);
        const char *fightText = "FIGHT!";
        int fw = MeasureText(fightText, fsize);
        DrawText(fightText, SCREEN_WIDTH/2 - fw/2 + 3, SCREEN_HEIGHT/2 + 10 + 3, fsize, (Color){0, 0, 0, 200});
        DrawText(fightText, SCREEN_WIDTH/2 - fw/2, SCREEN_HEIGHT/2 + 10, fsize, (Color){255, 50, 50, 255});
    }
}

// ============================================================================
// ROUND END SCREEN
// ============================================================================
void DrawRoundEndScreen(FighterGame *game)
{
    DrawBackground(game);
    FighterDrawParticles(game);
    FighterDrawProjectiles(game);
    DrawFighter(&game->player1, game->shakeOffset.x, game->shakeOffset.y);
    DrawFighter(&game->player2, game->shakeOffset.x, game->shakeOffset.y);
    DrawHUD(game);

    // Slow motion overlay
    if (game->stateTimer < 1.5f) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.2f));
    }

    // Winner text
    if (game->stateTimer > 1.0f) {
        const char *winnerName;
        if (game->player1.health <= 0) {
            winnerName = (game->player2.character == CHAR_RYU) ? "RYU" : "KEN";
        } else if (game->player2.health <= 0) {
            winnerName = (game->player1.character == CHAR_RYU) ? "RYU" : "KEN";
        } else {
            winnerName = (game->player1.health >= game->player2.health) ?
                ((game->player1.character == CHAR_RYU) ? "RYU" : "KEN") :
                ((game->player2.character == CHAR_RYU) ? "RYU" : "KEN");
        }

        char winStr[64];
        snprintf(winStr, sizeof(winStr), "%s WINS!", winnerName);
        int ww = MeasureText(winStr, 48);
        DrawText(winStr, SCREEN_WIDTH/2 - ww/2 + 3, SCREEN_HEIGHT/2 - 30 + 3, 48, (Color){0, 0, 0, 200});
        DrawText(winStr, SCREEN_WIDTH/2 - ww/2, SCREEN_HEIGHT/2 - 30, 48, (Color){255, 220, 50, 255});
    }
}

// ============================================================================
// MATCH END SCREEN
// ============================================================================
void DrawMatchEndScreen(FighterGame *game)
{
    DrawBackground(game);
    DrawFighter(&game->player1, 0, 0);
    DrawFighter(&game->player2, 0, 0);

    // Dark overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.5f));

    // Winner
    const char *winnerName;
    if (game->player1.roundsWon >= WINS_NEEDED) {
        winnerName = (game->player1.character == CHAR_RYU) ? "RYU" : "KEN";
    } else {
        winnerName = (game->player2.character == CHAR_RYU) ? "RYU" : "KEN";
    }

    // Big winner text
    float pulse = sinf(game->stateTimer * 3.0f) * 5.0f;
    char matchStr[64];
    snprintf(matchStr, sizeof(matchStr), "%s WINS THE MATCH!", winnerName);
    int mw = MeasureText(matchStr, 52);
    DrawText(matchStr, SCREEN_WIDTH/2 - mw/2 + 3, (int)(200 + pulse + 3), 52, (Color){0, 0, 0, 200});
    DrawText(matchStr, SCREEN_WIDTH/2 - mw/2, (int)(200 + pulse), 52, GOLD);

    // Score
    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "%d - %d", game->player1.roundsWon, game->player2.roundsWon);
    int sw = MeasureText(scoreStr, 40);
    DrawText(scoreStr, SCREEN_WIDTH/2 - sw/2, 270, 40, WHITE);

    // Restart prompt
    float blink = sinf(game->stateTimer * 4.0f);
    if (blink > 0) {
        const char *restartText = "PRESS ENTER TO PLAY AGAIN";
        int rw = MeasureText(restartText, 24);
        DrawText(restartText, SCREEN_WIDTH/2 - rw/2, 450, 24, (Color){255, 255, 100, 255});
    }
}
