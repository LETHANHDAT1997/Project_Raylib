#include "boot_input.h"
#include "raylib.h"
#include <string.h>

#define MAX_KEYS_PER_ACTION 4

// Giữ phím bao lâu thì bắt đầu lặp, và lặp mỗi bao nhiêu giây.
#define REPEAT_DELAY    0.40f
#define REPEAT_INTERVAL 0.07f

typedef struct {
    int keys[MAX_KEYS_PER_ACTION];
    bool repeats;     // Giữ phím có tự lặp không
} ActionBinding;

typedef struct {
    bool down;
    bool pressed;
    float holdTime;
    float repeatTimer;
} ActionState;

// Bảng ánh xạ phím. Đây là chỗ duy nhất cần sửa khi muốn đổi phím tắt.
static const ActionBinding BINDINGS[BOOT_ACTION_COUNT] = {
    [BOOT_ACTION_NAV_UP]     = {{KEY_UP, KEY_W, 0, 0}, true},
    [BOOT_ACTION_NAV_DOWN]   = {{KEY_DOWN, KEY_S, 0, 0}, true},
    [BOOT_ACTION_NAV_LEFT]   = {{KEY_LEFT, KEY_A, 0, 0}, true},
    [BOOT_ACTION_NAV_RIGHT]  = {{KEY_RIGHT, KEY_D, 0, 0}, true},
    [BOOT_ACTION_ACTIVATE]   = {{KEY_ENTER, KEY_KP_ENTER, KEY_SPACE, 0}, false},
    [BOOT_ACTION_BACK]       = {{KEY_ESCAPE, 0, 0, 0}, false},
    [BOOT_ACTION_NEXT_TAB]   = {{KEY_TAB, 0, 0, 0}, false},
    [BOOT_ACTION_PREV_TAB]   = {{0, 0, 0, 0}, false},   // Xử lý riêng: Shift+Tab
    [BOOT_ACTION_HOME]       = {{KEY_F1, KEY_HOME, 0, 0}, false},
    [BOOT_ACTION_FULLSCREEN] = {{KEY_F11, 0, 0, 0}, false},
    [BOOT_ACTION_SCREENSHOT] = {{KEY_F12, 0, 0, 0}, false},
};

static ActionState s_state[BOOT_ACTION_COUNT];
static int  s_digit = 0;
static bool s_mouseActive = false;
static bool s_keyboardActive = false;

static bool AnyKeyDown(const ActionBinding *b)
{
    for (int i = 0; i < MAX_KEYS_PER_ACTION; i++) {
        if (b->keys[i] != 0 && IsKeyDown(b->keys[i])) return true;
    }
    return false;
}

void BootInputUpdate(float dt)
{
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    s_keyboardActive = false;

    for (int i = 0; i < BOOT_ACTION_COUNT; i++) {
        const ActionBinding *b = &BINDINGS[i];
        ActionState *st = &s_state[i];

        bool down = AnyKeyDown(b);

        // Tab và Shift+Tab dùng chung một phím nhưng là hai hành động.
        if (i == BOOT_ACTION_NEXT_TAB)      down = down && !shift;
        else if (i == BOOT_ACTION_PREV_TAB) down = IsKeyDown(KEY_TAB) && shift;

        st->pressed = false;

        if (down && !st->down) {
            st->pressed = true;
            st->holdTime = 0.0f;
            st->repeatTimer = REPEAT_DELAY;
        } else if (down && b->repeats) {
            st->holdTime += dt;
            st->repeatTimer -= dt;
            if (st->repeatTimer <= 0.0f) {
                st->pressed = true;
                st->repeatTimer = REPEAT_INTERVAL;
            }
        }

        st->down = down;
        if (st->pressed) s_keyboardActive = true;
    }

    // Phím số chọn nhanh
    s_digit = 0;
    for (int i = 0; i < 9; i++) {
        if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) {
            s_digit = i + 1;
            s_keyboardActive = true;
            break;
        }
    }

    // Chuột được coi là "đang dùng" khi di chuyển hoặc bấm; lúc đó vòng
    // focus bàn phím sẽ ẩn đi để giao diện không có hai con trỏ cùng lúc.
    Vector2 delta = GetMouseDelta();
    if (delta.x != 0.0f || delta.y != 0.0f ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetMouseWheelMove() != 0.0f) {
        s_mouseActive = true;
    }
    if (s_keyboardActive) s_mouseActive = false;
}

bool BootInputPressed(BootAction action)
{
    if (action < 0 || action >= BOOT_ACTION_COUNT) return false;
    return s_state[action].pressed;
}

bool BootInputDown(BootAction action)
{
    if (action < 0 || action >= BOOT_ACTION_COUNT) return false;
    return s_state[action].down;
}

int BootInputDigitPressed(void)
{
    return s_digit;
}

bool BootInputMouseActive(void)
{
    return s_mouseActive;
}

bool BootInputKeyboardActive(void)
{
    return s_keyboardActive;
}

