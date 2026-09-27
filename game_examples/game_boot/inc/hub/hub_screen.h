/**
 * hub_screen.h - Màn hình launcher: điều phối topbar, sidebar, hero, info.
 *
 * HubContext là "bảng thông tin" mà mọi thành phần con đọc để tự vẽ,
 * nhờ đó các module con không cần biết về nhau.
 */
#ifndef HUB_SCREEN_H
#define HUB_SCREEN_H

#include "boot_types.h"
#include "game_registry.h"

typedef struct {
    BootApp *app;
    const GameEntry *game;   // Game đang được chọn
    int gameIndex;
    float time;
    Color accent;            // Màu nhấn đã nội suy mượt khi đổi game
    Rectangle window;        // Khung cửa sổ kính ngoài cùng
    Rectangle content;       // Vùng nội dung bên trong cửa sổ
    bool reduceMotion;
} HubContext;

void HubScreenInit(BootApp *app);
void HubScreenUpdate(BootApp *app, float dt);
void HubScreenPrepare(BootApp *app);  // Vẽ nền + tranh hero vào RenderTexture (trước canvas chính)
void HubScreenDraw(BootApp *app);
void HubScreenClose(void);

#endif // HUB_SCREEN_H
