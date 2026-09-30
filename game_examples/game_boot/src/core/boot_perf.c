#include "boot_perf.h"
#include "boot_settings.h"
#include "perf_hint.h"
#include "raylib.h"

// Bỏ qua vài khung đầu (nạp tài nguyên, cửa sổ đang dựng) rồi đo trong
// khoảng này. Ngưỡng thấp hơn hẳn 30 để màn hình 30 Hz không bị bắt nhầm.
#define PERF_SKIP_FRAMES   3
#define PERF_MEASURE_SECS  2.0f
#define PERF_MIN_FPS       24.0f

static bool  s_weakDevice = false;

static bool  s_measureDone = false;
static bool  s_slowMeasured = false;
static int   s_skipped = 0;
static int   s_frames = 0;
static float s_elapsed = 0.0f;

void BootPerfInit(void)
{
    s_weakDevice = PerfHintWeakDevice();
    if (s_weakDevice) {
        TraceLog(LOG_INFO, "PERF: máy yếu ('%s'), dùng đồ hoạ Nhẹ ở chế độ Tự động",
                 PerfHintDeviceModel());
    }
}

bool BootPerfWeakDevice(void)
{
    return s_weakDevice;
}

void BootPerfSample(float frameTime)
{
    if (s_measureDone) return;
    if (s_skipped < PERF_SKIP_FRAMES) {
        s_skipped++;
        return;
    }

    s_frames++;
    s_elapsed += frameTime;
    if (s_elapsed < PERF_MEASURE_SECS) return;

    float fps = (float)s_frames / s_elapsed;
    s_slowMeasured = (fps < PERF_MIN_FPS);
    s_measureDone = true;

    if (s_slowMeasured) {
        TraceLog(LOG_WARNING, "PERF: Hub chỉ đạt %.1f FPS, chuyển sang đồ hoạ Nhẹ", fps);
    }
}

void BootPerfResetMeasure(void)
{
    s_measureDone = false;
    s_slowMeasured = false;
    s_skipped = 0;
    s_frames = 0;
    s_elapsed = 0.0f;
}

bool BootPerfLite(void)
{
    switch (BootSettingsGet()->graphicsMode) {
        case BOOT_GFX_QUALITY: return false;
        case BOOT_GFX_LITE:    return true;
        default:               return s_weakDevice || s_slowMeasured;
    }
}

const char *BootPerfReason(void)
{
    if (s_weakDevice)   return "máy yếu (Raspberry Pi 0-3)";
    if (s_slowMeasured) return "đo được FPS thấp";
    return "";
}
