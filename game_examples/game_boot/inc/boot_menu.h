#ifndef BOOT_MENU_H
#define BOOT_MENU_H

#include "boot_types.h"

void InitBootMenu(BootApp *app);
void UpdateBootMenu(BootApp *app, float dt);
void DrawBootMenu(const BootApp *app);

#endif // BOOT_MENU_H
