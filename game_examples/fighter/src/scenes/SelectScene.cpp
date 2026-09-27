#include "scenes/SelectScene.hpp"
#include "Game.hpp"
#include "characters/Roster.hpp"
#include "core/Config.hpp"
#include "core/Text.hpp"
#include "scenes/BattleScene.hpp"
#include "scenes/TitleScene.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace fighter {

namespace {

// --- hình học của màn hình -----------------------------------------------
// Lưới một hàng bốn ô, phân trang khi roster đông hơn - nhờ vậy bố cục không
// đổi dù thêm bao nhiêu nhân vật.
constexpr int   kCols = 4;
constexpr int   kRows = 1;
constexpr int   kPerPage = kCols * kRows;
constexpr float kCellW = 140.0f;
constexpr float kCellH = 160.0f;
constexpr float kCellGap = 10.0f;
constexpr float kGridX = 345.0f;
constexpr float kGridY = 100.0f;
constexpr float kGridW = kCols * kCellW + (kCols - 1) * kCellGap;

constexpr float kPanelW = 300.0f;
constexpr float kPanelY = 92.0f;
constexpr float kPanelH = 518.0f;

constexpr float kInfoY  = 286.0f;
constexpr float kInfoH  = 92.0f;

constexpr float kOptY   = 392.0f;
constexpr float kOptH   = 218.0f;

const char *kRoundsLabel[] = {"BO1 · thắng 1 hiệp", "BO3 · thắng 2 hiệp", "BO5 · thắng 3 hiệp"};

void Panel(Rectangle r, Color border, float alpha = 1.0f)
{
    DrawRectangleRounded(r, 0.06f, 8, Color{14, 13, 22, (unsigned char)(215 * alpha)});
    DrawRectangleRoundedLines(r, 0.06f, 8, Color{border.r, border.g, border.b,
                                                 (unsigned char)(200 * alpha)});
}

} // namespace

void SelectScene::OnEnter(Game &game)
{
    Roster &roster = Roster::Instance();
    const int n = std::max(1, roster.Count());

    cursor_[0] = std::clamp(game.Config().p1Character, 0, n - 1);
    cursor_[1] = std::clamp(game.Config().p2Character, 0, n - 1);
    if (n > 1 && cursor_[1] == cursor_[0]) cursor_[1] = (cursor_[0] + 1) % n;

    locked_[0] = locked_[1] = false;
    activeSide_ = 0;
    row_ = Row::Grid;
    page_ = PageOfIndex(cursor_[0]);
    enterFade_ = 0.0f;
    previewChar_[0] = previewChar_[1] = -1;
}

int SelectScene::PageCount() const
{
    const int n = Roster::Instance().Count();
    return std::max(1, (n + kPerPage - 1) / kPerPage);
}

int SelectScene::PageOfIndex(int index) const
{
    return std::clamp(index / kPerPage, 0, PageCount() - 1);
}

int SelectScene::CharacterAtCell(int cell) const
{
    const int index = page_ * kPerPage + cell;
    return (index < Roster::Instance().Count()) ? index : -1;
}

Rectangle SelectScene::CellRect(int slotOnPage) const
{
    const int col = slotOnPage % kCols;
    const int row = slotOnPage / kCols;
    return Rectangle{kGridX + col * (kCellW + kCellGap),
                     kGridY + row * (kCellH + kCellGap),
                     kCellW, kCellH};
}

