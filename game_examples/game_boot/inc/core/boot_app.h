/**
 * boot_app.h - Vòng đời ứng dụng: khởi tạo, cập nhật, vẽ, dọn dẹp.
 *
 * main.c chỉ còn nhiệm vụ dựng cửa sổ và quay vòng lặp; mọi điều phối
 * giữa Hub và các game con nằm ở đây.
 */
#ifndef BOOT_APP_H
#define BOOT_APP_H

#include "boot_types.h"

void BootAppInit(BootApp *app);
void BootAppUpdate(BootApp *app, float dt);
void BootAppDraw(BootApp *app);
void BootAppClose(BootApp *app);

// Yêu cầu chuyển màn hình; gameIndex = -1 nghĩa là quay về Hub.
void BootAppRequestGame(BootApp *app, int gameIndex);

#endif // BOOT_APP_H
