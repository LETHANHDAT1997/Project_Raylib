#include "hub_art.h"
#include "ui_theme.h"
#include "flappy_assets.h"
#include "caro_assets.h"
#include "chess_assets.h"
#include <math.h>

// Artwork vẽ bằng primitive nên không cần file ảnh và luôn sắc nét ở mọi độ
// phân giải (riêng Flappy Plane, Cờ Caro và Cờ Vua dùng thẳng tài nguyên của
// game, xem cuối file).
// Mỗi game có hai biến thể: icon vuông nhỏ và tranh hero.

// Toạ độ tương đối 0..1 trong khung art, giúp cùng một hình dùng lại được
// ở cả icon lẫn hero mà không phải tính lại tỉ lệ.
static Vector2 At(Rectangle a, float x, float y)
{
    return (Vector2){a.x + a.width * x, a.y + a.height * y};
}

static float Unit(Rectangle a)
{
    return fminf(a.width, a.height);
}

static void FillBackdrop(Rectangle a, Color top, Color bottom)
{
    DrawRectangleGradientV((int)a.x, (int)a.y, (int)a.width, (int)a.height, top, bottom);
}

// Vệt sáng toả ra từ một điểm, dùng làm nguồn sáng chính của tranh hero.
static void Bloom(Vector2 center, float radius, Color color)
{
    BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(center, radius, UiAlpha(color, 0.55f), UiAlpha(color, 0.0f));
    EndBlendMode();
}

static void StarField(Rectangle a, float time, int count, Color color)
{
    for (int i = 0; i < count; i++) {
        // Phân bố giả ngẫu nhiên nhưng ổn định giữa các khung hình.
        float fx = fmodf(sinf((float)i * 12.9898f) * 43758.5453f, 1.0f);
        float fy = fmodf(sinf((float)i * 78.233f) * 12345.6789f, 1.0f);
        if (fx < 0.0f) fx += 1.0f;
        if (fy < 0.0f) fy += 1.0f;

        float twinkle = 0.35f + 0.65f * (0.5f + 0.5f * sinf(time * 2.0f + (float)i));
        float r = (0.6f + fx * 1.4f) * (Unit(a) / 400.0f) * 1.6f;
        DrawCircleV(At(a, fx, fy), fmaxf(r, 0.7f), UiAlpha(color, twinkle));
    }
}

// --------------------------------------------------------------- TETRIS

// Bảng màu 7 khối tetromino chuẩn, chỉnh sang tông pastel cho hợp theme kính.
static const Color TETRO_COLORS[7] = {
    {126, 214, 255, 255},  // I
    {255, 214, 126, 255},  // O
    {198, 150, 255, 255},  // T
    {150, 235, 178, 255},  // S
    {255, 150, 160, 255},  // Z
    {130, 168, 255, 255},  // J
    {255, 184, 128, 255}   // L
};

static void TetroCell(Rectangle a, float gx, float gy, float cell, Color c)
{
    Rectangle r = {a.x + gx * cell, a.y + gy * cell, cell - cell * 0.12f, cell - cell * 0.12f};
    DrawRectangleRounded(r, 0.22f, 6, c);
    // Mặt vát sáng phía trên tạo cảm giác khối có độ dày.
    DrawRectangleRounded((Rectangle){r.x + r.width * 0.12f, r.y + r.height * 0.10f,
                                     r.width * 0.76f, r.height * 0.30f},
                         0.5f, 6, UiAlpha(WHITE, 0.28f));
}

void HubArtTetrisIcon(Rectangle area, float time)
{
    (void)time;
    float cell = Unit(area) / 4.4f;
    Rectangle o = {area.x + (area.width - cell * 4.0f) * 0.5f,
                   area.y + (area.height - cell * 4.0f) * 0.5f, area.width, area.height};

    // Khối T
    TetroCell(o, 0, 1, cell, TETRO_COLORS[2]);
    TetroCell(o, 1, 1, cell, TETRO_COLORS[2]);
    TetroCell(o, 2, 1, cell, TETRO_COLORS[2]);
    TetroCell(o, 1, 0, cell, TETRO_COLORS[2]);
    // Khối O
    TetroCell(o, 2, 2, cell, TETRO_COLORS[1]);
    TetroCell(o, 3, 2, cell, TETRO_COLORS[1]);
    TetroCell(o, 2, 3, cell, TETRO_COLORS[1]);
    TetroCell(o, 3, 3, cell, TETRO_COLORS[1]);
    // Khối I
    TetroCell(o, 0, 3, cell, TETRO_COLORS[0]);
    TetroCell(o, 1, 3, cell, TETRO_COLORS[0]);
}

