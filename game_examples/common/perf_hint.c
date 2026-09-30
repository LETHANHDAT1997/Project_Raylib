#include "perf_hint.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool s_probed = false;
static bool s_weak = false;
static char s_model[96] = "";

static bool s_liteSet = false;
static bool s_lite = false;

// Raspberry Pi 4/5/400/500 có GPU V3D đủ sức; các đời trước dùng VC4.
static bool IsWeakPiModel(const char *model)
{
    if (!strstr(model, "Raspberry Pi")) return false;
    return !(strstr(model, " 4 ") || strstr(model, " 5 ") ||
             strstr(model, " 400") || strstr(model, " 500"));
}

static void Probe(void)
{
    if (s_probed) return;
    s_probed = true;

    FILE *f = fopen("/proc/device-tree/model", "r");
    if (f) {
        size_t n = fread(s_model, 1, sizeof(s_model) - 1, f);
        fclose(f);
        s_model[n] = '\0';   // File thường có sẵn NUL ở cuối, nhưng không phải lúc nào cũng vậy
        s_weak = IsWeakPiModel(s_model);
    }

    const char *env = getenv("RAYLIB_ARCADE_LITE");
    if (env && env[0] == '1') s_weak = true;
    if (env && env[0] == '0') s_weak = false;
}

bool PerfHintWeakDevice(void)
{
    Probe();
    return s_weak;
}

const char *PerfHintDeviceModel(void)
{
    Probe();
    return s_model;
}

bool PerfHintLite(void)
{
    return s_liteSet ? s_lite : PerfHintWeakDevice();
}

void PerfHintSetLite(bool lite)
{
    s_liteSet = true;
    s_lite = lite;
}
