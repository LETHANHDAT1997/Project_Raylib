#include "hub_screen.h"
#include "hub_background.h"
#include "hub_wallpaper.h"
#include "hub_topbar.h"
#include "hub_sidebar.h"
#include "hub_hero.h"
#include "hub_info.h"
#include "hub_pages.h"

#include "boot_app.h"
#include "boot_settings.h"
#include "boot_input.h"
#include "ui_theme.h"
#include "ui_anim.h"
#include "ui_glass.h"
#include "ui_widgets.h"
#include "ui_focus.h"

#include <math.h>

// ------------------------------------------------------------- Bố cục
//
// Toàn bộ giao diện nằm trong một "cửa sổ kính" bo góc lớn, giống cách macOS
// đặt một tấm kính lên trên hình nền. Các hằng số dưới đây định nghĩa lưới
// duy nhất mà mọi thành phần con bám theo.

#define WIN_MARGIN_X   44.0f
#define WIN_MARGIN_Y   28.0f
#define WIN_PAD        18.0f
#define TOPBAR_HEIGHT  52.0f
#define GRID_GAP       18.0f
#define SIDEBAR_WIDTH  330.0f
#define HERO_HEIGHT    350.0f

static HubContext s_ctx;
static Color s_animAccent = {126, 184, 255, 255};

static Rectangle WindowRect(void)
{
    return (Rectangle){WIN_MARGIN_X, WIN_MARGIN_Y,
                       HUB_VIRTUAL_WIDTH - WIN_MARGIN_X * 2.0f,
                       HUB_VIRTUAL_HEIGHT - WIN_MARGIN_Y * 2.0f};
}

static Rectangle ContentRect(void)
{
    Rectangle w = WindowRect();
    return (Rectangle){w.x + WIN_PAD, w.y + WIN_PAD,
                       w.width - WIN_PAD * 2.0f, w.height - WIN_PAD * 2.0f};
}

static Rectangle TopbarRect(void)
{
    Rectangle c = ContentRect();
    return (Rectangle){c.x, c.y, c.width, TOPBAR_HEIGHT};
}

static Rectangle BodyRect(void)
{
    Rectangle c = ContentRect();
    float top = c.y + TOPBAR_HEIGHT + GRID_GAP;
    return (Rectangle){c.x, top, c.width, c.y + c.height - top};
}

static Rectangle SidebarRect(void)
{
    Rectangle b = BodyRect();
    return (Rectangle){b.x, b.y, SIDEBAR_WIDTH, b.height};
}

static Rectangle HeroRect(void)
{
    Rectangle b = BodyRect();
    float x = b.x + SIDEBAR_WIDTH + GRID_GAP;
    return (Rectangle){x, b.y, b.x + b.width - x, HERO_HEIGHT};
}

static Rectangle InfoRect(void)
{
    Rectangle b = BodyRect();
    Rectangle h = HeroRect();
    float y = h.y + h.height + GRID_GAP;
    return (Rectangle){h.x, y, h.width, b.y + b.height - y};
}

// --------------------------------------------------------------- Vòng đời

void HubScreenInit(BootApp *app)
{
    UiGlassInit(HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT);
    UiGlassSetBlurAmount(BootSettingsGet()->glassBlur);
    UiGlassSetLevelScale(GlassLevelScale(BootSettingsGet()->glassTint));
    HubWallpaperInit(BootSettingsGet()->wallpaperId);

    const GameEntry *g = GameRegistryGet(app->selectedGame);
    if (g) s_animAccent = g->accent;
}

void HubScreenClose(void)
{
    HubHeroRelease();
    HubWallpaperShutdown();
    UiGlassShutdown();
}

// Dựng lại "bảng thông tin" mà mọi thành phần con đọc chung.
static void RefreshContext(BootApp *app)
{
    s_ctx.app = app;
    s_ctx.gameIndex = app->selectedGame;
    s_ctx.game = GameRegistryGet(app->selectedGame);
    s_ctx.time = app->globalTime;
    s_ctx.accent = s_animAccent;
    s_ctx.window = WindowRect();
    s_ctx.content = ContentRect();
    s_ctx.reduceMotion = BootSettingsGet()->reduceMotion;
}

// ----------------------------------------------------------------- Nhập