void HubArtTetrisHero(Rectangle area, float time)
{
    FillBackdrop(area, (Color){22, 40, 86, 255}, (Color){10, 18, 44, 255});
    Bloom(At(area, 0.72f, 0.30f), Unit(area) * 1.0f, (Color){90, 150, 255, 255});

    // Giếng 10x11 ô, tính sao cho luôn nằm gọn trong khung tranh.
    float cell = area.height / 14.5f;
    float boardW = cell * 10.0f;
    float boardX = area.x + area.width * 0.64f - boardW * 0.5f;
    float boardY = area.y + area.height * 0.11f;
    Rectangle board = {boardX, boardY, boardW, cell * 11.0f};

    // Khung giếng
    DrawRectangleRounded(board, 0.06f, 8, UiAlpha((Color){8, 16, 38, 255}, 0.55f));
    DrawRectangleRoundedLinesEx(board, 0.06f, 8, 1.5f, UiAlpha(WHITE, 0.16f));

    // Lưới mờ bên trong giếng
    for (int gx = 1; gx < 10; gx++) {
        DrawLineEx((Vector2){board.x + gx * cell, board.y},
                   (Vector2){board.x + gx * cell, board.y + board.height}, 1.0f, UiAlpha(WHITE, 0.05f));
    }
    for (int gy = 1; gy < 11; gy++) {
        DrawLineEx((Vector2){board.x, board.y + gy * cell},
                   (Vector2){board.x + board.width, board.y + gy * cell}, 1.0f, UiAlpha(WHITE, 0.05f));
    }

    // Đống gạch đã xếp: chiều cao cố định để bố cục luôn cân đối.
    const int stack[10] = {3, 5, 4, 6, 5, 7, 4, 3, 5, 2};
    Rectangle boardOrigin = {board.x, board.y, 0, 0};
    for (int gx = 0; gx < 10; gx++) {
        for (int i = 0; i < stack[gx]; i++) {
            int gy = 11 - 1 - i;
            Color c = TETRO_COLORS[(gx * 3 + i) % 7];
            TetroCell(boardOrigin, (float)gx, (float)gy, cell, UiAlpha(c, 0.92f));
        }
    }

    // Khối đang rơi, lơ lửng theo nhịp thở
    float fall = 1.2f + sinf(time * 1.6f) * 0.35f;
    TetroCell(boardOrigin, 4.0f, fall, cell, TETRO_COLORS[0]);
    TetroCell(boardOrigin, 5.0f, fall, cell, TETRO_COLORS[0]);
    TetroCell(boardOrigin, 6.0f, fall, cell, TETRO_COLORS[0]);
    TetroCell(boardOrigin, 7.0f, fall, cell, TETRO_COLORS[0]);

    // Vệt sáng của hàng sắp ăn điểm
    float flash = 0.5f + 0.5f * sinf(time * 3.0f);
    DrawRectangleRec((Rectangle){board.x, board.y + board.height - cell * 1.0f, board.width, cell * 0.9f},
                     UiAlpha(WHITE, 0.06f + 0.10f * flash));
}

// ---------------------------------------------------------------- SPACE

static void Alien(Vector2 center, float px, Color color, int frame)
{
    static const char *rowsA[8] = {
        "  x     x  ", "   x   x   ", "  xxxxxxx  ", " xx xxx xx ",
        "xxxxxxxxxxx", "x xxxxxxx x", "x x     x x", "   xx xx   "
    };
    static const char *rowsB[8] = {
        "  x     x  ", "x  x   x  x", "x xxxxxxx x", "xxx xxx xxx",
        "xxxxxxxxxxx", " xxxxxxxxx ", "  x     x  ", " x       x "
    };
    const char **rows = (frame % 2 == 0) ? rowsA : rowsB;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 11; c++) {
            if (rows[r][c] != 'x') continue;
            DrawRectangleRec((Rectangle){center.x + (c - 5.5f) * px, center.y + (r - 4.0f) * px,
                                         px * 0.92f, px * 0.92f}, color);
        }
    }
}

void HubArtSpaceIcon(Rectangle area, float time)
{
    float px = Unit(area) / 13.0f;
    Alien(At(area, 0.5f, 0.45f), px, (Color){122, 232, 176, 255}, (int)(time * 2.0f));

    // Pháo phòng thủ phía dưới
    float w = Unit(area) * 0.42f;
    Vector2 base = At(area, 0.5f, 0.86f);
    DrawRectangleRounded((Rectangle){base.x - w * 0.5f, base.y - px * 0.8f, w, px * 1.5f},
                         0.4f, 6, (Color){200, 236, 255, 255});
    DrawRectangleRounded((Rectangle){base.x - px * 0.5f, base.y - px * 2.2f, px, px * 1.6f},
                         0.5f, 4, (Color){200, 236, 255, 255});
}

