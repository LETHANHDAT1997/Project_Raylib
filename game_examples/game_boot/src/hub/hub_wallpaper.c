#include "hub_wallpaper.h"
#include "ui_theme.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WALLPAPER_MAX       24
#define WALLPAPER_THUMB_W   320
#define WALLPAPER_THUMB_H   180

typedef struct {
    char id[64];
    char label[64];
    char path[512];
    Texture2D thumb;
    bool hasFile;
} Wallpaper;

// Các định dạng raylib giải mã được trong bản build này.
#define WALLPAPER_FILTER ".png;.jpg;.jpeg;.bmp;.qoi;.tga"

static Wallpaper s_list[WALLPAPER_MAX];
static int       s_count = 0;
static int       s_skipped = 0;
static int       s_current = 0;
static Texture2D s_fullTexture = {0};

// ------------------------------------------------------------- Tiện ích

// Chạy từ thư mục game_boot, từ build/, hay từ gốc dự án đều tìm ra assets.
static const char *FindWallpaperDir(void)
{
    static const char *candidates[] = {
        "assets/wallpapers",
        "../assets/wallpapers",
        "../../assets/wallpapers",
        "../../game_examples/assets/wallpapers",
        NULL
    };

    for (int i = 0; candidates[i]; i++) {
        if (DirectoryExists(candidates[i])) return candidates[i];
    }
    return NULL;
}

// "lake_cabin" -> "Lake cabin": đủ để tên file thành nhãn đọc được.
static void MakeLabel(const char *stem, char *out, size_t outSize)
{
    size_t n = 0;
    for (const char *p = stem; *p && n < outSize - 1; p++) {
        char c = *p;
        if (c == '_' || c == '-') c = ' ';
        if (n == 0 && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        out[n] = c;
        n++;
    }
    out[n] = '\0';
}

static int CompareByLabel(const void *a, const void *b)
{
    return strcmp(((const Wallpaper *)a)->label, ((const Wallpaper *)b)->label);
}

// Trả về false nếu raylib không đọc được file. Dùng luôn bước này để xác
// thực ảnh: mục nào không nạp nổi thumbnail thì cũng không nạp nổi ảnh đầy
// đủ, nên loại khỏi danh sách thay vì để người dùng bấm vào mà không có gì
// xảy ra.
static bool LoadThumbnail(Wallpaper *w)
{
    Image img = LoadImage(w->path);
    if (!IsImageValid(img)) {
        TraceLog(LOG_WARNING, "WALLPAPER: bỏ qua '%s' - raylib không giải mã được định dạng này", w->path);
        return false;
    }

    ImageResize(&img, WALLPAPER_THUMB_W, WALLPAPER_THUMB_H);
    w->thumb = LoadTextureFromImage(img);
    UnloadImage(img);

    if (!IsTextureValid(w->thumb)) return false;

    SetTextureFilter(w->thumb, TEXTURE_FILTER_BILINEAR);
    return true;
}

static void UnloadFullTexture(void)
{
    if (IsTextureValid(s_fullTexture)) UnloadTexture(s_fullTexture);
    s_fullTexture = (Texture2D){0};
}

static void LoadFullTexture(int index)
{
    UnloadFullTexture();
    if (index <= 0 || index >= s_count || !s_list[index].hasFile) return;

    s_fullTexture = LoadTexture(s_list[index].path);
    if (IsTextureValid(s_fullTexture)) {
        SetTextureFilter(s_fullTexture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(s_fullTexture, TEXTURE_WRAP_CLAMP);
    }
}

// ------------------------------------------------------------- Vòng đời

void HubWallpaperInit(const char *selectedId)
{
    HubWallpaperShutdown();

    // Mục 0 luôn là gradient động dựng bằng mã, không phụ thuộc file nào.
    snprintf(s_list[0].id, sizeof(s_list[0].id), "%s", WALLPAPER_GRADIENT_ID);
    snprintf(s_list[0].label, sizeof(s_list[0].label), "Gradient động");
    s_list[0].hasFile = false;
    s_count = 1;

    const char *dir = FindWallpaperDir();
    if (dir) {
        FilePathList files = LoadDirectoryFilesEx(dir, WALLPAPER_FILTER, false);
        for (unsigned int i = 0; i < files.count && s_count < WALLPAPER_MAX; i++) {
            Wallpaper *w = &s_list[s_count];
            memset(w, 0, sizeof(*w));
            snprintf(w->path, sizeof(w->path), "%s", files.paths[i]);
            snprintf(w->id, sizeof(w->id), "%s", GetFileNameWithoutExt(files.paths[i]));
            MakeLabel(w->id, w->label, sizeof(w->label));
            w->hasFile = true;

            // Chỉ giữ lại ảnh thật sự mở được.
            if (LoadThumbnail(w)) s_count++;
            else s_skipped++;
        }
        UnloadDirectoryFiles(files);

        // Sắp xếp để thứ tự hiển thị ổn định giữa các lần chạy.
        if (s_count > 2) qsort(&s_list[1], (size_t)(s_count - 1), sizeof(Wallpaper), CompareByLabel);
    } else {
        TraceLog(LOG_INFO, "WALLPAPER: không tìm thấy thư mục assets/wallpapers, dùng gradient động");
    }

    int wanted = HubWallpaperIndexOfId(selectedId);
    HubWallpaperSelect((wanted >= 0) ? wanted : 0);
}

void HubWallpaperShutdown(void)
{
    UnloadFullTexture();
    for (int i = 0; i < s_count; i++) {
        if (IsTextureValid(s_list[i].thumb)) UnloadTexture(s_list[i].thumb);
    }
    memset(s_list, 0, sizeof(s_list));
    s_count = 0;
    s_current = 0;
    s_skipped = 0;
}

// ---------------------------------------------------------------- Truy vấn

int HubWallpaperCount(void) { return s_count; }

int HubWallpaperSkippedCount(void) { return s_skipped; }

const char *HubWallpaperId(int index)
{
    return (index >= 0 && index < s_count) ? s_list[index].id : WALLPAPER_GRADIENT_ID;
}

const char *HubWallpaperLabel(int index)
{
    return (index >= 0 && index < s_count) ? s_list[index].label : "";
}

Texture2D HubWallpaperThumbnail(int index)
{
    return (index >= 0 && index < s_count) ? s_list[index].thumb : (Texture2D){0};
}

int HubWallpaperIndexOfId(const char *id)
{
    if (!id || !id[0]) return -1;
    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_list[i].id, id) == 0) return i;
    }
    return -1;
}

