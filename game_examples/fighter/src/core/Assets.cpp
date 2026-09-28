#include "core/Assets.hpp"
#include <cstdio>
#include <sys/stat.h>

namespace fighter {

namespace {

bool DirExists(const std::string &p)
{
    struct stat st{};
    return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool FileExists(const std::string &p)
{
    struct stat st{};
    return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

} // namespace

Assets &Assets::Instance()
{
    static Assets instance;
    return instance;
}

Assets::Assets()
{
    root_ = ResolveRoot();
}

// Game có thể được chạy từ thư mục build của chính nó, từ gốc repo, hoặc từ
// Arcade Hub - nên ta dò ngược vài cấp thay vì cố định một đường dẫn.
std::string Assets::ResolveRoot() const
{
    const char *candidates[] = {
        "game_examples/assets/fighter/",
        "../game_examples/assets/fighter/",
        "../../game_examples/assets/fighter/",
        "../../../game_examples/assets/fighter/",
        "assets/fighter/",
        "../assets/fighter/",
        "../../assets/fighter/",
        "../../../assets/fighter/",
    };
    for (const char *c : candidates) {
        if (DirExists(std::string(c) + "characters")) return c;
    }
    return "assets/fighter/";
}

bool Assets::Exists(const std::string &relativePath) const
{
    return FileExists(root_ + relativePath);
}

const Texture2D &Assets::Texture(const std::string &relativePath)
{
    auto it = cache_.find(relativePath);
    if (it != cache_.end()) return it->second;

    const std::string full = root_ + relativePath;
    if (!FileExists(full)) {
        TraceLog(LOG_WARNING, "FIGHTER: thiếu asset '%s'", full.c_str());
        if (fallback_.id == 0) {
            Image img = GenImageColor(2, 2, MAGENTA);
            fallback_ = LoadTextureFromImage(img);
            UnloadImage(img);
        }
        return fallback_;
    }

    Texture2D tex = LoadTexture(full.c_str());
    // Sprite pixel-art: lọc NEAREST để phóng to không bị nhoè.
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    auto res = cache_.emplace(relativePath, tex);
    return res.first->second;
}

const Texture2D &Assets::TextureProcessed(const std::string &relativePath, const std::string &key,
                                          void (*process)(Image &, void *), void *user)
{
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;

    const std::string full = root_ + relativePath;
    if (!FileExists(full)) return Texture(relativePath);   // đi đường báo thiếu file

    Image img = LoadImage(full.c_str());
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if (process) process(img, user);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return cache_.emplace(key, tex).first->second;
}

void Assets::UnloadAll()
{
    for (auto &kv : cache_) UnloadTexture(kv.second);
    cache_.clear();
    if (fallback_.id != 0) {
        UnloadTexture(fallback_);
        fallback_ = Texture2D{};
    }
}

} // namespace fighter