void HubArtSpaceHero(Rectangle area, float time)
{
    FillBackdrop(area, (Color){10, 26, 40, 255}, (Color){6, 14, 26, 255});
    StarField(area, time, 70, (Color){200, 255, 235, 255});
    Bloom(At(area, 0.66f, 0.68f), Unit(area) * 0.9f, (Color){60, 220, 160, 255});

    // Đội hình chiếm nửa phải của tranh, chừa nửa trái cho chữ.
    const int cols = 6, rows = 3;
    float fieldX = area.x + area.width * 0.50f;
    float fieldW = area.width * 0.44f;
    float colGap = fieldW / (float)cols;
    float px = colGap / 13.0f;                 // Alien rộng 11 ô + khoảng hở
    float rowGap = px * 12.0f;
    float sway = sinf(time * 0.9f) * colGap * 0.22f;
    float topY = area.y + area.height * 0.20f;

    for (int row = 0; row < rows; row++) {
        Color c = (row == 0) ? (Color){255, 184, 204, 255}
                : (row == 1) ? (Color){164, 226, 255, 255}
                             : (Color){126, 234, 180, 255};
        for (int col = 0; col < cols; col++) {
            Vector2 p = {fieldX + (col + 0.5f) * colGap + sway, topY + row * rowGap};
            Alien(p, px, UiAlpha(c, 0.95f), (int)(time * 2.0f) + row);
        }
    }

    // Pháo phòng thủ
    float unit = area.height / 52.0f;
    Vector2 base = {fieldX + fieldW * 0.5f, area.y + area.height * 0.86f};
    DrawRectangleRounded((Rectangle){base.x - unit * 4.0f, base.y, unit * 8.0f, unit * 2.4f},
                         0.4f, 6, (Color){214, 242, 255, 255});
    DrawRectangleRounded((Rectangle){base.x - unit * 0.9f, base.y - unit * 3.0f, unit * 1.8f, unit * 3.2f},
                         0.5f, 4, (Color){214, 242, 255, 255});

    // Boongke phòng thủ
    for (int i = 0; i < 3; i++) {
        float bx = fieldX + fieldW * (0.12f + i * 0.34f);
        DrawRectangleRounded((Rectangle){bx, area.y + area.height * 0.72f, unit * 6.0f, unit * 3.0f},
                             0.35f, 6, UiAlpha((Color){110, 200, 160, 255}, 0.70f));
    }

    // Đạn đang bay lên
    for (int i = 0; i < 3; i++) {
        float t = fmodf(time * 0.7f + i * 0.33f, 1.0f);
        float bx = fieldX + fieldW * (0.25f + i * 0.28f);
        float by = area.y + area.height * (0.88f - t * 0.52f);
        DrawRectangleRec((Rectangle){bx, by, unit * 0.7f, unit * 2.6f},
                         UiAlpha((Color){255, 246, 180, 255}, 1.0f - t * 0.5f));
    }
}

// ---------------------------------------------------------------- SNAKE

void HubArtSnakeIcon(Rectangle area, float time)
{
    (void)time;
    float cell = Unit(area) / 5.2f;
    Vector2 o = At(area, 0.12f, 0.36f);

    for (int i = 0; i < 3; i++) {
        Color c = UiMix((Color){56, 142, 76, 255}, (Color){170, 240, 150, 255}, i / 2.0f);
        DrawRectangleRounded((Rectangle){o.x + i * cell, o.y, cell * 0.88f, cell * 0.88f}, 0.35f, 6, c);
    }
    // Mắt
    DrawCircleV((Vector2){o.x + 2.55f * cell, o.y + cell * 0.30f}, cell * 0.10f, (Color){20, 40, 30, 255});
    // Mồi
    Vector2 apple = (Vector2){o.x + 4.0f * cell, o.y + cell * 0.44f};
    DrawCircleV(apple, cell * 0.42f, (Color){255, 130, 130, 255});
    DrawRectangleRounded((Rectangle){apple.x - cell * 0.06f, apple.y - cell * 0.62f, cell * 0.12f, cell * 0.24f},
                         0.5f, 4, (Color){120, 200, 120, 255});
}

