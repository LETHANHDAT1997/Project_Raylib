/**
 * hub_pages.h - Nội dung của các tab không phải Thư viện.
 */
#ifndef HUB_PAGES_H
#define HUB_PAGES_H

#include "hub_screen.h"

void HubControlsPageDraw(HubContext *ctx, Rectangle area);
void HubSettingsPageDraw(HubContext *ctx, Rectangle area);

#endif // HUB_PAGES_H
