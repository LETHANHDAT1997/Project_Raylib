#include "save_data.h"
#include "raylib.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #define MKDIR(p) mkdir((p), 0755)
#endif

#define SAVE_MAX_GAMES 16
#define SAVE_MAX_KEYS  32
#define SAVE_KEY_LEN   48
#define SAVE_ID_LEN    48

typedef struct {
    char key[SAVE_KEY_LEN];
    int value;
} SaveEntry;

// Bộ nhớ đệm theo game: Hub vẽ mỗi khung hình nên không được đọc đĩa liên tục.
typedef struct {
    char gameId[SAVE_ID_LEN];
    SaveEntry entries[SAVE_MAX_KEYS];
    int count;
    bool used;
} SaveRecord;

static SaveRecord s_records[SAVE_MAX_GAMES];
static char s_dir[512] = "";

// ---------------------------------------------------------------- Thư mục

// Tạo thư mục kèm mọi thư mục cha còn thiếu (như "mkdir -p").
static void MakeDirs(const char *path)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", path);
    size_t len = strlen(buf);
    for (size_t i = 1; i < len; i++) {
        if (buf[i] == '/' || buf[i] == '\\') {
            char saved = buf[i];
            buf[i] = '\0';
            if (!(i == 2 && buf[1] == ':')) MKDIR(buf);   // Bỏ qua "C:" trên Windows
            buf[i] = saved;
        }
    }
    MKDIR(buf);
}

static void ResolveDir(void)
{
    if (s_dir[0] != '\0') return;

    const char *override = getenv("RAYLIB_ARCADE_DATA");
    char base[480] = "";

    if (override && override[0]) {
        snprintf(base, sizeof(base), "%s", override);
    } else {
#if defined(_WIN32)
        const char *appdata = getenv("APPDATA");
        if (appdata && appdata[0]) snprintf(base, sizeof(base), "%s\\RaylibArcade", appdata);
#elif defined(__APPLE__)
        const char *home = getenv("HOME");
        if (home && home[0]) snprintf(base, sizeof(base), "%s/Library/Application Support/RaylibArcade", home);
#else
        const char *xdg = getenv("XDG_DATA_HOME");
        const char *home = getenv("HOME");
        if (xdg && xdg[0])       snprintf(base, sizeof(base), "%s/raylib-arcade", xdg);
        else if (home && home[0]) snprintf(base, sizeof(base), "%s/.local/share/raylib-arcade", home);
#endif
    }

    // Không xác định được thư mục người dùng: đành dùng thư mục "save" cạnh nơi chạy.
    if (base[0] == '\0') snprintf(base, sizeof(base), "save");

    size_t n = strlen(base);
    while (n > 1 && (base[n - 1] == '/' || base[n - 1] == '\\')) base[--n] = '\0';

    MakeDirs(base);
    if (!DirectoryExists(base)) {
        TraceLog(LOG_WARNING, "SAVE: không tạo được thư mục dữ liệu '%s'", base);
    }
    snprintf(s_dir, sizeof(s_dir), "%s/", base);
    TraceLog(LOG_INFO, "SAVE: thư mục dữ liệu: %s", s_dir);
}

const char *SaveDataDir(void)
{
    ResolveDir();
    return s_dir;
}

const char *SaveDataFilePath(const char *fileName)
{
    static char pool[4][640];
    static int slot = 0;
    slot = (slot + 1) % 4;
    snprintf(pool[slot], sizeof(pool[slot]), "%s%s", SaveDataDir(), fileName);
    return pool[slot];
}

// ---------------------------------------------------------------- Bản ghi

static const char *RecordPath(const char *gameId)
{
    return SaveDataFilePath(TextFormat("%s.txt", gameId));
}

static void LoadRecord(SaveRecord *r)
{
    r->count = 0;
    FILE *f = fopen(RecordPath(r->gameId), "r");
    if (!f) return;

    char line[160];
    while (fgets(line, sizeof(line), f) && r->count < SAVE_MAX_KEYS) {
        if (line[0] == '#') continue;
        char key[SAVE_KEY_LEN];
        int value = 0;
        if (sscanf(line, "%47s %d", key, &value) == 2) {
            snprintf(r->entries[r->count].key, SAVE_KEY_LEN, "%s", key);
            r->entries[r->count].value = value;
            r->count++;
        }
    }
    fclose(f);
}