// ---------------------------------------------------------------------------
// Điều khiển
// ---------------------------------------------------------------------------
void SelectScene::UpdateGridInput(Game &game)
{
    Roster &roster = Roster::Instance();
    const int n = roster.Count();
    if (n <= 0) return;

    int &cur = cursor_[activeSide_];

    const bool left  = IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A);
    const bool right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);

    if (left)  { cur = (cur - 1 + n) % n; page_ = PageOfIndex(cur); sideFlash_[activeSide_] = 0.25f; }
    if (right) { cur = (cur + 1) % n;     page_ = PageOfIndex(cur); sideFlash_[activeSide_] = 0.25f; }

    // Nhảy nguyên trang khi roster dài.
    if (IsKeyPressed(KEY_Q) && PageCount() > 1) {
        page_ = (page_ - 1 + PageCount()) % PageCount();
        cur = std::min(page_ * kPerPage, n - 1);
    }
    if (IsKeyPressed(KEY_E) && PageCount() > 1) {
        page_ = (page_ + 1) % PageCount();
        cur = std::min(page_ * kPerPage, n - 1);
    }

    if (IsKeyPressed(KEY_TAB)) {
        activeSide_ = 1 - activeSide_;
        page_ = PageOfIndex(cursor_[activeSide_]);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) {
        locked_[activeSide_] = true;
        sideFlash_[activeSide_] = 0.5f;
        if (!locked_[1 - activeSide_]) {
            activeSide_ = 1 - activeSide_;
            page_ = PageOfIndex(cursor_[activeSide_]);
        } else {
            row_ = Row::Start;
        }
    }

    game.Config().p1Character = cursor_[0];
    game.Config().p2Character = cursor_[1];
}

void SelectScene::UpdateOptionInput(Game &game)
{
    MatchConfig &cfg = game.Config();
    const int dir = (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) ? 1
                  : (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) ? -1 : 0;

    switch (row_) {
        case Row::Difficulty:
            if (dir != 0 && !cfg.twoPlayers) {
                int d = (int)cfg.difficulty + dir;
                const int count = (int)Difficulty::Count;
                cfg.difficulty = (Difficulty)((d + count) % count);
            }
            break;

        case Row::Rounds:
            if (dir != 0) {
                cfg.roundsToWin = std::clamp(cfg.roundsToWin + dir, 1, 3);
            }
            break;

        case Row::Mode:
            if (dir != 0) cfg.twoPlayers = !cfg.twoPlayers;
            break;

        case Row::Start:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) {
                CommitStart(game);
            }
            break;

        default:
            break;
    }
}

void SelectScene::CommitStart(Game &game)
{
    game.Config().p1Character = cursor_[0];
    game.Config().p2Character = cursor_[1];
    game.ChangeScene(std::unique_ptr<Scene>(new BattleScene()));
}

void SelectScene::Update(Game &game, float dt)
{
    time_ += dt;
    enterFade_ = std::min(1.0f, enterFade_ + dt * 3.2f);
    for (float &f : sideFlash_) f = std::max(0.0f, f - dt);

    // Đổi hàng đang điều khiển
    const int rowCount = (int)Row::Count;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        row_ = (Row)(((int)row_ + 1) % rowCount);
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        row_ = (Row)(((int)row_ + rowCount - 1) % rowCount);
    }

    if (row_ == Row::Grid) UpdateGridInput(game);
    else                   UpdateOptionInput(game);

    // Quay lui: mở khoá lần chọn gần nhất, hết thì về màn tiêu đề.
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        if (locked_[1])      { locked_[1] = false; activeSide_ = 1; row_ = Row::Grid; }
        else if (locked_[0]) { locked_[0] = false; activeSide_ = 0; row_ = Row::Grid; }
        else                 game.ChangeScene(std::unique_ptr<Scene>(new TitleScene()));
    }

    // Bấm F ở bất kỳ đâu để vào trận ngay.
    if (IsKeyPressed(KEY_F)) CommitStart(game);

    // Cập nhật ảnh xem trước của hai bên.
    Roster &roster = Roster::Instance();
    for (int side = 0; side < 2; ++side) {
        if (previewChar_[side] != cursor_[side]) {
            previewChar_[side] = cursor_[side];
            preview_[side].Play(&roster.At(cursor_[side]).Anim(AnimId::Idle), true);
        }
        preview_[side].Update(dt);
    }
}

