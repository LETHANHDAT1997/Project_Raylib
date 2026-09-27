#pragma once
#include "raylib.h"
#include "core/Animation.hpp"
#include "scenes/Scene.hpp"
#include <vector>

namespace fighter {

// ============================================================================
// SelectScene - màn "Settings" mà đề bài yêu cầu: chọn nhân vật cho hai bên,
// chọn độ khó, số hiệp và chế độ chơi.
//
// Lưới nhân vật tự chia trang theo Roster::Count() nên roster có 4 hay 40 nhân
// vật thì màn này vẫn chạy đúng, chỉ khác số trang.
// ============================================================================
class SelectScene final : public Scene {
public:
    void OnEnter(Game &game) override;
    void Update(Game &game, float dt) override;
    void Draw(Game &game) override;

private:
    // Hàng nào đang được điều khiển bằng phím lên/xuống
    enum class Row { Grid = 0, Difficulty, Rounds, Mode, Start, Count };

    void UpdateGridInput(Game &game);
    void UpdateOptionInput(Game &game);
    void CommitStart(Game &game);

    void DrawGrid(Game &game) const;
    void DrawPortrait(Game &game, bool leftSide) const;
    void DrawOptions(Game &game) const;
    void DrawFooter(Game &game) const;
    void DrawStatBar(const char *label, int value, float x, float y, float w, Color c) const;

    Rectangle CellRect(int slotOnPage) const;
    int  PageCount() const;
    int  PageOfIndex(int index) const;
    int  CharacterAtCell(int cell) const;   // -1 nếu ô trống

    // Con trỏ của hai bên trên lưới
    int  cursor_[2] = {0, 1};
    bool locked_[2] = {false, false};
    int  activeSide_ = 0;      // bên nào đang chọn (chế độ 1 người chỉ dùng 0)
    int  page_ = 0;

    Row  row_ = Row::Grid;
    float time_ = 0.0f;
    float enterFade_ = 0.0f;
    float sideFlash_[2] = {0.0f, 0.0f};

    Animator preview_[2];
    int previewChar_[2] = {-1, -1};
};

} // namespace fighter
