/**
 * game_registry.h - Danh mục game theo kiểu "data driven".
 *
 * Muốn thêm một game mới vào launcher, chỉ cần:
 *   1. Viết 4 hàm vòng đời Init/Update/Draw/Close trong thư mục game đó.
 *   2. Vẽ hai hàm artwork (icon nhỏ cho sidebar, tranh lớn cho hero) ở hub_art.c.
 *   3. Thêm một phần tử vào mảng trong src/core/game_registry.c.
 * Không cần sửa bất kỳ file UI nào khác.
 */
#ifndef GAME_REGISTRY_H
#define GAME_REGISTRY_H

#include "boot_types.h"

#define GAME_MAX_TAGS     4
#define GAME_MAX_CONTROLS 6

typedef void (*GameInitFn)(void);
typedef void (*GameUpdateFn)(float dt);
typedef void (*GameDrawFn)(void);
typedef void (*GameCloseFn)(void);

// Hàm vẽ artwork: nhận khung chữ nhật cần lấp đầy và thời gian để làm động.
typedef void (*GameArtFn)(Rectangle area, float time);

typedef struct {
    const char *id;            // Khoá bền vững dùng khi lưu thống kê ra file
    const char *title;         // Tên hiển thị
    const char *platform;      // Dòng phụ dưới tên trong sidebar
    const char *tagline;       // Mô tả ngắn hiển thị ở hero (tối đa 2 dòng)
    const char *developer;
    const char *releaseDate;
    const char *engine;

    const char *tags[GAME_MAX_TAGS];
    int tagCount;

    const char *controls[GAME_MAX_CONTROLS];
    int controlCount;

    int canvasWidth;           // Độ phân giải ảo riêng của game
    int canvasHeight;

    Color accent;              // Màu chủ đạo, lan toả ra cả nền và nút bấm
    Color accentDeep;          // Biến thể tối hơn, dùng cho gradient

    GameArtFn drawIcon;        // Ảnh thumbnail vuông trong sidebar
    GameArtFn drawHeroArt;     // Tranh lớn nền hero panel

    GameInitFn   init;
    GameUpdateFn update;
    GameDrawFn   draw;
    GameCloseFn  close;
} GameEntry;

int              GameRegistryCount(void);
const GameEntry *GameRegistryGet(int index);
int              GameRegistryIndexOfId(const char *id);

#endif // GAME_REGISTRY_H
