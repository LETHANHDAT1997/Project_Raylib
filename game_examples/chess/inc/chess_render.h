/**
 * chess_render.h - Cảnh 3D của Cờ Vua: bộ cờ đá cẩm thạch Poly Haven (CC0),
 * ánh sáng PBR, bóng đổ shadow map và khử răng cưa siêu lấy mẫu.
 *
 * Cảnh được dựng vào một render texture riêng ở bước Update (ngoài mọi
 * BeginTextureMode) vì bóng đổ cần đổi framebuffer - việc không làm được khi
 * đang vẽ vào canvas ảo của game. Bước Draw chỉ việc dán ảnh đã dựng lên canvas.
 */
#ifndef CHESS_RENDER_H
#define CHESS_RENDER_H

#include "chess_types.h"

#define CHESS_SQUARE_SIZE 1.0f      // Một ô cờ = 1 đơn vị thế giới

void ChessRenderInit(void);         // Có đếm tham chiếu, gọi lặp lại vô hại
void ChessRenderClose(void);
bool ChessRenderHasModel(void);     // false nếu thiếu file mô hình (đang dùng hình dự phòng)

void ChessRenderSetQuality(bool high);
void ChessRenderScene(const ChessGame *game);     // Gọi NGOÀI BeginTextureMode
void ChessRenderBlit(void);                       // Dán cảnh đã dựng lên canvas hiện tại

// Hình học
Vector3 ChessSquareWorld(int sq);
Vector3 ChessGraveWorld(int color, int index, int viewerColor);
Camera3D ChessCameraFromRig(const ChessCameraRig *rig);
Ray     ChessScreenRay(const ChessGame *game, Vector2 canvasPos);
Vector2 ChessWorldToCanvas(const ChessGame *game, Vector3 world, bool *visible);
int     ChessPickSquare(const ChessGame *game, Vector2 canvasPos);   // -1 nếu không trúng ô nào
float   ChessBoardTop(void);

#endif // CHESS_RENDER_H