static void WriteRecord(const SaveRecord *r)
{
    FILE *f = fopen(RecordPath(r->gameId), "w");
    if (!f) {
        TraceLog(LOG_WARNING, "SAVE: không ghi được '%s' (errno %d)", RecordPath(r->gameId), errno);
        return;
    }
    fprintf(f, "# Raylib Arcade - du lieu cua game '%s' (khoa gia_tri)\n", r->gameId);
    for (int i = 0; i < r->count; i++) {
        fprintf(f, "%s %d\n", r->entries[i].key, r->entries[i].value);
    }
    fclose(f);
}

static SaveRecord *GetRecord(const char *gameId)
{
    if (!gameId || !gameId[0]) return NULL;

    SaveRecord *freeSlot = NULL;
    for (int i = 0; i < SAVE_MAX_GAMES; i++) {
        if (s_records[i].used && strcmp(s_records[i].gameId, gameId) == 0) return &s_records[i];
        if (!s_records[i].used && !freeSlot) freeSlot = &s_records[i];
    }
    if (!freeSlot) freeSlot = &s_records[0];   // Hết chỗ: tái dùng ô đầu (chỉ là bộ đệm)

    memset(freeSlot, 0, sizeof(*freeSlot));
    snprintf(freeSlot->gameId, SAVE_ID_LEN, "%s", gameId);
    freeSlot->used = true;
    LoadRecord(freeSlot);
    return freeSlot;
}

static SaveEntry *FindEntry(SaveRecord *r, const char *key)
{
    for (int i = 0; i < r->count; i++) {
        if (strcmp(r->entries[i].key, key) == 0) return &r->entries[i];
    }
    return NULL;
}

int SaveDataGetInt(const char *gameId, const char *key, int fallback)
{
    SaveRecord *r = GetRecord(gameId);
    if (!r) return fallback;
    SaveEntry *e = FindEntry(r, key);
    return e ? e->value : fallback;
}

bool SaveDataHasKey(const char *gameId, const char *key)
{
    SaveRecord *r = GetRecord(gameId);
    return r && FindEntry(r, key) != NULL;
}

void SaveDataSetInt(const char *gameId, const char *key, int value)
{
    SaveRecord *r = GetRecord(gameId);
    if (!r || !key || !key[0]) return;

    SaveEntry *e = FindEntry(r, key);
    if (!e) {
        if (r->count >= SAVE_MAX_KEYS) return;
        e = &r->entries[r->count++];
        snprintf(e->key, SAVE_KEY_LEN, "%s", key);
    } else if (e->value == value) {
        return;   // Không đổi thì khỏi ghi đĩa
    }
    e->value = value;
    WriteRecord(r);
}

bool SaveDataSubmitBest(const char *gameId, const char *key, int score)
{
    if (score <= 0) return false;
    if (SaveDataHasKey(gameId, key) && score <= SaveDataGetInt(gameId, key, 0)) return false;
    SaveDataSetInt(gameId, key, score);
    return true;
}

void SaveDataClearGame(const char *gameId)
{
    SaveRecord *r = GetRecord(gameId);
    if (!r) return;
    r->count = 0;

    const char *path = RecordPath(gameId);
    if (FileExists(path) && remove(path) != 0) {
        TraceLog(LOG_WARNING, "SAVE: không xoá được '%s'", path);
    }
}

void SaveDataInvalidate(void)
{
    memset(s_records, 0, sizeof(s_records));
}

void SaveDataMigrateLegacy(const char *legacyFile, void (*importFn)(const char *legacyPath))
{
    if (!legacyFile || !FileExists(legacyFile)) return;

    if (importFn) importFn(legacyFile);

    char renamed[512];
    snprintf(renamed, sizeof(renamed), "%s.migrated", legacyFile);
    if (rename(legacyFile, renamed) == 0) {
        TraceLog(LOG_INFO, "SAVE: đã chuyển '%s' sang %s", legacyFile, SaveDataDir());
    }
}