// ---------------------------------------------------------------------------
// Vẽ
// ---------------------------------------------------------------------------
void SelectScene::DrawStatBar(const char *label, int value, float x, float y,
                              float w, Color c) const
{
    DrawText(label, Vector2{x, y}, 14.0f, Color{170, 166, 162, 230});

    const float bx  = x + 88.0f;
    const float pip = (w - 88.0f - 4.0f * 4.0f) / 5.0f;
    for (int i = 0; i < 5; ++i) {
        const Rectangle r{bx + i * (pip + 4.0f), y + 3.0f, pip, 9.0f};
        if (i < value) DrawRectangleRec(r, c);
        else           DrawRectangleRec(r, Color{48, 46, 58, 220});
    }
}

void SelectScene::DrawPortrait(Game &game, bool leftSide) const
{
    const int side = leftSide ? 0 : 1;
    Roster &roster = Roster::Instance();
    const CharacterDef &def = roster.At(cursor_[side]);

    const float x = leftSide ? 24.0f : (kCanvasWidth - kPanelW - 24.0f);
    const Rectangle panel{x, kPanelY, kPanelW, kPanelH};

    const bool active = (row_ == Row::Grid) && (activeSide_ == side);
    const Color border = locked_[side] ? def.accent
                       : active        ? Color{255, 220, 140, 255}
                                       : Color{80, 76, 92, 255};
    Panel(panel, border);

    if (active) {
        const float pulse = 0.5f + 0.5f * std::sin(time_ * 6.0f);
        DrawRectangleRoundedLines(
            Rectangle{panel.x - 3, panel.y - 3, panel.width + 6, panel.height + 6},
            0.06f, 8, Color{255, 220, 140, (unsigned char)(90 + 120 * pulse)});
    }

    // --- nhãn 1P / 2P / CPU ------------------------------------------------
    const char *tag = leftSide ? "1P" : (game.Config().twoPlayers ? "2P" : "CPU");
    DrawRectangleRounded(Rectangle{panel.x + 10, panel.y + 10, 54, 24}, 0.4f, 8, def.accentDeep);
    DrawTextBoldCentered(tag, panel.x + 37, panel.y + 14, 16.0f, Color{255, 255, 255, 235});

    if (locked_[side]) {
        const char *done = "ĐÃ CHỌN";
        DrawTextBold(done,
                     Vector2{panel.x + panel.width - TextWidthBold(done, 14.0f) - 12.0f,
                             panel.y + 15.0f},
                     14.0f, Color{140, 255, 170, 235});
    }

    // --- sprite lớn --------------------------------------------------------
    const Animation &a = def.Anim(AnimId::Idle);
    if (a.texture.id != 0) {
        const float sidePx = (float)a.texture.height;
        const float scale  = def.scale * 0.86f;
        const float px = panel.x + panel.width * 0.5f;
        const float py = panel.y + 282.0f + std::sin(time_ * 1.8f + side) * 3.0f;

        Rectangle src = a.FrameRect(preview_[side].Frame());
        if (!leftSide) src.width = -src.width;
        const float anchorX = leftSide ? def.anchor.x : (sidePx - def.anchor.x);

        DrawEllipse((int)px, (int)py + 6, 58.0f, 11.0f, Color{0, 0, 0, 70});
        DrawEllipse((int)px, (int)py + 6, 40.0f, 8.0f,  Color{0, 0, 0, 70});

        BeginScissorMode((int)panel.x + 2, (int)panel.y + 34,
                         (int)panel.width - 4, 256);
        DrawTexturePro(a.texture, src,
                       Rectangle{px - anchorX * scale, py - def.anchor.y * scale,
                                 sidePx * scale, sidePx * scale},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        EndScissorMode();
    }

    const float cx = panel.x + panel.width * 0.5f;

    // --- tên & danh hiệu ---------------------------------------------------
    DrawTextBoldCentered(def.name.c_str(), cx, panel.y + 296.0f, 28.0f, def.accent);
    DrawTextCentered(def.title.c_str(), cx, panel.y + 332.0f, 15.0f,
                     Color{186, 182, 176, 220});

    // --- chỉ số ------------------------------------------------------------
    const float sx = panel.x + 18.0f;
    const float sw = panel.width - 36.0f;
    float sy = panel.y + 362.0f;
    DrawStatBar("SỨC MẠNH",  def.stats.power,   sx, sy,         sw, def.accent);
    DrawStatBar("TỐC ĐỘ",    def.stats.speed,   sx, sy + 22.0f, sw, def.accent);
    DrawStatBar("TẦM ĐÁNH",  def.stats.range,   sx, sy + 44.0f, sw, def.accent);
    DrawStatBar("PHÒNG THỦ", def.stats.defense, sx, sy + 66.0f, sw, def.accent);

    // --- hai chiêu đặc trưng ------------------------------------------------
    float ky = panel.y + 456.0f;
    DrawLine((int)sx, (int)ky - 8, (int)(sx + sw), (int)ky - 8, Color{60, 58, 72, 220});

    char line[128];
    std::snprintf(line, sizeof(line), "L  %s", def.specialName.c_str());
    DrawText(line, Vector2{sx, ky}, 14.0f, Color{196, 192, 188, 235});
    std::snprintf(line, sizeof(line), "U  %s", def.superName.c_str());
    DrawText(line, Vector2{sx, ky + 20.0f}, 14.0f, Color{196, 192, 188, 235});
    DrawText("(chiêu cuối · cần đầy thanh Super)", Vector2{sx, ky + 40.0f}, 12.0f,
             Color{128, 126, 132, 210});
}

void SelectScene::DrawGrid(Game &game) const
{
    Roster &roster = Roster::Instance();
    const float gridCx = kGridX + kGridW * 0.5f;

    for (int cell = 0; cell < kPerPage; ++cell) {
        const Rectangle r = CellRect(cell);
        const int index = CharacterAtCell(cell);

        if (index < 0) {
            // Ô trống của trang cuối - chỗ dành sẵn cho nhân vật thêm sau này.
            DrawRectangleRounded(r, 0.1f, 8, Color{20, 19, 28, 150});
            DrawRectangleRoundedLines(r, 0.1f, 8, Color{46, 44, 56, 140});
            continue;
        }

        const CharacterDef &def = roster.At(index);
        const bool hover = (row_ == Row::Grid) && (cursor_[activeSide_] == index);

        DrawRectangleRounded(r, 0.1f, 8, Color{18, 17, 26, 238});
        DrawRectangleRounded(Rectangle{r.x, r.y, r.width, 4.0f}, 0.4f, 6, def.accent);
        DrawRectangleRoundedLines(r, 0.1f, 8,
                                  hover ? Color{255, 220, 140, 255} : Color{70, 68, 84, 255});

        // Ảnh nhân vật trong ô
        const Animation &a = def.Anim(AnimId::Idle);
        if (a.texture.id != 0) {
            const float sidePx = (float)a.texture.height;
            const float scale  = def.scale * 0.46f;
            BeginScissorMode((int)r.x + 2, (int)r.y + 6, (int)r.width - 4, (int)r.height - 38);
            DrawTexturePro(a.texture, a.FrameRect(0),
                           Rectangle{r.x + r.width * 0.5f - def.anchor.x * scale,
                                     r.y + r.height - 36.0f - def.anchor.y * scale,
                                     sidePx * scale, sidePx * scale},
                           Vector2{0.0f, 0.0f}, 0.0f,
                           hover ? WHITE : Color{196, 196, 208, 230});
            EndScissorMode();
        }

        // Tên
        DrawRectangle((int)r.x, (int)(r.y + r.height - 30.0f), (int)r.width, 30,
                      Color{10, 10, 16, 215});
        DrawTextBoldCentered(def.name.c_str(), r.x + r.width * 0.5f,
                             r.y + r.height - 25.0f, 16.0f,
                             hover ? def.accent : Color{206, 202, 198, 230});

        // Dấu cho biết bên nào đang đứng ở ô này
        if (cursor_[0] == index) {
            DrawRectangleRounded(Rectangle{r.x + 6, r.y + 10, 30, 21}, 0.4f, 6,
                                 Color{255, 190, 72, 240});
            DrawTextBoldCentered("1P", r.x + 21, r.y + 12, 14.0f, Color{30, 20, 10, 255});
        }
        if (cursor_[1] == index) {
            DrawRectangleRounded(Rectangle{r.x + r.width - 40, r.y + 10, 34, 21}, 0.4f, 6,
                                 Color{120, 200, 255, 240});
            DrawTextBoldCentered(game.Config().twoPlayers ? "2P" : "CPU",
                                 r.x + r.width - 23, r.y + 13, 12.0f, Color{10, 20, 30, 255});
        }

        if (hover) {
            const float pulse = 0.5f + 0.5f * std::sin(time_ * 7.0f);
            DrawRectangleRoundedLines(Rectangle{r.x - 3, r.y - 3, r.width + 6, r.height + 6},
                                      0.1f, 8,
                                      Color{255, 220, 140, (unsigned char)(100 + 120 * pulse)});
        }
    }

    // --- chỉ báo trang -----------------------------------------------------
    const float dotY = kGridY + kCellH + 14.0f;
    if (PageCount() > 1) {
        for (int i = 0; i < PageCount(); ++i) {
            const float dx = gridCx - (PageCount() - 1) * 10.0f + i * 20.0f;
            if (i == page_) DrawCircle((int)dx, (int)dotY, 5.0f, Color{255, 208, 120, 255});
            else            DrawCircleLines((int)dx, (int)dotY, 5.0f, Color{140, 136, 150, 200});
        }
        DrawTextCentered("Q / E đổi trang", gridCx, dotY + 10.0f, 12.0f,
                         Color{150, 146, 150, 190});
    }

    // --- mô tả nhân vật đang trỏ tới ---------------------------------------
    const CharacterDef &focus = roster.At(cursor_[activeSide_]);
    const Rectangle info{kGridX, kInfoY, kGridW, kInfoH};
    Panel(info, Color{60, 58, 72, 255});

    char skills[160];
    std::snprintf(skills, sizeof(skills), "%s   ·   %s",
                  focus.specialName.c_str(), focus.superName.c_str());
    DrawTextBoldCentered(skills, gridCx, info.y + 12.0f, 18.0f, focus.accent);

    DrawTextWrapped(focus.blurb.c_str(),
                    Rectangle{info.x + 18.0f, info.y + 42.0f, info.width - 36.0f, 44.0f},
                    14.0f, 4.0f, Color{174, 170, 168, 225}, true);
}

void SelectScene::DrawOptions(Game &game) const
{
    const MatchConfig &cfg = game.Config();
    const Rectangle panel{kGridX, kOptY, kGridW, kOptH};
    Panel(panel, Color{70, 68, 84, 255});

    const float cx = panel.x + panel.width * 0.5f;

    char roundsBuf[64];
    std::snprintf(roundsBuf, sizeof(roundsBuf), "%s",
                  kRoundsLabel[std::clamp(cfg.roundsToWin, 1, 3) - 1]);

    const DifficultyProfile &dp = GetDifficulty(cfg.difficulty);

    struct RowInfo { Row row; const char *label; std::string value; std::string note; bool enabled; };
    const RowInfo rows[] = {
        {Row::Difficulty, "ĐỘ KHÓ", dp.name,
         cfg.twoPlayers ? std::string("Chế độ 2 người không dùng máy") : std::string(dp.note),
         !cfg.twoPlayers},
        {Row::Rounds, "SỐ HIỆP", roundsBuf,
         "Bên nào thắng đủ số hiệp trước thì thắng cả trận", true},
        {Row::Mode, "CHẾ ĐỘ", cfg.twoPlayers ? "2 NGƯỜI" : "1 NGƯỜI",
         cfg.twoPlayers ? "Người 2 dùng phím mũi tên và numpad"
                        : "Máy điều khiển đối thủ theo độ khó đã chọn", true},
    };

    float y = panel.y + 14.0f;
    std::string note;
    for (const RowInfo &ri : rows) {
        const bool sel = (row_ == ri.row);
        const Rectangle rr{panel.x + 10.0f, y, panel.width - 20.0f, 38.0f};

        if (sel) {
            DrawRectangleRounded(rr, 0.3f, 8, Color{255, 200, 100, 26});
            DrawRectangleRoundedLines(rr, 0.3f, 8, Color{255, 210, 130, 200});
            note = ri.note;
        }

        const Color labelCol = ri.enabled ? Color{214, 210, 204, 240} : Color{108, 106, 114, 220};
        DrawTextBold(ri.label, Vector2{rr.x + 16.0f, y + 10.0f}, 18.0f, labelCol);

        const Color valCol = ri.enabled
                           ? (sel ? Color{255, 226, 150, 255} : Color{224, 220, 214, 240})
                           : Color{108, 106, 114, 220};
        const float valCx = panel.x + panel.width * 0.62f;
        DrawTextBoldCentered(ri.value.c_str(), valCx, y + 9.0f, 19.0f, valCol);

        if (sel && ri.enabled) {
            DrawTextBold("‹", Vector2{valCx - 118.0f, y + 7.0f}, 22.0f, Color{255, 210, 130, 255});
            DrawTextBold("›", Vector2{valCx + 106.0f, y + 7.0f}, 22.0f, Color{255, 210, 130, 255});
        }

        y += 42.0f;
    }

    // Chú thích của đúng dòng đang chọn - gọn hơn là nhét vào từng dòng.
    if (!note.empty()) {
        DrawTextCentered(note.c_str(), cx, y + 4.0f, 14.0f, Color{152, 150, 156, 220});
    }

    // --- nút bắt đầu -------------------------------------------------------
    const bool startSel = (row_ == Row::Start);
    const Rectangle btn{cx - 140.0f, panel.y + panel.height - 50.0f, 280.0f, 38.0f};
    const float pulse = 0.5f + 0.5f * std::sin(time_ * 6.0f);

    DrawRectangleRounded(btn, 0.35f, 8,
                         startSel ? Color{255, 190, 72, (unsigned char)(190 + 60 * pulse)}
                                  : Color{56, 53, 66, 235});
    DrawRectangleRoundedLines(btn, 0.35f, 8,
                              startSel ? Color{255, 240, 190, 255} : Color{96, 92, 106, 230});
    DrawTextBoldCentered("VÀO TRẬN", btn.x + btn.width * 0.5f, btn.y + 8.0f, 22.0f,
                         startSel ? Color{40, 24, 8, 255} : Color{188, 184, 180, 235});
}

void SelectScene::DrawFooter(Game &game) const
{
    (void)game;
    const char *hint =
        "↑↓ đổi mục  ·  ←→ đổi lựa chọn  ·  Tab đổi bên  ·  Enter xác nhận  ·  "
        "F vào trận ngay  ·  Esc quay lại";
    DrawTextCentered(hint, kCanvasWidth * 0.5f, kCanvasHeight - 30.0f, 15.0f,
                     Color{156, 152, 150, 210});
}

void SelectScene::Draw(Game &game)
{
    game.Backdrop().Update(time_ * 14.0f, GetFrameTime());
    game.Backdrop().Draw();
    DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight, Color{8, 7, 16, 195});

    DrawTextBoldCentered("CHỌN ĐẤU SĨ", kCanvasWidth * 0.5f, 26.0f, 34.0f,
                         Color{255, 208, 120, 255});
    DrawTextCentered("Mỗi nhân vật có bộ chiêu riêng — xem chỉ số ở hai bên",
                     kCanvasWidth * 0.5f, 66.0f, 15.0f, Color{170, 166, 164, 210});

    DrawPortrait(game, true);
    DrawPortrait(game, false);
    DrawGrid(game);
    DrawOptions(game);
    DrawFooter(game);

    // Mờ dần khi vừa vào màn.
    if (enterFade_ < 1.0f) {
        DrawRectangle(0, 0, kCanvasWidth, kCanvasHeight,
                      Color{0, 0, 0, (unsigned char)(255 * (1.0f - enterFade_))});
    }
}

} // namespace fighter
