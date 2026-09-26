#ifndef BOOT_CANVAS_H
#define BOOT_CANVAS_H

#include "boot_types.h"

void InitBootCanvas(BootCanvas *canvas, int virtualWidth, int virtualHeight);
void SetBootCanvasSize(BootCanvas *canvas, int virtualWidth, int virtualHeight);
void UpdateBootCanvas(BootCanvas *canvas);
void BeginBootCanvas(BootCanvas *canvas);
void EndBootCanvas(BootCanvas *canvas);
void CloseBootCanvas(BootCanvas *canvas);

#endif // BOOT_CANVAS_H