void HubArtSnakeHero(Rectangle area, float time)
{
    FillBackdrop(area, (Color){18, 48, 40, 255}, (Color){8, 22, 22, 255});
    Bloom(At(area, 0.68f, 0.45f), Unit(area) * 0.95f, (Color){90, 230, 150, 255});

    float cell = area.height / 11.0f;
    Rectangle grid = {area.x + area.width * 0.38f, area.y + area.height * 0.12f, cell * 11.0f, cell * 8.0f};

    DrawRectangleRounded(grid, 0.05f, 8, UiAlpha((Color){6, 24, 20, 255}, 0.5f));
    for (int gx = 0; gx <= 11; gx++) {
        DrawLineEx((Vector2){grid.x + gx * cell, grid.y},
                   (Vector2){grid.x + gx * cell, grid.y + grid.height}, 1.0f, UiAlpha(WHITE, 0.05f));
    }
    for (int gy = 0; gy <= 8; gy++) {
        DrawLineEx((Vector2){grid.x, grid.y + gy * cell},
                   (Vector2){grid.x + grid.width, grid.y + gy * cell}, 1.0f, UiAlpha(WHITE, 0.05f));
    }
    DrawRectangleRoundedLinesEx(grid, 0.05f, 8, 1.5f, UiAlpha(WHITE, 0.16f));

    // Thân rắn bò theo một đường gấp khúc cố định, có hiệu ứng trườn.
    static const int path[12][2] = {
        {1,6},{2,6},{3,6},{4,6},{4,5},{4,4},{5,4},{6,4},{7,4},{7,3},{7,2},{8,2}
    };
    int head = (int)(time * 3.0f) % 12;
    for (int i = 0; i < 9; i++) {
        int idx = (head - i + 12 + 12) % 12;
        float t = 1.0f - (float)i / 9.0f;
        Color c = UiMix((Color){40, 110, 70, 255}, (Color){170, 245, 160, 255}, t);
        Rectangle seg = {grid.x + path[idx][0] * cell + cell * 0.08f,
                         grid.y + path[idx][1] * cell + cell * 0.08f,
                         cell * 0.84f, cell * 0.84f};
        DrawRectangleRounded(seg, 0.34f, 6, c);
        if (i == 0) {
            DrawCircleV((Vector2){seg.x + seg.width * 0.68f, seg.y + seg.height * 0.32f}, cell * 0.08f, (Color){15, 35, 25, 255});
            DrawCircleV((Vector2){seg.x + seg.width * 0.68f, seg.y + seg.height * 0.68f}, cell * 0.08f, (Color){15, 35, 25, 255});
        }
    }

    // Mồi nhấp nháy
    float pulse = 0.85f + 0.15f * sinf(time * 4.0f);
    Vector2 apple = {grid.x + 9.5f * cell, grid.y + 6.5f * cell};
    DrawCircleV(apple, cell * 0.36f * pulse, (Color){255, 128, 128, 255});
    DrawCircleV((Vector2){apple.x - cell * 0.10f, apple.y - cell * 0.12f}, cell * 0.10f, UiAlpha(WHITE, 0.6f));
}

// -------------------------------------------------------------- FIGHTER

// Người que tỉ lệ theo chiều cao `h`, quay mặt sang phải khi facing = 1.
static void Fighter(Vector2 feet, float h, int facing, Color gi, Color skin, Color hair, float lean)
{
    float u = h / 10.0f;
    float bodyH = u * 4.2f;
    Vector2 hip = {feet.x + lean * u * 0.6f, feet.y - u * 3.4f};
    Vector2 chest = {hip.x + lean * u * 0.5f, hip.y - bodyH * 0.55f};
    Vector2 head = {chest.x + lean * u * 0.4f, chest.y - u * 1.5f};

    // Chân
    DrawLineEx(hip, (Vector2){feet.x - u * 1.5f * facing, feet.y}, u * 0.75f, gi);
    DrawLineEx(hip, (Vector2){feet.x + u * 1.6f * facing, feet.y}, u * 0.75f, gi);
    // Thân
    DrawLineEx(hip, chest, u * 1.5f, gi);
    // Tay trước tung đòn
    DrawLineEx(chest, (Vector2){chest.x + u * 2.6f * facing, chest.y + u * 0.2f}, u * 0.62f, skin);
    DrawCircleV((Vector2){chest.x + u * 2.8f * facing, chest.y + u * 0.2f}, u * 0.55f, skin);
    // Tay sau thủ
    DrawLineEx(chest, (Vector2){chest.x - u * 1.1f * facing, chest.y + u * 0.9f}, u * 0.58f, skin);
    // Đầu
    DrawCircleV(head, u * 0.85f, skin);
    DrawCircleSector(head, u * 0.88f, 180.0f, 360.0f, 16, hair);
}

