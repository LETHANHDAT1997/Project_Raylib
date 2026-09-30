#include "font_vn.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static Font s_fontVN = {0};
static Font s_fontVNBold = {0};
static int s_fontUsersCount = 0;

// Cỡ atlas gốc: chữ được nạp ở kích thước này rồi thu nhỏ khi vẽ.
// Đặt cao hơn mọi cỡ chữ thực tế để tiêu đề lớn vẫn sắc nét.
#define FONT_VN_BASE_SIZE 72

static const char *VIETNAMESE_UNICODE_CHARS =
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
    "ÀÁÂÃÈÉÊÌÍÒÓÔÕÙÚÝàáâãèéêìíòóôõùúýĂăĐđĨĩŨũƠơƯư"
    "ẠạẢảẤấẦầẨẩẪẫẬậẮắẰằẲẳẴẵẶặẸẹẺẻẼẽẾếỀềỂểỄễỆệỈỉỊị"
    "ỌọỎỏỐốỒồỔổỖỗỘộỚớỜờỞởỠỡỢợỤụỦủỨứỪừỬửỮữỰựỲỳỴỵỶỷỸỹ"
    "•★▶◀▲▼←→↑↓↖↗↘↙‹›∞⌂🔄💡🔊🔇"
    // Ký tự dấu câu và biểu tượng mà giao diện Liquid Glass sử dụng
    "·–—…×✓°«»";

static const char *FindFontPath(const char *fontName)
{
    static char pathBuffer[256];

    // KHÔNG dùng TextFormat() cho danh sách này: TextFormat chỉ có 4 buffer
    // xoay vòng (MAX_TEXTFORMAT_BUFFERS), lần gọi thứ 5 ghi đè lên chuỗi của
    // candidates[0] -> "assets/fonts/..." bị mất. Trên PC lỗi bị che vì máy có
    // sẵn DejaVu ở /usr/share/fonts; trên Raspberry Pi thì rơi về font mặc định
    // của raylib (chỉ có ASCII) và mất toàn bộ chữ có dấu.
    static const char *relativeDirs[] = {
        "assets/fonts/",
        "../assets/fonts/",
        "../../assets/fonts/",
        "../../game_examples/assets/fonts/",
    };
    const char *systemFonts[] = {
        strcmp(fontName, "dejavu_bold.ttf") == 0 ? "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
                                                 : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    };

    for (size_t i = 0; i < sizeof(relativeDirs) / sizeof(relativeDirs[0]); i++) {
        snprintf(pathBuffer, sizeof(pathBuffer), "%s%s", relativeDirs[i], fontName);
        if (FileExists(pathBuffer)) return pathBuffer;
    }
    for (size_t i = 0; i < sizeof(systemFonts) / sizeof(systemFonts[0]); i++) {
        if (FileExists(systemFonts[i])) {
            snprintf(pathBuffer, sizeof(pathBuffer), "%s", systemFonts[i]);
            return pathBuffer;
        }
    }
    return NULL;
}

void InitVietnameseFont(void)
{
    s_fontUsersCount++;
    if (s_fontVN.texture.id > 0) return;

    int codepointCount = 0;
    int *codepoints = LoadCodepoints(VIETNAMESE_UNICODE_CHARS, &codepointCount);

    if (codepoints && codepointCount > 0) {
        // Tải font thường
        const char *regPath = FindFontPath("dejavu.ttf");
        if (regPath) {
            s_fontVN = LoadFontEx(regPath, FONT_VN_BASE_SIZE, codepoints, codepointCount);
            SetTextureFilter(s_fontVN.texture, TEXTURE_FILTER_BILINEAR);
        }

        // Tải font đậm
        const char *boldPath = FindFontPath("dejavu_bold.ttf");
        if (boldPath) {
            s_fontVNBold = LoadFontEx(boldPath, FONT_VN_BASE_SIZE, codepoints, codepointCount);
            SetTextureFilter(s_fontVNBold.texture, TEXTURE_FILTER_BILINEAR);
        } else if (regPath) {
            s_fontVNBold = s_fontVN;
        }

        UnloadCodepoints(codepoints);
    }
}

