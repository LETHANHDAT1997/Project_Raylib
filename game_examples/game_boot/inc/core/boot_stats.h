/**
 * boot_stats.h - Thống kê chơi game, lưu bền vững ra đĩa.
 *
 * Dữ liệu này nuôi các thẻ "Lần chơi gần đây" và "Thông tin game" ở Hub,
 * nên toàn bộ số liệu hiển thị đều là số thật chứ không phải placeholder.
 */
#ifndef BOOT_STATS_H
#define BOOT_STATS_H

#include <stdbool.h>

typedef struct {
    double totalSeconds;   // Tổng thời gian đã chơi
    double lastSeconds;    // Thời lượng của phiên gần nhất
    long   lastPlayedUnix; // Mốc thời gian phiên gần nhất (0 = chưa chơi)
    int    sessions;       // Số lần khởi động game
} GameStats;

void             BootStatsLoad(void);
void             BootStatsSave(void);
const GameStats *BootStatsGet(int gameIndex);
void             BootStatsReset(int gameIndex);
void             BootStatsResetAll(void);

// Đo thời lượng phiên chơi: gọi Begin khi vào game, End khi thoát ra Hub.
void   BootStatsBeginSession(int gameIndex);
void   BootStatsEndSession(int gameIndex);

double BootStatsTotalPlaytime(void);   // Tổng thời gian của mọi game
double BootStatsMaxPlaytime(void);     // Thời gian của game được chơi nhiều nhất
int    BootStatsPlayedGameCount(void); // Số game đã từng chơi ít nhất 1 lần
int    BootStatsMostPlayedIndex(void); // Chỉ số game chơi nhiều nhất (-1 nếu chưa có)

// Định dạng chuỗi hiển thị (trả về buffer tĩnh xoay vòng, dùng ngay trong frame).
const char *BootStatsFormatDuration(double seconds);   // "3h 42m"
const char *BootStatsFormatRelative(long unixTime);    // "2 ngày trước"

#endif // BOOT_STATS_H