int HubWallpaperCurrent(void) { return s_current; }

void HubWallpaperSelect(int index)
{
    if (index < 0 || index >= s_count) index = 0;
    if (index == s_current && (index == 0 || IsTextureValid(s_fullTexture))) return;

    s_current = index;
    LoadFullTexture(index);

    // Nếu ảnh hỏng hoặc thiếu thì lùi về gradient thay vì để nền đen.
    if (index > 0 && !IsTextureValid(s_fullTexture)) s_current = 0;
}

// -------------------------------------------------------------------- Vẽ

bool HubWallpaperDraw(int width, int height, float time, Color accent, bool reduceMotion)
{
    (void)accent;   // Ảnh nền giữ nguyên màu gốc, không nhuộm theo game đang chọn
    if (s_current <= 0 || !IsTextureValid(s_fullTexture)) return false;

    float w = (float)width;
    float h = (float)height;
    float texW = (float)s_fullTexture.width;
    float texH = (float)s_fullTexture.height;

    // Phủ kín khung theo kiểu "cover": cạnh ngắn vừa khít, cạnh dài bị cắt bớt.
    float zoom = 1.04f;
    float panX = 0.0f, panY = 0.0f;
    if (!reduceMotion) {
        // Ken Burns rất chậm: nền như đang thở chứ không gây mất tập trung.
        zoom += 0.025f * (0.5f + 0.5f * sinf(time * 0.045f));
        panX = sinf(time * 0.031f) * w * 0.012f;
        panY = cosf(time * 0.024f) * h * 0.010f;
    }

    float scale = fmaxf(w / texW, h / texH) * zoom;
    float destW = texW * scale;
    float destH = texH * scale;

    Rectangle dest = {
        (w - destW) * 0.5f + panX,
        (h - destH) * 0.5f + panY,
        destW, destH
    };
    DrawTexturePro(s_fullTexture, (Rectangle){0.0f, 0.0f, texW, texH}, dest,
                   (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

    // Lớp phủ giữ ở mức gần như không thấy, và dùng đen thuần.
    //
    // Bản trước pha màu xanh đậm cộng thêm một lớp màu nhấn, khiến ảnh nền
    // trắng bị kéo thành xám ngả xanh. Giờ không còn cần lớp phủ để chữ dễ
    // đọc nữa: chính vật liệu kính đã chuẩn hoá độ sáng, mà mọi chữ trong
    // giao diện đều nằm trên kính chứ không nằm trực tiếp trên ảnh nền.
    DrawRectangleGradientV(0, 0, width, height,
                           (Color){0, 0, 0, 3}, (Color){0, 0, 0, 14});

    // Tối rất nhẹ bốn góc cho khung kính ở giữa nổi lên; xám trung tính
    // để không nhuộm màu cho ảnh.
    float unit = fminf(w, h);
    BeginBlendMode(BLEND_MULTIPLIED);
        DrawCircleGradient((Vector2){w * 0.5f, h * 0.5f}, unit * 1.45f,
                           (Color){255, 255, 255, 255}, (Color){243, 243, 243, 255});
    EndBlendMode();

    return true;
}
