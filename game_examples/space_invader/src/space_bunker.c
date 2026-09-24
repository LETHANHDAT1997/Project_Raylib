#include "space_bunker.h"
#include "space_audio.h"
#include "space_game.h"

static const int DEFAULT_BUNKER_SHAPE[BUNKER_ROWS][BUNKER_COLS] = {
    {0, 3, 3, 3, 3, 0},
    {3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3},
    {3, 3, 0, 0, 3, 3},
    {3, 3, 0, 0, 3, 3}
};

void InitBunkers(SpaceBunker bunkers[MAX_BUNKERS])
{
    float bunkerWidth = BUNKER_COLS * BUNKER_BLOCK_SIZE;
    float totalSpacing = (SCREEN_WIDTH - 2 * 60.0f - MAX_BUNKERS * bunkerWidth) / (MAX_BUNKERS - 1);
    float bunkerY = SCREEN_HEIGHT - 190.0f;

    for (int i = 0; i < MAX_BUNKERS; i++) {
        bunkers[i].x = 60.0f + i * (bunkerWidth + totalSpacing);
        bunkers[i].y = bunkerY;

        for (int r = 0; r < BUNKER_ROWS; r++) {
            for (int c = 0; c < BUNKER_COLS; c++) {
                bunkers[i].grid[r][c] = DEFAULT_BUNKER_SHAPE[r][c];
            }
        }
    }
}

void DrawBunkers(const SpaceBunker bunkers[MAX_BUNKERS])
{
    for (int i = 0; i < MAX_BUNKERS; i++) {
        const SpaceBunker *b = &bunkers[i];

        for (int r = 0; r < BUNKER_ROWS; r++) {
            for (int c = 0; c < BUNKER_COLS; c++) {
                int hp = b->grid[r][c];
                if (hp <= 0) continue;

                Color col;
                if (hp == 3) col = (Color){ 46, 204, 113, 255 };      // Pristine Green
                else if (hp == 2) col = (Color){ 39, 174, 96, 220 }; // Damaged Green
                else col = (Color){ 26, 120, 70, 180 };              // Heavily chipped

                int bx = (int)(b->x + c * BUNKER_BLOCK_SIZE);
                int by = (int)(b->y + r * BUNKER_BLOCK_SIZE);

                DrawRectangle(bx, by, BUNKER_BLOCK_SIZE - 1, BUNKER_BLOCK_SIZE - 1, col);
            }
        }
    }
}

bool CheckBunkerCollision(SpaceBunker bunkers[MAX_BUNKERS], float bx, float by, float radius, SpaceGame *game)
{
    for (int i = 0; i < MAX_BUNKERS; i++) {
        SpaceBunker *b = &bunkers[i];

        float bw = BUNKER_COLS * BUNKER_BLOCK_SIZE;
        float bh = BUNKER_ROWS * BUNKER_BLOCK_SIZE;

        // Broad-phase AABB test
        if (bx + radius < b->x || bx - radius > b->x + bw ||
            by + radius < b->y || by - radius > b->y + bh) {
            continue;
        }

        // Narrow-phase test per block
        for (int r = 0; r < BUNKER_ROWS; r++) {
            for (int c = 0; c < BUNKER_COLS; c++) {
                if (b->grid[r][c] > 0) {
                    float cellX = b->x + c * BUNKER_BLOCK_SIZE;
                    float cellY = b->y + r * BUNKER_BLOCK_SIZE;

                    if (bx >= cellX && bx <= cellX + BUNKER_BLOCK_SIZE &&
                        by >= cellY && by <= cellY + BUNKER_BLOCK_SIZE) {
                        b->grid[r][c]--;
                        AddSpaceParticles(game, cellX + BUNKER_BLOCK_SIZE * 0.5f, cellY + BUNKER_BLOCK_SIZE * 0.5f, (Color){ 46, 204, 113, 255 }, 5);
                        PlaySoundBunkerHit(game->soundEnabled);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
