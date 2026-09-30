#include "game_registry.h"
#include "hub_art.h"

#include "tetris_runner.h"
#include "space_runner.h"
#include "snake_runner.h"
#include "flappy_runner.h"
#include "fighter_runner.h"

#include <string.h>

// Toàn bộ thư viện game của launcher nằm trong đúng một mảng.
// Thêm game mới = thêm một khối dữ liệu ở đây, không đụng tới UI.
static const GameEntry s_games[] = {
    {
        .id = "tetris",
        .title = "Tetris",
        .platform = "PC · Raylib",
        .tagline = "Xếp từng khối gạch rơi xuống thành hàng hoàn chỉnh, càng lên cấp tốc độ càng dồn dập.",
        .developer = "Raylib Arcade",
        .releaseDate = "Bản dựng 2024",
        .engine = "Raylib 6.x",
        .tags = {"Giải đố", "Cổ điển", "Vô tận"},
        .tagCount = 3,
        .controls = {
            "← →  Di chuyển khối",
            "↑  Xoay khối",
            "↓  Rơi nhanh",
            "Space  Thả rơi tức thì",
            "C  Giữ khối (Hold)",
            "P  Tạm dừng"
        },
        .controlCount = 6,
        .recordLabel = "Kỷ lục",
        .canvasWidth = 780,
        .canvasHeight = 740,
        .accent     = (Color){126, 196, 255, 255},
        .accentDeep = (Color){52, 108, 200, 255},
        .drawIcon = HubArtTetrisIcon,
        .drawHeroArt = HubArtTetrisHero,
        .init = InitTetrisApp, .update = UpdateTetrisApp,
        .draw = DrawTetrisApp, .close = CloseTetrisApp
    },
    {
        .id = "space_invader",
        .title = "Space Invader",
        .platform = "PC · Raylib",
        .tagline = "Chặn đứng đội hình ngoài hành tinh đang tiến xuống, nấp sau boongke và bắn hạ đĩa bay bí ẩn.",
        .developer = "Raylib Arcade",
        .releaseDate = "Bản dựng 2024",
        .engine = "Raylib 6.x",
        .tags = {"Bắn súng", "Arcade", "Phản xạ"},
        .tagCount = 3,
        .controls = {
            "← →  Di chuyển pháo",
            "Space  Bắn (giữ để bắn liên tục)",
            "P  Tạm dừng",
            "R  Chơi lại"
        },
        .controlCount = 4,
        .recordLabel = "Kỷ lục",
        .canvasWidth = 800,
        .canvasHeight = 880,
        .accent     = (Color){122, 232, 176, 255},
        .accentDeep = (Color){28, 138, 108, 255},
        .drawIcon = HubArtSpaceIcon,
        .drawHeroArt = HubArtSpaceHero,
        .init = InitSpaceApp, .update = UpdateSpaceApp,
        .draw = DrawSpaceApp, .close = CloseSpaceApp
    },
    {
        .id = "snake",
        .title = "Snake",
        .platform = "PC · Raylib",
        .tagline = "Săn mồi để dài ra mà không tự cắn đuôi mình, với chế độ xuyên tường cho người thích mạo hiểm.",
        .developer = "Raylib Arcade",
        .releaseDate = "Bản dựng 2024",
        .engine = "Raylib 6.x",
        .tags = {"Cổ điển", "Kỹ năng", "Điểm cao"},
        .tagCount = 3,
        .controls = {
            "← ↑ → ↓  Đổi hướng",
            "W A S D  Đổi hướng (thay thế)",
            "P  Tạm dừng",
            "R  Chơi lại"
        },
        .controlCount = 4,
        .recordLabel = "Kỷ lục",
        .canvasWidth = 960,
        .canvasHeight = 720,
        .accent     = (Color){148, 226, 136, 255},
        .accentDeep = (Color){56, 142, 76, 255},
        .drawIcon = HubArtSnakeIcon,
        .drawHeroArt = HubArtSnakeHero,
        .init = InitSnakeApp, .update = UpdateSnakeApp,
        .draw = DrawSnakeApp, .close = CloseSnakeApp,
        .migrateSave = MigrateSnakeSave
    },
    {
        .id = "flappy",
        .title = "Flappy Plane",
        .platform = "PC · Raylib",
        .tagline = "Vỗ cánh luồn qua khe giữa những mỏm đá nhọn, nhặt sao thưởng "
                   "và băng qua năm vùng địa hình để giành huy chương vàng.",
        .developer = "Raylib Arcade",
        .releaseDate = "Bản dựng 2026",
        .engine = "Raylib 6.x",
        .tags = {"Một chạm", "Phản xạ", "Điểm cao"},
        .tagCount = 3,
        .controls = {
            "Space / ↑ / Click  Vỗ cánh",
            "← →  Chọn máy bay (menu)",
            "↑ ↓ / 1-3  Độ khó (menu)",
            "P / Esc  Tạm dừng",
            "R  Chơi lại · Q  Về menu",
            "M  Bật / tắt âm thanh"
        },
        .controlCount = 6,
        .recordLabel = "Kỷ lục",
        .canvasWidth = 1280,
        .canvasHeight = 768,
        .accent     = (Color){255, 206, 92, 255},
        .accentDeep = (Color){206, 128, 36, 255},
        .drawIcon = HubArtFlappyIcon,
        .drawHeroArt = HubArtFlappyHero,
        .init = InitFlappyApp, .update = UpdateFlappyApp,
        .draw = DrawFlappyApp, .close = CloseFlappyApp,
        .migrateSave = MigrateFlappySave
    },
    {
        .id = "fighter",
        .title = "Đấu Sĩ",
        .platform = "PC · Raylib",
        .tagline = "Đối kháng kiểu Street Fighter: lệnh quay tay, combo hủy đòn, "
                   "20 chiêu đặc biệt, siêu chiêu và chế độ luyện tập.",
        .developer = "Raylib Arcade",
        .releaseDate = "Bản dựng 2026",
        .engine = "Raylib 6.x · C++17",
        .tags = {"Đối kháng", "Combo", "2 người", "Luyện tập"},
        .tagCount = 4,
        .controls = {
            "WASD hoặc mũi tên  Đi · nhảy · ngồi",
            "Giữ lùi  Đỡ đòn (↙ đỡ thấp)",
            "J K L (hoặc Z X C)  Nhẹ · vừa · mạnh",
            "↓↘→ / →↓↘ / ↓↙← + đòn  Chiêu",
            "I  Chiêu nhanh · U  Siêu chiêu",
            "J+K  Vật · P  Tạm dừng / bảng chiêu"
        },
        .controlCount = 6,
        .canvasWidth = 1280,
        .canvasHeight = 720,
        .accent     = (Color){255, 148, 142, 255},
        .accentDeep = (Color){178, 52, 62, 255},
        .drawIcon = HubArtFighterIcon,
        .drawHeroArt = HubArtFighterHero,
        .init = InitFighterApp, .update = UpdateFighterApp,
        .draw = DrawFighterApp, .close = CloseFighterApp
    }
};

static const int s_gameCount = (int)(sizeof(s_games) / sizeof(s_games[0]));

int GameRegistryCount(void)
{
    return s_gameCount;
}

const GameEntry *GameRegistryGet(int index)
{
    if (index < 0 || index >= s_gameCount) return NULL;
    return &s_games[index];
}

int GameRegistryIndexOfId(const char *id)
{
    if (!id) return -1;
    for (int i = 0; i < s_gameCount; i++) {
        if (strcmp(s_games[i].id, id) == 0) return i;
    }
    return -1;
}
