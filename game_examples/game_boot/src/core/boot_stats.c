#include "boot_stats.h"
#include "game_registry.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define STATS_FILE     "game_boot_stats.txt"
#define STATS_VERSION  1

static GameStats s_stats[BOOT_MAX_GAMES];
static double    s_sessionStart[BOOT_MAX_GAMES];
static bool      s_dirty = false;

static bool ValidIndex(int i)
{
    return (i >= 0 && i < GameRegistryCount() && i < BOOT_MAX_GAMES);
}

// Lưu ở dạng văn bản khoá-giá trị theo id game: thêm/bớt game trong registry
// không làm hỏng dữ liệu cũ, và file vẫn đọc được bằng mắt khi cần gỡ lỗi.
void BootStatsLoad(void)
{
    memset(s_stats, 0, sizeof(s_stats));
    memset(s_sessionStart, 0, sizeof(s_sessionStart));

    if (!FileExists(STATS_FILE)) return;

    FILE *f = fopen(STATS_FILE, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char id[64];
        double total = 0.0, last = 0.0;
        long when = 0;
        int sessions = 0;

        if (sscanf(line, "%63s %lf %lf %ld %d", id, &total, &last, &when, &sessions) == 5) {
            int idx = GameRegistryIndexOfId(id);
            if (ValidIndex(idx)) {
                s_stats[idx].totalSeconds   = total;
                s_stats[idx].lastSeconds    = last;
                s_stats[idx].lastPlayedUnix = when;
                s_stats[idx].sessions       = sessions;
            }
        }
    }
    fclose(f);
}

void BootStatsSave(void)
{
    if (!s_dirty) return;

    FILE *f = fopen(STATS_FILE, "w");
    if (!f) return;

    fprintf(f, "# version %d | id total last unix sessions\n", STATS_VERSION);
    for (int i = 0; i < GameRegistryCount() && i < BOOT_MAX_GAMES; i++) {
        const GameEntry *g = GameRegistryGet(i);
        if (!g) continue;
        fprintf(f, "%s %.2f %.2f %ld %d\n", g->id,
                s_stats[i].totalSeconds, s_stats[i].lastSeconds,
                s_stats[i].lastPlayedUnix, s_stats[i].sessions);
    }
    fclose(f);
    s_dirty = false;
}

const GameStats *BootStatsGet(int gameIndex)
{
    static const GameStats empty = {0};
    return ValidIndex(gameIndex) ? &s_stats[gameIndex] : &empty;
}

void BootStatsReset(int gameIndex)
{
    if (!ValidIndex(gameIndex)) return;
    s_stats[gameIndex] = (GameStats){0};
    s_dirty = true;
    BootStatsSave();
}

void BootStatsResetAll(void)
{
    memset(s_stats, 0, sizeof(s_stats));
    s_dirty = true;
    BootStatsSave();
}

void BootStatsBeginSession(int gameIndex)
{
    if (!ValidIndex(gameIndex)) return;
    s_sessionStart[gameIndex] = GetTime();
    s_stats[gameIndex].sessions++;
    s_stats[gameIndex].lastPlayedUnix = (long)time(NULL);
    s_dirty = true;
}

void BootStatsEndSession(int gameIndex)
{
    if (!ValidIndex(gameIndex)) return;
    if (s_sessionStart[gameIndex] <= 0.0) return;

    double elapsed = GetTime() - s_sessionStart[gameIndex];
    if (elapsed < 0.0) elapsed = 0.0;

    s_stats[gameIndex].totalSeconds += elapsed;
    s_stats[gameIndex].lastSeconds   = elapsed;
    s_stats[gameIndex].lastPlayedUnix = (long)time(NULL);
    s_sessionStart[gameIndex] = 0.0;
    s_dirty = true;
    BootStatsSave();
}

double BootStatsTotalPlaytime(void)
{
    double sum = 0.0;
    for (int i = 0; i < GameRegistryCount() && i < BOOT_MAX_GAMES; i++) sum += s_stats[i].totalSeconds;
    return sum;
}

double BootStatsMaxPlaytime(void)
{
    double best = 0.0;
    for (int i = 0; i < GameRegistryCount() && i < BOOT_MAX_GAMES; i++) {
        if (s_stats[i].totalSeconds > best) best = s_stats[i].totalSeconds;
    }
    return best;
}

int BootStatsMostPlayedIndex(void)
{
    int best = -1;
    double bestTime = 0.0;
    for (int i = 0; i < GameRegistryCount() && i < BOOT_MAX_GAMES; i++) {
        if (s_stats[i].totalSeconds > bestTime) {
            bestTime = s_stats[i].totalSeconds;
            best = i;
        }
    }
    return best;
}

int BootStatsPlayedGameCount(void)
{
    int n = 0;
    for (int i = 0; i < GameRegistryCount() && i < BOOT_MAX_GAMES; i++) {
        if (s_stats[i].sessions > 0) n++;
    }
    return n;
}

// Bộ đệm xoay vòng: cho phép gọi nhiều lần trong cùng một lệnh vẽ.
static char *NextBuffer(void)
{
    static char pool[4][48];
    static int slot = 0;
    slot = (slot + 1) % 4;
    return pool[slot];
}

const char *BootStatsFormatDuration(double seconds)
{
    char *buf = NextBuffer();

    if (seconds < 1.0) {
        snprintf(buf, 48, "—");
    } else if (seconds < 60.0) {
        snprintf(buf, 48, "%ds", (int)seconds);
    } else if (seconds < 3600.0) {
        snprintf(buf, 48, "%dm", (int)(seconds / 60.0));
    } else {
        int hours = (int)(seconds / 3600.0);
        int mins  = (int)((seconds - hours * 3600.0) / 60.0);
        snprintf(buf, 48, "%dh %02dm", hours, mins);
    }
    return buf;
}

const char *BootStatsFormatRelative(long unixTime)
{
    char *buf = NextBuffer();

    if (unixTime <= 0) {
        snprintf(buf, 48, "Chưa chơi");
        return buf;
    }

    double diff = difftime(time(NULL), (time_t)unixTime);
    if (diff < 0.0) diff = 0.0;

    if (diff < 60.0)          snprintf(buf, 48, "Vừa xong");
    else if (diff < 3600.0)   snprintf(buf, 48, "%d phút trước", (int)(diff / 60.0));
    else if (diff < 86400.0)  snprintf(buf, 48, "%d giờ trước", (int)(diff / 3600.0));
    else if (diff < 2592000.0) snprintf(buf, 48, "%d ngày trước", (int)(diff / 86400.0));
    else {
        struct tm *lt = localtime((const time_t *)&unixTime);
        if (lt) snprintf(buf, 48, "%02d/%02d/%d", lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900);
        else    snprintf(buf, 48, "Đã lâu");
    }
    return buf;
}