void HubArtFighterIcon(Rectangle area, float time)
{
    float h = Unit(area) * 0.56f;
    Vector2 ground = At(area, 0.5f, 0.86f);

    Fighter((Vector2){ground.x - Unit(area) * 0.26f, ground.y}, h, 1,
            (Color){240, 240, 235, 255}, (Color){240, 200, 170, 255}, (Color){70, 50, 40, 255}, 0.2f);
    Fighter((Vector2){ground.x + Unit(area) * 0.26f, ground.y}, h, -1,
            (Color){230, 110, 100, 255}, (Color){245, 210, 180, 255}, (Color){240, 210, 110, 255}, -0.2f);

    float pulse = 0.8f + 0.2f * sinf(time * 5.0f);
    DrawCircleV(At(area, 0.5f, 0.52f), Unit(area) * 0.11f * pulse, UiAlpha((Color){150, 210, 255, 255}, 0.75f));
    DrawCircleV(At(area, 0.5f, 0.52f), Unit(area) * 0.06f * pulse, UiAlpha(WHITE, 0.95f));
}

void HubArtFighterHero(Rectangle area, float time)
{
    FillBackdrop(area, (Color){62, 26, 48, 255}, (Color){16, 10, 26, 255});
    Bloom(At(area, 0.62f, 0.42f), Unit(area) * 1.1f, (Color){255, 120, 110, 255});

    // Sàn đấu
    float groundY = area.y + area.height * 0.84f;
    DrawRectangleGradientV((int)area.x, (int)groundY, (int)area.width, (int)(area.y + area.height - groundY),
                           (Color){52, 24, 40, 255}, (Color){24, 12, 22, 255});
    DrawLineEx((Vector2){area.x, groundY}, (Vector2){area.x + area.width, groundY}, 2.0f, UiAlpha((Color){255, 170, 150, 255}, 0.35f));

    float h = area.height * 0.46f;
    float lean = sinf(time * 1.8f) * 0.35f;

    Fighter((Vector2){area.x + area.width * 0.48f, groundY}, h, 1,
            (Color){242, 242, 238, 255}, (Color){238, 198, 168, 255}, (Color){70, 50, 40, 255}, 0.25f + lean * 0.2f);
    Fighter((Vector2){area.x + area.width * 0.78f, groundY}, h, -1,
            (Color){228, 96, 92, 255}, (Color){246, 212, 182, 255}, (Color){240, 208, 104, 255}, -0.25f - lean * 0.2f);

    // Hadouken bay giữa hai nhân vật
    float t = fmodf(time * 0.8f, 1.0f);
    Vector2 orb = {area.x + area.width * (0.53f + t * 0.20f), groundY - h * 0.52f};
    float r = area.height * (0.05f + t * 0.02f);
    BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient(orb, r * 2.6f, UiAlpha((Color){120, 190, 255, 255}, 0.55f), UiAlpha((Color){120, 190, 255, 255}, 0.0f));
    EndBlendMode();
    DrawCircleV(orb, r, UiAlpha((Color){170, 220, 255, 255}, 0.92f));
    DrawCircleV(orb, r * 0.55f, UiAlpha(WHITE, 0.95f));
}

// --------------------------------------------------------------- FLAPPY

// Flappy Plane dùng thẳng bộ sprite CC0 của game thay vì vẽ primitive, nên
// artwork nạp texture (dùng chung bộ đếm tham chiếu với game) ngay lần vẽ đầu.
static bool s_flappyArtLoaded = false;

static const FlappyAssets *FlappyArt(void)
{
    if (!s_flappyArtLoaded) {
        LoadFlappyAssets();
        s_flappyArtLoaded = true;
    }
    return FlappyAssetsGet();
}

static bool s_caroArtLoaded = false;
static Texture2D s_chessHero = {0};
static Texture2D s_chessIcon = {0};
static bool s_chessArtTried = false;

void HubArtRelease(void)
{
    if (s_flappyArtLoaded) {
        UnloadFlappyAssets();
        s_flappyArtLoaded = false;
    }
    if (s_caroArtLoaded) {
        UnloadCaroAssets();
        s_caroArtLoaded = false;
    }
    if (s_chessHero.id > 0) UnloadTexture(s_chessHero);
    if (s_chessIcon.id > 0) UnloadTexture(s_chessIcon);
    s_chessHero = (Texture2D){0};
    s_chessIcon = (Texture2D){0};
    s_chessArtTried = false;
}

static void DrawTexCentered(Texture2D tex, Vector2 c, float scale, float rot)
{
    Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
    Rectangle dst = {c.x, c.y, tex.width * scale, tex.height * scale};
    DrawTexturePro(tex, src, dst, (Vector2){dst.width * 0.5f, dst.height * 0.5f}, rot, WHITE);
}

