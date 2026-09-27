/**
 * boot_types.h - Các kiểu dữ liệu nền tảng dùng chung cho Arcade Hub.
 *
 * File này cố tình giữ ở mức "thuần dữ liệu": không khai báo hàm xử lý,
 * không phụ thuộc vào module UI. Nhờ vậy mọi tầng (core / ui / hub) đều
 * có thể include mà không tạo vòng phụ thuộc.
 */
#ifndef BOOT_TYPES_H
#define BOOT_TYPES_H

#include "raylib.h"
#include "boot_viewport.h"
#include <stdbool.h>

// Độ phân giải ảo của màn hình Hub. Mọi toạ độ trong tầng UI đều tính theo hệ này.
#define HUB_VIRTUAL_WIDTH  1280
#define HUB_VIRTUAL_HEIGHT 720

// Số game tối đa mà registry có thể chứa (tăng số này khi thêm nhiều game).
#define BOOT_MAX_GAMES 16

// Trạng thái cấp cao của ứng dụng.
typedef enum {
    BOOT_STATE_HUB = 0,   // Đang ở màn hình launcher
    BOOT_STATE_GAME       // Đang chạy một game con
} BootState;

// Các tab trên thanh điều hướng trên cùng.
typedef enum {
    HUB_TAB_LIBRARY = 0,  // Thư viện game
    HUB_TAB_CONTROLS,     // Bảng phím điều khiển
    HUB_TAB_SETTINGS,     // Cài đặt ứng dụng
    HUB_TAB_COUNT
} HubTab;

// Virtual canvas: render toàn bộ nội dung vào một RenderTexture rồi scale
// đồng dạng ra cửa sổ thật (letterbox/pillarbox).
typedef struct {
    RenderTexture2D target;
    int virtualWidth;
    int virtualHeight;
    Rectangle sourceRec;
    Rectangle destRec;
    float scale;
} BootCanvas;

// Trạng thái chuyển cảnh mờ dần giữa Hub và game.
typedef struct {
    bool active;        // Đang trong pha fade-out để đổi màn hình
    float alpha;        // 0 = trong suốt, 1 = đen hoàn toàn
    int pendingGame;    // Chỉ số game sẽ mở, -1 nghĩa là quay về Hub
} BootTransition;

// Toàn bộ trạng thái runtime của ứng dụng.
typedef struct {
    BootState state;
    int activeGame;         // Chỉ số game đang chạy (-1 khi ở Hub)
    int selectedGame;       // Chỉ số game đang được chọn trong thư viện
    HubTab tab;
    BootTransition transition;
    BootCanvas canvas;      // Dùng khi chạy game con (độ phân giải cố định)
    BootViewport viewport;  // Dùng cho màn hình Hub (vẽ ở độ phân giải gốc)
    float globalTime;       // Đồng hồ tích luỹ, dùng cho mọi hiệu ứng động
} BootApp;

#endif // BOOT_TYPES_H
