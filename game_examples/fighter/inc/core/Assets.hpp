#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>

namespace fighter {

// ============================================================================
// Assets - kho texture dùng chung.
//
// Texture được nạp một lần rồi tái sử dụng: màn chọn nhân vật và màn đấu cùng
// dùng chung sprite sheet, nạp lại mỗi lần sẽ vừa chậm vừa rò VRAM.
// ============================================================================
class Assets {
public:
    static Assets &Instance();

    // Trả về texture đã nạp; nếu chưa có thì nạp từ đĩa. Đường dẫn là tương đối
    // so với thư mục assets (ví dụ "characters/samurai/idle.png").
    const Texture2D &Texture(const std::string &relativePath);

    // Nạp ảnh, cho phép chỉnh sửa điểm ảnh trước khi đưa lên GPU, rồi cache
    // dưới tên `key` (ví dụ bản sprite đã làm dịu vệt chém).
    const Texture2D &TextureProcessed(const std::string &relativePath, const std::string &key,
                                      void (*process)(Image &, void *), void *user);

    // Có nạp được file này không (dùng để báo lỗi asset thiếu cho người dùng).
    bool Exists(const std::string &relativePath) const;

    void UnloadAll();

    // Thư mục gốc của assets, dò tự động lúc khởi động.
    const std::string &Root() const { return root_; }

private:
    Assets();
    Assets(const Assets &) = delete;
    Assets &operator=(const Assets &) = delete;

    std::string ResolveRoot() const;

    std::string root_;
    std::unordered_map<std::string, Texture2D> cache_;
    Texture2D fallback_{};   // texture hồng 1x1 khi thiếu file
};

} // namespace fighter
