#include "boot_settings.h"
#include "raylib.h"
#include "save_data.h"
#include <stdio.h>
#include <string.h>

#define SETTINGS_FILE SaveDataFilePath("game_boot_settings.txt")

static BootSettings s_settings = {
    .glassBlur = 0.62f,
    .glassTint = 0.80f,
    .graphicsMode = BOOT_GFX_AUTO,
    .showFps = false,
    .reduceMotion = false,
    .masterVolume = 0.7f,
    .wallpaperId = "lake_cabin"
};

// Bản cũ ghi file ngay tại thư mục đang chạy Hub: chép nguyên nội dung sang
// thư mục dữ liệu chung (chỉ khi bên đó chưa có file).
static void ImportLegacySettings(const char *legacyPath)
{
    if (FileExists(SETTINGS_FILE)) return;
    char *text = LoadFileText(legacyPath);
    if (!text) return;
    SaveFileText(SETTINGS_FILE, text);
    UnloadFileText(text);
}

void BootSettingsLoad(void)
{
    SaveDataMigrateLegacy("game_boot_settings.txt", ImportLegacySettings);
    if (!FileExists(SETTINGS_FILE)) return;

    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;

    char key[64];
    float value = 0.0f;
    char line[160];
    while (fgets(line, sizeof(line), f)) {
        // Hình nền là giá trị chuỗi, xử lý trước khi thử đọc kiểu số.
        char text[BOOT_WALLPAPER_ID_MAX];
        if (sscanf(line, "wallpaper %63s", text) == 1) {
            snprintf(s_settings.wallpaperId, sizeof(s_settings.wallpaperId), "%s", text);
            continue;
        }
        if (sscanf(line, "%63s %f", key, &value) != 2) continue;

        if      (strcmp(key, "glass_blur") == 0)    s_settings.glassBlur = value;
        else if (strcmp(key, "glass_tint") == 0)    s_settings.glassTint = value;
        // Khoá cũ từ thời hiệu ứng kính chỉ có hai mức: quy đổi sang thang liên tục.
        else if (strcmp(key, "glass_quality") == 0) {
            s_settings.glassBlur = (value >= 1.0f) ? 0.62f : 0.18f;
            s_settings.glassTint = (value >= 1.0f) ? 0.80f : 0.65f;
        }
        else if (strcmp(key, "graphics_mode") == 0) s_settings.graphicsMode = (int)value;
        else if (strcmp(key, "show_fps") == 0)      s_settings.showFps = (value >= 0.5f);
        else if (strcmp(key, "reduce_motion") == 0) s_settings.reduceMotion = (value >= 0.5f);
        else if (strcmp(key, "master_volume") == 0) s_settings.masterVolume = value;
    }
    fclose(f);

    // Kẹp mọi giá trị về khoảng hợp lệ phòng khi file bị sửa tay.
    if (s_settings.masterVolume < 0.0f) s_settings.masterVolume = 0.0f;
    if (s_settings.masterVolume > 1.0f) s_settings.masterVolume = 1.0f;
    if (s_settings.glassBlur < 0.0f) s_settings.glassBlur = 0.0f;
    if (s_settings.glassBlur > 1.0f) s_settings.glassBlur = 1.0f;
    if (s_settings.glassTint < 0.0f) s_settings.glassTint = 0.0f;
    if (s_settings.glassTint > 1.0f) s_settings.glassTint = 1.0f;
    if (s_settings.graphicsMode < 0 || s_settings.graphicsMode >= BOOT_GFX_COUNT) {
        s_settings.graphicsMode = BOOT_GFX_AUTO;
    }
}

void BootSettingsSave(void)
{
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return;

    fprintf(f, "glass_blur %.3f\n", s_settings.glassBlur);
    fprintf(f, "glass_tint %.3f\n", s_settings.glassTint);
    fprintf(f, "graphics_mode %d\n", s_settings.graphicsMode);
    fprintf(f, "show_fps %d\n", s_settings.showFps ? 1 : 0);
    fprintf(f, "reduce_motion %d\n", s_settings.reduceMotion ? 1 : 0);
    fprintf(f, "master_volume %.3f\n", s_settings.masterVolume);
    fprintf(f, "wallpaper %s\n", s_settings.wallpaperId);
    fclose(f);
}

BootSettings *BootSettingsGet(void)
{
    return &s_settings;
}

void BootSettingsApply(void)
{
    if (IsAudioDeviceReady()) SetMasterVolume(s_settings.masterVolume);
}
