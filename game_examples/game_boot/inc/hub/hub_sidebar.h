/**
 * hub_sidebar.h - Danh sách game bên trái kèm ô tổng quan thống kê.
 */
#ifndef HUB_SIDEBAR_H
#define HUB_SIDEBAR_H

#include "hub_screen.h"

void HubSidebarUpdate(HubContext *ctx, Rectangle area, float dt);
void HubSidebarDraw(HubContext *ctx, Rectangle area);

#endif // HUB_SIDEBAR_H