void HubArtFlappyIcon(Rectangle area, float time)
{
    const FlappyAssets *a = FlappyArt();
    Vector2 c = At(area, 0.5f, 0.5f);
    DrawCircleV(c, Unit(area) * 0.46f, (Color){196, 232, 248, 255});
    DrawCircleV(At(area, 0.5f, 0.62f), Unit(area) * 0.30f, UiAlpha(WHITE, 0.55f));

    int frame = (int)(time * 18.0f) % PLANE_FRAMES;
    float bob = sinf(time * 3.0f) * Unit(area) * 0.04f;
    float scale = Unit(area) * 0.86f / (float)a->plane[0][0].width;
    DrawTexCentered(a->plane[0][frame], (Vector2){c.x, c.y + bob}, scale, sinf(time * 3.0f + 1.0f) * 6.0f);
}

void HubArtFlappyHero(Rectangle area, float time)
{
    const FlappyAssets *a = FlappyArt();

    // Nền trời Kenney phủ kín khung, cuộn chậm sang trái
    float s = area.height / (float)a->background.height;
    float bgW = a->background.width * s;
    float off = fmodf(time * 18.0f * s, bgW);
    for (int i = 0; i < 3; i++) {
        Rectangle src = {0.0f, 0.0f, (float)a->background.width, (float)a->background.height};
        Rectangle dst = {area.x - off + i * bgW, area.y, bgW + 1.0f, area.height};
        DrawTexturePro(a->background, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    }

    // Hai cặp đá ở nửa phải (nửa trái chừa cho chữ)
    float groundTop = area.y + area.height - GROUND_TEX_H * s;
    float rockW = ROCK_W * s, rockH = ROCK_H * s;
    const float rx[2] = {0.60f, 0.84f};
    const float gapC[2] = {0.44f, 0.36f};
    for (int i = 0; i < 2; i++) {
        float x = area.x + area.width * rx[i] - rockW * 0.5f;
        float gap = area.height * 0.34f;
        float topTip = area.y + area.height * gapC[i] - gap * 0.5f;
        float bottomTip = topTip + gap;
        float topH = fmaxf(rockH, topTip - area.y + 10.0f);
        float bottomH = fmaxf(rockH, area.y + area.height - bottomTip + 4.0f);
        Rectangle src = {0.0f, 0.0f, (float)ROCK_W, (float)ROCK_H};
        DrawTexturePro(a->rockDown[i], src, (Rectangle){x, topTip - topH, rockW, topH}, (Vector2){0}, 0.0f, WHITE);
        DrawTexturePro(a->rock[i], src, (Rectangle){x, bottomTip, rockW, bottomH}, (Vector2){0}, 0.0f, WHITE);
    }

    // Sao lấp lánh giữa hai cặp đá
    float starScale = s * (0.85f + 0.1f * sinf(time * 5.0f));
    DrawTexCentered(a->star[STAR_GOLD], At(area, 0.72f, 0.30f + 0.02f * sinf(time * 3.0f)), starScale, 0.0f);

    // Dải đất cuộn nhanh hơn nền (thị sai)
    float gW = GROUND_TEX_W * s;
    float gOff = fmodf(time * 90.0f * s, gW);
    for (int i = 0; i < 3; i++) {
        Rectangle src = {0.0f, 0.0f, (float)GROUND_TEX_W, (float)GROUND_TEX_H};
        Rectangle dst = {area.x - gOff + i * gW, groundTop, gW + 1.0f, GROUND_TEX_H * s};
        DrawTexturePro(a->ground[0], src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    }

    // Máy bay nhấp nhô xuyên qua khe đá
    int frame = (int)(time * 20.0f) % PLANE_FRAMES;
    float wave = sinf(time * 2.2f);
    Vector2 plane = At(area, 0.50f, 0.42f + wave * 0.06f);
    DrawTexCentered(a->puffSmall, (Vector2){plane.x - 44.0f * s, plane.y + 4.0f * s}, s * 0.7f, time * 40.0f);
    DrawTexCentered(a->puffLarge, (Vector2){plane.x - 70.0f * s, plane.y + 8.0f * s}, s * 0.55f, -time * 30.0f);
    DrawTexCentered(a->plane[1][frame], plane, s * 0.85f, -wave * 12.0f);
}

// --------------------------------------------------------------- CỜ CARO

// Dùng chính vân gỗ và shader vẽ quân X/O của game, nên tranh trong Hub
// giống hệt bàn cờ khi vào chơi.
static void CaroArt(void)
{
    if (!s_caroArtLoaded) {
        LoadCaroAssets();
        s_caroArtLoaded = true;
    }
}

static const Color CARO_X_COL  = {236, 84, 84, 255};
static const Color CARO_X_EDGE = {150, 28, 40, 255};
static const Color CARO_O_COL  = {64, 142, 236, 255};
static const Color CARO_O_EDGE = {22, 64, 150, 255};

static void CaroWood(Rectangle dst, float texScale, Color tint)
{
    const CaroAssets *a = CaroAssetsGet();
    Rectangle src = {0.0f, 0.0f, dst.width * texScale, dst.height * texScale};
    DrawTexturePro(a->wood, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, tint);
}

void HubArtCaroIcon(Rectangle area, float time)
{
    CaroArt();
    float u = Unit(area);
    Rectangle board = {area.x + area.width * 0.5f - u * 0.46f, area.y + area.height * 0.5f - u * 0.46f, u * 0.92f, u * 0.92f};
    CaroWood(board, 2.0f, WHITE);
    DrawRectangleRec(board, (Color){255, 234, 196, 70});
    for (int i = 1; i < 3; i++) {
        float t = board.width * (float)i / 3.0f;
        DrawLineEx((Vector2){board.x + t, board.y}, (Vector2){board.x + t, board.y + board.height}, fmaxf(1.0f, u * 0.012f), (Color){92, 58, 30, 150});
        DrawLineEx((Vector2){board.x, board.y + t}, (Vector2){board.x + board.width, board.y + t}, fmaxf(1.0f, u * 0.012f), (Color){92, 58, 30, 150});
    }
    float c = board.width / 3.0f;
    float pulse = 0.25f + 0.2f * sinf(time * 2.5f);
    CaroDrawShape(CARO_SHAPE_X, (Rectangle){board.x + c * 0.08f, board.y + c * 0.08f, c * 0.84f, c * 0.84f}, 0.0f, CARO_X_COL, CARO_X_EDGE, 1.0f, 0.0f, 1.0f);
    CaroDrawShape(CARO_SHAPE_O, (Rectangle){board.x + c * 1.08f, board.y + c * 1.08f, c * 0.84f, c * 0.84f}, 0.0f, CARO_O_COL, CARO_O_EDGE, 1.0f, pulse, 1.0f);
    CaroDrawShape(CARO_SHAPE_X, (Rectangle){board.x + c * 2.08f, board.y + c * 2.08f, c * 0.84f, c * 0.84f}, 0.0f, CARO_X_COL, CARO_X_EDGE, 1.0f, 0.0f, 1.0f);
}

void HubArtCaroHero(Rectangle area, float time)
{
    CaroArt();
    FillBackdrop(area, (Color){46, 30, 22, 255}, (Color){14, 10, 10, 255});
    Bloom(At(area, 0.78f, 0.35f), area.height * 0.9f, (Color){255, 180, 110, 255});

    // Bàn gỗ lớn tràn mép phải, nghiêng góc nhẹ cho có chiều sâu
    float cell = area.height / 9.0f;
    int cols = 10, rows = 11;
    Rectangle board = {area.x + area.width * 0.48f, area.y - cell * 0.6f, cell * cols, cell * rows};
    for (int i = 6; i >= 1; i--) {
        float g = cell * 0.12f * (float)i;
        DrawRectangleRec((Rectangle){board.x - g, board.y - g + cell * 0.2f, board.width + g * 2, board.height + g * 2},
                         UiAlpha(BLACK, 0.08f));
    }
    CaroWood(board, 1.0f, WHITE);
    DrawRectangleRec(board, (Color){255, 234, 196, 80});
    for (int i = 0; i <= cols; i++) {
        DrawLineEx((Vector2){board.x + i * cell, board.y}, (Vector2){board.x + i * cell, board.y + board.height}, 1.5f, (Color){92, 58, 30, 150});
    }
    for (int i = 0; i <= rows; i++) {
        DrawLineEx((Vector2){board.x, board.y + i * cell}, (Vector2){board.x + board.width, board.y + i * cell}, 1.5f, (Color){92, 58, 30, 150});
    }

    // Ván mẫu: quân lần lượt được vẽ nét, cuối cùng X thắng đường chéo
    static const int MOVES[][2] = {{3, 5}, {4, 5}, {4, 4}, {5, 5}, {2, 6}, {3, 4}, {5, 3}, {6, 2}, {1, 7}};
    const int count = (int)(sizeof(MOVES) / sizeof(MOVES[0]));
    float cycle = count * 0.6f + 2.8f;
    float t = fmodf(time, cycle);
    float fade = fminf(1.0f, (cycle - t) * 2.0f);
    for (int i = 0; i < count; i++) {
        float appear = t - i * 0.6f;
        if (appear < 0.0f) break;
        float p = fminf(1.0f, appear * 3.0f);
        Rectangle r = {board.x + MOVES[i][0] * cell + cell * 0.1f, board.y + MOVES[i][1] * cell + cell * 0.1f, cell * 0.8f, cell * 0.8f};
        bool isX = (i % 2) == 0;
        Color col = UiAlpha(isX ? CARO_X_COL : CARO_O_COL, fade);
        CaroDrawShape(isX ? CARO_SHAPE_X : CARO_SHAPE_O, r, 0.0f, col, isX ? CARO_X_EDGE : CARO_O_EDGE,
                      1.0f - (1.0f - p) * (1.0f - p) * (1.0f - p), 0.0f, 1.0f);
    }
    float done = t - count * 0.6f;
    if (done > 0.0f) {
        // Vạch vàng nối năm quân X (1,7) -> (5,3)
        Vector2 a = {board.x + 1.5f * cell, board.y + 7.5f * cell};
        Vector2 b = {board.x + 5.5f * cell, board.y + 3.5f * cell};
        float len = sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)) + cell * 0.9f;
        Rectangle bar = {(a.x + b.x) * 0.5f - len * 0.5f, (a.y + b.y) * 0.5f - cell * 0.34f, len, cell * 0.68f};
        float p = fminf(1.0f, done * 1.8f);
        CaroDrawShape(CARO_SHAPE_BAR, bar, -45.0f, UiAlpha((Color){255, 222, 120, 235}, fade), (Color){214, 140, 40, 255},
                      1.0f - (1.0f - p) * (1.0f - p), 0.6f + 0.4f * sinf(time * 4.0f), 0.6f);
    }
}