// Điều hướng bằng bàn phím.
//
// Trang Thư viện tự xử lý Lên/Xuống để đổi game đang chọn - danh sách game
// chính là "vòng focus" của nó. Các trang còn lại giao hẳn cho ui_focus:
// mọi widget ở đó tự đăng ký vùng của mình nên điều hướng đi đúng theo bố
// cục thật mà không cần khai báo thứ tự ở đây.
static void HandleNavigation(BootApp *app)
{
    int count = GameRegistryCount();

    if (BootInputPressed(BOOT_ACTION_NEXT_TAB)) {
        app->tab = (HubTab)((app->tab + 1) % HUB_TAB_COUNT);
    }
    if (BootInputPressed(BOOT_ACTION_PREV_TAB)) {
        app->tab = (HubTab)((app->tab + HUB_TAB_COUNT - 1) % HUB_TAB_COUNT);
    }

    bool library = (app->tab == HUB_TAB_LIBRARY);
    UiFocusSetEnabled(!library);

    if (!library) {
        if (BootInputPressed(BOOT_ACTION_BACK)) app->tab = HUB_TAB_LIBRARY;

        UiFocusSetNav(BootInputPressed(BOOT_ACTION_NAV_UP),
                      BootInputPressed(BOOT_ACTION_NAV_DOWN),
                      BootInputPressed(BOOT_ACTION_NAV_LEFT),
                      BootInputPressed(BOOT_ACTION_NAV_RIGHT),
                      BootInputPressed(BOOT_ACTION_ACTIVATE));
        return;
    }

    if (count <= 0) return;

    if (BootInputPressed(BOOT_ACTION_NAV_DOWN)) {
        app->selectedGame = (app->selectedGame + 1) % count;
    }
    if (BootInputPressed(BOOT_ACTION_NAV_UP)) {
        app->selectedGame = (app->selectedGame - 1 + count) % count;
    }

    int digit = BootInputDigitPressed();
    if (digit > 0 && digit <= count) app->selectedGame = digit - 1;

    if (BootInputPressed(BOOT_ACTION_ACTIVATE)) {
        BootAppRequestGame(app, app->selectedGame);
    }
}

void HubScreenUpdate(BootApp *app, float dt)
{
    HandleNavigation(app);

    const GameEntry *g = GameRegistryGet(app->selectedGame);
    Color target = g ? g->accent : UI.accent;
    s_animAccent = UiApproachColor(s_animAccent, target, 6.0f, dt);

    RefreshContext(app);
    HubBackgroundUpdate(s_animAccent, dt, s_ctx.reduceMotion);

    if (app->tab == HUB_TAB_LIBRARY) {
        HubSidebarUpdate(&s_ctx, SidebarRect(), dt);
    }
}

// --------------------------------------------------------------- Dựng nền
//
// Bước này chạy TRƯỚC khi mở canvas chính vì raylib không cho lồng render
// target: nền và tranh hero đều phải được vẽ vào texture riêng trước.

void HubScreenPrepare(BootApp *app)
{
    RefreshContext(app);

    UiGlassBeginBackdrop();
        HubBackgroundDraw(HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT, app->globalTime);
    UiGlassEndBackdrop();

    if (app->tab == HUB_TAB_LIBRARY) {
        HubHeroPrepare(&s_ctx, HeroRect());
    }
}

// -------------------------------------------------------------------- Vẽ

void HubScreenDraw(BootApp *app)
{
    RefreshContext(app);

    // Danh sách widget có thể focus được dựng lại mỗi khung hình trong lúc
    // vẽ; đổi tab thì scope đổi theo và focus quay về mục đầu tiên.
    UiFocusBeginFrame((int)app->tab);

    // 1. Hình nền. Vẽ lại (thay vì phóng to texture nền đã dựng ở bước
    // Prepare) để nền được rasterise đúng độ phân giải cửa sổ; texture kia
    // chỉ dùng làm nguồn lấy mẫu cho lớp mờ của kính.
    HubBackgroundDraw(HUB_VIRTUAL_WIDTH, HUB_VIRTUAL_HEIGHT, app->globalTime);

    // 2. Cửa sổ kính ngoài cùng
    Rectangle win = s_ctx.window;
    UiGlassShadow(win, UI_RADIUS_XL, 24.0f, 0.26f);
    UiGlassPanel(win, UI_RADIUS_XL, UiGlassStyleWindow());
    UiGlassStroke(win, UI_RADIUS_XL, 1.4f, UI.strokeSoft);

    // 3. Thanh điều hướng
    HubTopbarDraw(&s_ctx, TopbarRect());

    // 4. Nội dung theo tab đang mở
    switch (app->tab) {
        case HUB_TAB_LIBRARY:
            HubSidebarDraw(&s_ctx, SidebarRect());
            HubHeroDraw(&s_ctx, HeroRect());
            HubInfoDraw(&s_ctx, InfoRect());
            // Menu ngữ cảnh vẽ sau cùng để nằm trên mọi thẻ khác.
            HubHeroDrawOverlay(&s_ctx, HeroRect());
            break;

        case HUB_TAB_CONTROLS:
            HubControlsPageDraw(&s_ctx, BodyRect());
            break;

        case HUB_TAB_SETTINGS:
            HubSettingsPageDraw(&s_ctx, BodyRect());
            break;

        default:
            break;
    }

    UiFocusEndFrame();
}
