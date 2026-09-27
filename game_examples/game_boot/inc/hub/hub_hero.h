/**
 * hub_hero.h - Tấm hero lớn: tranh game, tiêu đề, mô tả, thẻ, nút chơi.
 */
#ifndef HUB_HERO_H
#define HUB_HERO_H

#include "hub_screen.h"

void HubHeroPrepare(HubContext *ctx, Rectangle area);  // Vẽ tranh vào RenderTexture
void HubHeroDraw(HubContext *ctx, Rectangle area);

// Menu "..." phải vẽ sau cùng để không bị các thẻ phía dưới che mất.
void HubHeroDrawOverlay(HubContext *ctx, Rectangle area);
void HubHeroRelease(void);

#endif // HUB_HERO_H