// --------------------------------------------------------------- CỜ VUA 3D

// Hub vẽ tranh vào render texture riêng nên không dựng được cảnh 3D có bóng
// đổ ở đây; dùng ảnh chụp sẵn từ chính game (assets/chess/textures/hub_*.jpg).
static void ChessArt(void)
{
    if (s_chessArtTried) return;
    s_chessArtTried = true;
    const char *hero = ChessAssetPath("textures/hub_hero.jpg");
    const char *icon = ChessAssetPath("textures/hub_icon.jpg");
    if (FileExists(hero)) s_chessHero = LoadTexture(hero);
    if (FileExists(icon)) s_chessIcon = LoadTexture(icon);
    if (s_chessHero.id > 0) SetTextureFilter(s_chessHero, TEXTURE_FILTER_BILINEAR);
    if (s_chessIcon.id > 0) SetTextureFilter(s_chessIcon, TEXTURE_FILTER_BILINEAR);
}

// Phủ kín khung theo kiểu "cover", kèm độ phóng nhẹ (Ken Burns).
static void DrawCover(Texture2D tex, Rectangle area, float zoom, Vector2 focus)
{
    float s = fmaxf(area.width / tex.width, area.height / tex.height) * zoom;
    float w = area.width / s, h = area.height / s;
    float x = (tex.width - w) * focus.x, y = (tex.height - h) * focus.y;
    DrawTexturePro(tex, (Rectangle){x, y, w, h}, area, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
}

// Quân mã vẽ bằng khối đơn giản - chỉ dùng khi thiếu ảnh chụp sẵn.
static void ChessFallback(Rectangle area, float time)
{
    FillBackdrop(area, (Color){40, 40, 48, 255}, (Color){10, 10, 14, 255});
    Bloom(At(area, 0.7f, 0.4f), Unit(area) * 0.8f, (Color){236, 196, 120, 255});
    float u = Unit(area);
    float cell = u * 0.14f;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            Color c = ((x + y) & 1) ? (Color){210, 208, 202, 255} : (Color){60, 62, 68, 255};
            DrawRectangleRec((Rectangle){area.x + area.width * 0.55f + x * cell, area.y + area.height * 0.12f + y * cell, cell, cell}, c);
        }
    }
    (void)time;
}

void HubArtChessIcon(Rectangle area, float time)
{
    ChessArt();
    if (s_chessIcon.id == 0) {
        ChessFallback(area, time);
        return;
    }
    DrawCover(s_chessIcon, area, 1.0f, (Vector2){0.5f, 0.5f});
}

void HubArtChessHero(Rectangle area, float time)
{
    ChessArt();
    if (s_chessHero.id == 0) {
        ChessFallback(area, time);
        return;
    }
    float zoom = 1.04f + 0.04f * sinf(time * 0.12f);
    DrawCover(s_chessHero, area, zoom, (Vector2){0.72f, 0.5f + 0.1f * sinf(time * 0.09f)});
}
