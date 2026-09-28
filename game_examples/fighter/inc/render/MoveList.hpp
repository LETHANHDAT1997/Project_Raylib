#pragma once
#include "raylib.h"

namespace fighter {

struct CharacterDef;

// Bảng chiêu của một nhân vật: 4 chiêu đặc biệt + siêu chiêu (lấy từ
// CharacterDef::moveList) và các thao tác hệ thống chung cho mọi nhân vật.
void DrawMoveList(const CharacterDef &def, Rectangle area, float time);

} // namespace fighter