Font GetVietnameseFont(void)
{
    return s_fontVN;
}

Font GetVietnameseFontBold(void)
{
    return s_fontVNBold;
}

void CloseVietnameseFont(void)
{
    s_fontUsersCount--;
    if (s_fontUsersCount <= 0) {
        s_fontUsersCount = 0;

        // Ghi nhớ trước khi xoá: nếu không tìm được font đậm riêng thì
        // s_fontVNBold đang trỏ chung texture với s_fontVN.
        bool boldIsAlias = (s_fontVNBold.texture.id == s_fontVN.texture.id);

        if (s_fontVN.texture.id > 0) {
            UnloadFont(s_fontVN);
            s_fontVN = (Font){0};
        }
        if (!boldIsAlias && s_fontVNBold.texture.id > 0) {
            UnloadFont(s_fontVNBold);
        }
        s_fontVNBold = (Font){0};
    }
}

void DrawTextVN(const char *text, int posX, int posY, int fontSize, Color color)
{
    if (!text || text[0] == '\0') return;

    if (s_fontVN.texture.id == 0) {
        DrawText(text, posX, posY, fontSize, color);
        return;
    }
    DrawTextEx(s_fontVN, text, (Vector2){(float)posX, (float)posY}, (float)fontSize, 1.0f, color);
}

void DrawTextVNBold(const char *text, int posX, int posY, int fontSize, Color color)
{
    if (!text || text[0] == '\0') return;

    if (s_fontVNBold.texture.id == 0) {
        DrawTextVN(text, posX, posY, fontSize, color);
        return;
    }
    DrawTextEx(s_fontVNBold, text, (Vector2){(float)posX, (float)posY}, (float)fontSize, 1.0f, color);
}

int MeasureTextVN(const char *text, int fontSize)
{
    if (!text || text[0] == '\0') return 0;

    if (s_fontVN.texture.id == 0) {
        return MeasureText(text, fontSize);
    }
    return (int)MeasureTextEx(s_fontVN, text, (float)fontSize, 1.0f).x;
}

int MeasureTextVNBold(const char *text, int fontSize)
{
    if (!text || text[0] == '\0') return 0;

    if (s_fontVNBold.texture.id == 0) {
        return MeasureTextVN(text, fontSize);
    }
    return (int)MeasureTextEx(s_fontVNBold, text, (float)fontSize, 1.0f).x;
}

void DrawTextVNPro(const char *text, Vector2 pos, float fontSize, float spacing, Color color)
{
    if (!text || text[0] == '\0') return;

    if (s_fontVN.texture.id == 0) {
        DrawText(text, (int)pos.x, (int)pos.y, (int)fontSize, color);
        return;
    }
    DrawTextEx(s_fontVN, text, pos, fontSize, spacing, color);
}

void DrawTextVNBoldPro(const char *text, Vector2 pos, float fontSize, float spacing, Color color)
{
    if (!text || text[0] == '\0') return;

    if (s_fontVNBold.texture.id == 0) {
        DrawTextVNPro(text, pos, fontSize, spacing, color);
        return;
    }
    DrawTextEx(s_fontVNBold, text, pos, fontSize, spacing, color);
}

Vector2 MeasureTextVNPro(const char *text, float fontSize, float spacing)
{
    if (!text || text[0] == '\0') return (Vector2){0.0f, 0.0f};

    if (s_fontVN.texture.id == 0) {
        return (Vector2){(float)MeasureText(text, (int)fontSize), fontSize};
    }
    return MeasureTextEx(s_fontVN, text, fontSize, spacing);
}

Vector2 MeasureTextVNBoldPro(const char *text, float fontSize, float spacing)
{
    if (!text || text[0] == '\0') return (Vector2){0.0f, 0.0f};

    if (s_fontVNBold.texture.id == 0) {
        return MeasureTextVNPro(text, fontSize, spacing);
    }
    return MeasureTextEx(s_fontVNBold, text, fontSize, spacing);
}
