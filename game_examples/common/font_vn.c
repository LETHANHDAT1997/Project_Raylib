#include "font_vn.h"
#include <stddef.h>
#include <string.h>

static Font s_fontVN = {0};
static Font s_fontVNBold = {0};
static int s_fontUsersCount = 0;

static const char *VIETNAMESE_UNICODE_CHARS =
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
    "ÀÁÂÃÈÉÊÌÍÒÓÔÕÙÚÝàáâãèéêìíòóôõùúýĂăĐđĨĩŨũƠơƯư"
    "ẠạẢảẤấẦầẨẩẪẫẬậẮắẰằẲẳẴẵẶặẸẹẺẻẼẽẾếỀềỂểỄễỆệỈỉỊị"
    "ỌọỎỏỐốỒồỔổỖỗỘộỚớỜờỞởỠỡỢợỤụỦủỨứỪừỬửỮữỰựỲỳỴỵỶỷỸỹ"
    "•★▶◀▲▼←→↑↓⌂🔄💡🔊🔇";

static const char *FindFontPath(const char *fontName)
{
    static char pathBuffer[256];

    const char *candidates[8];
    candidates[0] = TextFormat("assets/fonts/%s", fontName);
    candidates[1] = TextFormat("../assets/fonts/%s", fontName);
    candidates[2] = TextFormat("../../assets/fonts/%s", fontName);
    candidates[3] = TextFormat("../../game_examples/assets/fonts/%s", fontName);
    candidates[4] = TextFormat("/usr/share/fonts/truetype/dejavu/%s", strcmp(fontName, "dejavu_bold.ttf") == 0 ? "DejaVuSans-Bold.ttf" : "DejaVuSans.ttf");
    candidates[5] = "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf";
    candidates[6] = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
    candidates[7] = NULL;

    for (int i = 0; candidates[i] != NULL; i++) {
        if (FileExists(candidates[i])) {
            strncpy(pathBuffer, candidates[i], sizeof(pathBuffer) - 1);
            pathBuffer[sizeof(pathBuffer) - 1] = '\0';
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
            s_fontVN = LoadFontEx(regPath, 36, codepoints, codepointCount);
            SetTextureFilter(s_fontVN.texture, TEXTURE_FILTER_BILINEAR);
        }

        // Tải font đậm
        const char *boldPath = FindFontPath("dejavu_bold.ttf");
        if (boldPath) {
            s_fontVNBold = LoadFontEx(boldPath, 36, codepoints, codepointCount);
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
        if (s_fontVN.texture.id > 0) {
            UnloadFont(s_fontVN);
            s_fontVN = (Font){0};
        }
        if (s_fontVNBold.texture.id > 0 && s_fontVNBold.texture.id != s_fontVN.texture.id) {
            UnloadFont(s_fontVNBold);
            s_fontVNBold = (Font){0};
        }
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
