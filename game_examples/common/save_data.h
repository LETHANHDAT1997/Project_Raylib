/**
 * save_data.h - Kho dữ liệu lưu bền vững dùng chung cho mọi game và Arcade Hub.
 *
 * Mọi thứ cần giữ qua các lần chạy (kỷ lục, thống kê, cài đặt) đều nằm trong
 * MỘT thư mục cố định theo người dùng, không phụ thuộc thư mục đang đứng khi
 * chạy game:
 *
 *   Linux   : $XDG_DATA_HOME/raylib-arcade   (mặc định ~/.local/share/raylib-arcade)
 *   macOS   : ~/Library/Application Support/RaylibArcade
 *   Windows : %APPDATA%\RaylibArcade
 *   Ghi đè  : biến môi trường RAYLIB_ARCADE_DATA=<thư mục>
 *
 * Mỗi game có một file văn bản riêng "<id>.txt" (id trùng với id trong
 * game_registry của Hub), mỗi dòng là "khoá giá trị". Nhờ vậy:
 *   - Hub đọc được kỷ lục của game để hiển thị mà không cần khởi động game;
 *   - Xoá dữ liệu một game = xoá đúng một file (SaveDataClearGame).
 *
 * Quy ước khoá: "best" là kỷ lục tổng của game - Hub hiển thị khoá này.
 */
#ifndef SAVE_DATA_H
#define SAVE_DATA_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Thư mục dữ liệu (có dấu '/' ở cuối), tự tạo nếu chưa có.
const char *SaveDataDir(void);

// Đường dẫn đầy đủ của một file nằm trong thư mục dữ liệu.
// Trả về bộ đệm tĩnh xoay vòng - hãy dùng hoặc sao chép ngay.
const char *SaveDataFilePath(const char *fileName);

// Đọc / ghi số nguyên theo khoá. Ghi là lưu xuống đĩa ngay.
int  SaveDataGetInt(const char *gameId, const char *key, int fallback);
bool SaveDataHasKey(const char *gameId, const char *key);
void SaveDataSetInt(const char *gameId, const char *key, int value);

// Ghi nhận điểm: chỉ lưu khi cao hơn giá trị đang có. Trả về true nếu là kỷ lục mới.
bool SaveDataSubmitBest(const char *gameId, const char *key, int score);

// Xoá toàn bộ dữ liệu đã lưu của một game (xoá file "<id>.txt").
void SaveDataClearGame(const char *gameId);

// Bỏ bộ nhớ đệm để lần đọc tới lấy lại từ đĩa (vd. file bị sửa từ tiến trình khác).
void SaveDataInvalidate(void);

// Chuyển file dữ liệu kiểu cũ (nằm ở thư mục đang chạy) sang thư mục dữ liệu.
// Nếu file cũ tồn tại: gọi importFn để nạp nó, rồi đổi tên thành "<file>.migrated"
// để không bị nhập lại lần nữa (nhất là sau khi người dùng bấm xoá dữ liệu).
void SaveDataMigrateLegacy(const char *legacyFile, void (*importFn)(const char *legacyPath));

#ifdef __cplusplus
}
#endif

#endif // SAVE_DATA_H
