# 🎮 Raylib Game Examples & Arcade Master Hub

Tuyển tập 5 trò chơi cổ điển được xây dựng bằng **C** và thư viện đồ họa **Raylib**, tích hợp kiến trúc **Virtual Canvas tự động thích ứng với mọi độ phân giải màn hình (HD, Full HD, 2K, 4K, Fullscreen)** và trình khởi động trung tâm **`game_boot` (Arcade Hub)** với giao diện **Liquid Glass**.

---

## 📁 Cấu Trúc Thư Mục

```text
game_examples/
├── CMakeLists.txt              # CMake tổng cấu hình & build tất cả game + Arcade Hub
├── build_all.sh                # Script build toàn bộ dự án vào thư mục build/
├── README.md                   # Tài liệu hướng dẫn
│
├── game_boot/                  # 🚀 [MASTER LAUNCHER] Arcade Hub - giao diện Liquid Glass
│   ├── CMakeLists.txt          # CMake cho game_boot với cờ -DIS_BUILD_ALL
│   ├── build_and_run.sh        # Script build và khởi chạy Arcade Hub
│   ├── inc/
│   │   ├── core/               # Tầng lõi: không biết gì về giao diện
│   │   │   ├── boot_types.h        # Kiểu dữ liệu dùng chung (BootApp, tab, transition)
│   │   │   ├── boot_app.h          # Vòng đời ứng dụng & điều phối Hub <-> game
│   │   │   ├── boot_canvas.h       # Virtual Canvas cho game con (render texture)
│   │   │   ├── boot_viewport.h     # Vẽ Hub ở độ phân giải gốc của cửa sổ
│   │   │   ├── boot_input.h        # Tầng input tập trung: phím -> hành động
│   │   │   ├── game_registry.h     # Danh mục game kiểu data-driven
│   │   │   ├── boot_stats.h        # Thống kê thời gian chơi (lưu ra đĩa)
│   │   │   └── boot_settings.h     # Tuỳ chọn người dùng (lưu ra đĩa)
│   │   ├── ui/                 # Tầng giao diện: thư viện Liquid Glass tái sử dụng được
│   │   │   ├── ui_theme.h          # Bảng màu, khoảng cách, cỡ chữ
│   │   │   ├── ui_glass.h          # Panel kính, blur nền, khúc xạ, quầng sáng, bóng đổ
│   │   │   ├── ui_glass_shaders.h  # Mã GLSL nhúng của hiệu ứng kính
│   │   │   ├── ui_focus.h          # Vòng focus bàn phím, điều hướng theo không gian
│   │   │   ├── ui_widgets.h        # Nút, chip, toggle, slider, progress, segmented
│   │   │   ├── ui_icons.h          # Bộ icon vector vẽ bằng primitive
│   │   │   └── ui_anim.h           # Easing và nội suy độc lập khung hình
│   │   └── hub/                # Tầng màn hình: lắp ghép các thành phần trên
│   │       ├── hub_screen.h        # Bố cục tổng & điều phối các khối con
│   │       ├── hub_background.h    # Nền mesh gradient trôi theo màu game
│   │       ├── hub_topbar.h        # Thanh điều hướng, tab, đồng hồ
│   │       ├── hub_sidebar.h       # Danh sách game + ô tổng quan
│   │       ├── hub_hero.h          # Tấm hero lớn: tranh, mô tả, nút chơi
│   │       ├── hub_info.h          # Thẻ thống kê và thông tin game
│   │       ├── hub_pages.h         # Trang Điều khiển và Cài đặt
│   │       ├── hub_wallpaper.h     # Quét & quản lý ảnh nền trong assets/wallpapers
│   │       ├── hub_art.h           # Artwork vẽ tay cho từng game
│   │       └── hub_overlay.h       # Dải gợi ý phím khi đang chơi
│   └── src/                    # Cấu trúc gương của inc/ (core/, ui/, hub/) + main.c
│
├── tetris/                     # Game 1: Xếp Gạch Cổ Điển Hiện Đại (780x740)
│   ├── CMakeLists.txt          # CMake độc lập cho Tetris
│   ├── build_and_run.sh        # Script build và chạy Tetris độc lập
│   ├── inc/                    # tetris_types.h, tetris_game.h, tetris_runner.h, ...
│   └── src/                    # main.c, tetris_game.c, tetris_runner.c, ...
│
├── space_invader/              # Game 2: Bắn Ruồi Không Gian (800x880)
│   ├── CMakeLists.txt          # CMake độc lập cho Space Invader
│   ├── build_and_run.sh        # Script build và chạy Space Invader độc lập
│   ├── inc/                    # space_types.h, space_game.h, space_runner.h, ...
│   └── src/                    # main.c, space_game.c, space_runner.c, ...
│
├── snake/                      # Game 3: Rắn Săn Mồi Retro Edition (960x720)
│   ├── CMakeLists.txt          # CMake độc lập cho Snake
│   ├── build_and_run.sh        # Script build và chạy Snake độc lập
│   ├── inc/                    # snake_types.h, snake_game.h, snake_runner.h, ...
│   └── src/                    # main.c, snake_game.c, snake_runner.c, ...
│
├── flappy/                     # Game 4: Flappy Plane - vỗ cánh né mỏm đá (1280x768)
│   ├── CMakeLists.txt          # CMake độc lập cho Flappy Plane
│   ├── build_and_run.sh        # Script build và chạy Flappy Plane độc lập
│   ├── README.md               # Luật chơi, cơ chế và cách thêm máy bay / vùng địa hình
│   ├── inc/                    # flappy_types.h, flappy_world.h, flappy_draw.h, flappy_runner.h, ...
│   └── src/                    # main.c, flappy_game.c, flappy_world.c, flappy_draw.c, ...
│
├── fighter/                    # Game 5: Đấu Sĩ - đối kháng 2D (1280x720, C++17 OOP)
│   ├── CMakeLists.txt          # CMake độc lập cho Fighter
│   ├── README.md               # Kiến trúc OOP & cách thêm nhân vật mới
│   ├── inc/
│   │   ├── core/               # Config, Assets, Animation, Input, Difficulty, Text
│   │   ├── entities/           # Fighter (lớp cơ sở), Move, Projectile, Particles, Arena
│   │   ├── characters/         # Roster + 4 lớp con nhân vật
│   │   ├── render/             # Background parallax, Hud
│   │   ├── scenes/             # Scene, Title, Select (Settings), Battle
│   │   └── fighter_runner.h    # API kiểu C để Arcade Hub gọi sang C++
│   └── src/                    # Cùng cấu trúc thư mục với inc/
│
├── common/                     # Mã dùng chung cho mọi game
│   ├── font_vn.h               # API vẽ chữ Unicode tiếng Việt
│   └── font_vn.c
│
└── assets/
    ├── fonts/                  # dejavu.ttf, dejavu_bold.ttf
    ├── wallpapers/             # Ảnh nền của Hub - thả thêm file vào đây là xong
    ├── flappy/                 # Sprite + âm thanh Kenney của Flappy Plane (CC0, xem LICENSES.md)
    └── fighter/                # Sprite của game Đấu Sĩ (giấy phép CC0, xem LICENSES.md)
        ├── characters/<id>/    # 8 file: idle, run, jump, fall, attack1, attack2, takehit, death
        └── backgrounds/        # Các lớp ảnh parallax của sân đấu
```

---

## 🌟 Kiến Trúc "Virtual Canvas" & Macro `IS_BUILD_ALL`

1. **Virtual Canvas (Độ phân giải ảo)**:
   * Toàn bộ logic hiển thị của mỗi trò chơi được vẽ lên một tấm canvas ảo cố định độ phân giải gốc của nó (`RenderTexture2D`).
   * Cửa sổ hiển thị của Raylib được cấu hình cờ: `FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI`.
   * Canvas ảo được tự động phóng to/thu nhỏ (Scale) tỷ lệ chuẩn lên bất kỳ màn hình nào (kể cả 2K, 4K hay kéo góc cửa sổ tự do) với viền đen cân đối (*Letterbox*), không bao giờ bị méo hình.
   * Tọa độ chuột được tự động chuyển đổi qua `SetMouseOffset` và `SetMouseScale`, giúp các nút bấm giao diện luôn nhấp chuột chuẩn xác trên mọi kích thước cửa sổ.
2. **Cơ chế Macro `IS_BUILD_ALL`**:
   * Khi build riêng lẻ từng thư mục con (`tetris/`, `space_invader/`, `snake/`, `flappy/`): Macro `IS_BUILD_ALL` không bật, file `main.c` của game đó được biên dịch thành file thực thi độc lập (đã tích hợp sẵn Virtual Canvas).
   * Khi build qua `game_boot`: Macro `-DIS_BUILD_ALL` được bật, các file `main.c` con sẽ tự động bỏ qua hàm `main()`, thay vào đó `game_boot` điều phối game thông qua các giao diện Runner (`Init...App`, `Update...App`, `Draw...App`, `Close...App`).

---

## 🚀 Cách Chạy Trình Khởi Động Trung Tâm (`game_boot`)

Khởi động Arcade Master Hub để chọn và chuyển đổi giữa tất cả các game:

```bash
cd game_examples/game_boot
./build_and_run.sh
```

**🎮 Phím điều khiển trong Arcade Hub:**

Toàn bộ giao diện dùng được **chỉ bằng bàn phím**, không cần chạm tới chuột.

| Phím | Ở trang Thư viện | Ở trang Điều khiển / Cài đặt |
| --- | --- | --- |
| **↑ ↓** hoặc **W S** | Chọn game | Di chuyển focus giữa các mục |
| **← →** hoặc **A D** | — | Chỉnh giá trị (thanh trượt, công tắc) hoặc sang mục bên cạnh |
| **Enter / Space** | Vào chơi game đang chọn | Kích hoạt mục đang focus |
| **1 … 9** | Chọn nhanh game theo thứ tự | — |
| **Tab / Shift+Tab** | Chuyển tới / lui giữa 3 trang | |
| **Esc** | — | Quay về trang Thư viện |
| **F1 / Home** | Đang chơi game: quay lại Hub ngay lập tức | |
| **F11** | Bật / tắt toàn màn hình | |
| **F12** | Chụp màn hình ra `arcade_hub_screenshot.png` | |

Giữ phím mũi tên sẽ tự lặp (chờ 0,4 s rồi lặp mỗi 0,07 s) nên kéo thanh trượt bằng bàn phím cũng mượt như bằng chuột.

**Chuột:** nhấn một lần để chọn, nhấn lần nữa (hoặc nhấn đúp) để vào chơi.

---

## ⌨️ Kiến Trúc Input & Focus

Việc xử lý input tách hẳn thành hai module, không nằm rải rác trong mã giao diện:

**`core/boot_input.c` — phím thành hành động.** Cả ứng dụng hỏi "người dùng muốn *làm gì*" chứ không hỏi "phím nào đang nhấn". Toàn bộ ánh xạ nằm trong đúng một bảng:

```c
static const ActionBinding BINDINGS[BOOT_ACTION_COUNT] = {
    [BOOT_ACTION_NAV_UP]   = {{KEY_UP, KEY_W, 0, 0}, true},
    [BOOT_ACTION_ACTIVATE] = {{KEY_ENTER, KEY_KP_ENTER, KEY_SPACE, 0}, false},
    ...
};
```

Muốn đổi phím tắt hay gắn thêm phím phụ thì sửa bảng này, không phải đi lùng `IsKeyPressed` khắp nơi. Cờ cuối cùng bật cơ chế giữ-để-lặp, viết một lần dùng cho mọi màn hình.

**`ui/ui_focus.c` — vòng focus bàn phím.** Kiểu immediate-mode: mỗi khung hình widget tự đăng ký hình chữ nhật của mình, module nhớ mục nào đang focus. Điều hướng tính **theo không gian** dựa trên vị trí thật của các ô:

```c
float along  = dx * dirX + dy * dirY;        // Đi đúng hướng bao xa
float across = fabsf(dx * dirY - dy * dirX); // Lệch sang bên bao nhiêu
float score  = along + across * 2.0f;        // Ưu tiên ô thẳng hàng
```

Nhờ vậy thứ tự vẽ không cần trùng thứ tự người dùng cảm nhận, và lưới 2×2 ở trang Điều khiển đi đúng bốn hướng. Nếu không có mục nào nằm đúng hướng thì lùi về đi theo thứ tự đăng ký, đảm bảo **mọi mục đều tới được** kể cả khi nằm ở cột khác.

Vài quyết định thiết kế đáng lưu ý:
* Widget khai báo cờ `UI_FOCUS_CONSUMES_H` nếu nó tự dùng Trái/Phải (thanh trượt, công tắc, nhóm nút chọn) — khi đó điều hướng ngang không nhảy sang widget khác.
* Cả **một hàng cài đặt** là một điểm dừng focus chứ không phải riêng ô điều khiển nhỏ bên trong, nên vòng focus to và rõ như thẻ game trong Thư viện.
* Vòng focus chỉ hiện khi người dùng thật sự đang dùng bàn phím (giống `:focus-visible` trên web); chạm vào chuột là nó ẩn đi, và bấm chuột vào đâu thì focus nhảy về đó để dùng tiếp bàn phím là đi từ chính chỗ vừa bấm.
* Trang Thư viện tự xử lý Lên/Xuống vì danh sách game chính là vòng focus của nó; các trang còn lại giao hẳn cho `ui_focus`.

---

## 🧊 Giao Diện "Liquid Glass"

Arcade Hub sử dụng giao diện kính mô phỏng phong cách Liquid Glass của Apple, dựng bằng shader GLSL nhúng sẵn trong mã nguồn (không cần file asset rời):

1. **Nền động** được vẽ vào một `RenderTexture` riêng: vài quầng màu lớn trôi chậm, ngả dần sang màu chủ đạo của game đang chọn.
2. **Làm mờ nhiều lớp**: nền được thu nhỏ 4 lần rồi lọc Gauss tách trục 3 vòng, cho ra lớp mờ mà tấm kính sẽ lấy mẫu.
3. **Shader kính** dựng hình bằng *signed distance field* chữ nhật bo góc, từ đó tính được mặt nạ khử răng cưa, pháp tuyến mép và độ sâu — đủ để tạo **khúc xạ ở viền**, **phản sáng đặc trưng**, **bóng đổ phía trong** và **quầng sáng** khi hover.
4. **Vẽ ở độ phân giải gốc**: riêng Hub không đi qua Virtual Canvas mà áp ma trận biến đổi rồi vẽ thẳng lên cửa sổ, nên chữ luôn sắc nét ở mọi kích thước màn hình. Các game con vẫn giữ nguyên cơ chế Virtual Canvas.
5. Nếu GPU không biên dịch được shader, giao diện **tự chuyển sang chế độ dự phòng** (hình chữ nhật bo góc bán trong suốt) thay vì hỏng màn hình.

**Chỉnh lớp kính:** trang **Cài đặt** có hai thanh trượt, hiệu lực ngay khi kéo:

| Thanh trượt | Tác dụng |
| --- | --- |
| **Độ mờ hậu cảnh** | Bán kính làm mờ của lớp nền mà kính lấy mẫu. Ở `0%` bỏ hẳn các vòng lọc và kính lấy mẫu thẳng hậu cảnh sắc nét — nhìn xuyên thấy rõ hình nền như kính trong. Kéo lên thì bán kính và số vòng lọc tăng cùng lúc, nên tăng mượt chứ không nhảy bậc. |
| **Độ đục mặt kính** | Mức kéo độ sáng của hậu cảnh về độ sáng đích của từng bề mặt. Ở `0%` tấm kính hiện gần như nguyên bản hậu cảnh; càng lên cao thì các lớp panel càng tách bạch khỏi nền. |

Hạ **Độ mờ hậu cảnh** cũng là cách giảm tải cho máy yếu: đó chính là phần tốn GPU nhất của hiệu ứng.

### Vì sao kính không có màu riêng

Vật liệu kính **không trộn về một màu cố định nào**. Shader lấy độ sáng của hậu cảnh rồi kéo về một mức đích bằng cách nhân đều cả ba kênh màu:

```glsl
float lum    = dot(base, vec3(0.2126, 0.7152, 0.0722));
float target = mix(lum, uTargetLum, uLevel);
vec3  col    = base * (target / max(lum, 0.02));
```

Vì scale đều ba kênh nên **sắc màu giữ nguyên tuyệt đối**, chỉ độ sáng thay đổi: nền trắng cho ra xám trung tính, nền xanh lục cho ra xanh lục. Mỗi lớp có một `targetLum` riêng (cửa sổ `0.30`, panel `0.34`, thẻ nổi `0.48`, vùng lõm `0.18`) — đó là thứ tạo ra cảm giác phân tầng, thay cho việc tô mỗi lớp một màu khác nhau.

Ngoại lệ duy nhất có chủ đích là `UiGlassStyleAccent`: nút "Chơi ngay", tab đang mở và mục đang chọn vẫn mang màu của game, vì đó là **tín hiệu trạng thái** chứ không phải màu nền của vật liệu. Bảng màu trong `ui_theme.h` cũng vì vậy mà không còn chứa bất kỳ màu nền nào cho kính.

---

## 🖼️ Hình Nền

Lớp kính lấy mẫu từ hình nền, nên đổi nền là đổi cả tông màu của toàn bộ giao diện.

**Thêm hình nền của bạn:** chỉ cần chép ảnh vào `game_examples/assets/wallpapers/`. Thư mục này được quét lúc khởi động, ảnh mới tự xuất hiện trong lưới chọn ở trang **Cài đặt → Hình nền**; tên file trở thành nhãn hiển thị (`lake_cabin.png` → *Lake cabin*).

Định dạng hỗ trợ: `.png` `.jpg` `.jpeg` `.bmp` `.qoi` `.tga`.

> **Lưu ý về JPEG:** raylib build mặc định **tắt** bộ giải mã JPEG (`config.h`: `SUPPORT_FILEFORMAT_JPG 0`), nên ảnh `.jpg` sẽ không nạp được. Hai file `CMakeLists.txt` của dự án đã bật lại bằng `target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_JPG=1)`. Nếu bạn dựng raylib theo cách khác, nhớ bật cờ này.

File nào raylib không giải mã được sẽ bị bỏ qua kèm cảnh báo trong log, và lưới chọn hiện số lượng bị loại bằng chữ màu cam — không im lặng biến mất.

Vài điểm đáng lưu ý:
* Lựa chọn được ghi vào `game_boot_settings.txt` theo tên file, nên vẫn đúng sau khi khởi động lại.
* Mục **Gradient động** luôn nằm đầu danh sách: đó là nền dựng bằng mã, không cần file, và cũng là phương án dự phòng nếu ảnh bị xoá hay hỏng.
* Ảnh được phủ kín theo kiểu *cover* kèm hiệu ứng Ken Burns rất chậm, cộng một lớp tối nhẹ để chữ trắng trên kính luôn đọc được. Tắt chuyển động bằng **Giảm chuyển động**.
* Bộ nhớ: mỗi ảnh chỉ giữ một thumbnail 320×180 cho ô chọn, ảnh đầy đủ chỉ nạp cho hình đang dùng.
* Nên dùng ảnh rộng từ 1920px trở lên. Ảnh mẫu `lake_cabin.png` chỉ 743×413 nên hơi mềm nét khi phóng lên màn hình lớn.

---

## ➕ Thêm Một Game Mới Vào Arcade Hub

Danh mục game là *data-driven*, nên không phải sửa bất kỳ file giao diện nào:

1. Viết game trong thư mục riêng, cung cấp 4 hàm vòng đời trong `<game>_runner.h`:
   `Init<Game>App()`, `Update<Game>App(float dt)`, `Draw<Game>App()`, `Close<Game>App()`.
2. Vẽ hai hàm artwork trong `game_boot/src/hub/hub_art.c`: một icon vuông cho sidebar và một tranh lớn cho hero (khai báo trong `hub_art.h`).
3. Thêm một phần tử vào mảng `s_games[]` trong `game_boot/src/core/game_registry.c`: tên, mô tả, thẻ thể loại, danh sách phím, độ phân giải, màu chủ đạo và con trỏ tới các hàm ở bước 1–2.
4. Thêm thư mục game vào `game_boot/CMakeLists.txt` (`file(GLOB ...)` và `target_include_directories`).
5. Nếu game có kỷ lục: lưu bằng `SaveDataSubmitBest("<id>", "best", diem)` trong `common/save_data.h`
   (với `<id>` trùng `.id` trong registry) và đặt `.recordLabel = "Kỷ lục"` - Hub tự hiển thị và tự xoá được.

Sidebar, hero, thẻ thông tin, trang Điều khiển và thống kê thời gian chơi sẽ tự động nhận game mới.

---

## 💾 Dữ Liệu Lưu Trên Đĩa

Mọi dữ liệu cần giữ qua các lần chạy - của Hub lẫn của từng game - nằm trong **một thư mục cố định
theo người dùng**, không phụ thuộc bạn chạy game từ thư mục nào (module `common/save_data.h`):

| Hệ điều hành | Thư mục dữ liệu |
| --- | --- |
| Linux | `$XDG_DATA_HOME/raylib-arcade/` (mặc định `~/.local/share/raylib-arcade/`) |
| macOS | `~/Library/Application Support/RaylibArcade/` |
| Windows | `%APPDATA%\RaylibArcade\` |
| Tuỳ chỉnh | đặt biến môi trường `RAYLIB_ARCADE_DATA=<thư mục>` |

Đường dẫn thực tế hiển thị trong trang **Cài đặt** của Hub.

| File | Nội dung |
| --- | --- |
| `game_boot_stats.txt` | Thời gian chơi, phiên gần nhất và số lần khởi động của từng game |
| `game_boot_settings.txt` | Hiệu ứng kính, âm lượng, hiện FPS, giảm chuyển động, hình nền |
| `<id game>.txt` | Dữ liệu riêng của game, mỗi dòng `khoá giá_trị` (vd. `snake.txt`, `flappy.txt`) |

Quy ước: khoá `best` là kỷ lục tổng của game - Hub đọc khoá này để hiện dòng **Kỷ lục** trong thẻ
"Thông tin game". Game có thể lưu thêm khoá khác (Flappy lưu `best.easy`, `best.normal`, `best.hard`).
Tên file trùng với `id` của game trong `game_registry.c`, nên khi bấm **Xoá thống kê & kỷ lục game này**
(menu `···` cạnh nút Chơi) hoặc **Xoá toàn bộ thống kê & kỷ lục** (trang Cài đặt), Hub biết chính xác
file nào cần xoá. Đấu Sĩ hiện chưa lưu điểm nên chỉ có thống kê thời gian chơi.

Bản cũ ghi các file `game_boot_*.txt`, `snake_highscore.dat`, `flappy_highscore.dat` ngay tại thư mục
đang chạy. Lần khởi động đầu tiên, Hub tự chuyển chúng sang thư mục mới rồi đổi tên file cũ thành
`*.migrated` (để không bị nhập lại sau khi xoá dữ liệu) - có thể xoá các file `.migrated` đó.

Các file của Hub đều ở dạng khoá-giá trị theo `id` game, nên thêm hoặc bớt game không làm hỏng dữ liệu cũ. Có thể xoá sạch thống kê ngay trong trang **Cài đặt**.

---

## 🕹️ Cách Build Và Chạy Từng Game Độc Lập

Bạn vẫn có thể vào từng thư mục để biên dịch và chạy riêng biệt từng game:

### 1. Game Tetris
```bash
cd game_examples/tetris
./build_and_run.sh
```

### 2. Game Space Invader
```bash
cd game_examples/space_invader
./build_and_run.sh
```

### 3. Game Snake
```bash
cd game_examples/snake
./build_and_run.sh
```

### 4. Game Flappy Plane
```bash
cd game_examples/flappy
./build_and_run.sh
```

### 5. Game Đấu Sĩ (Fighter)
```bash
cd game_examples/fighter
./build_and_run.sh
```

---

## ⚡ Build Toàn Bộ Dự Án Bằng Script Tổng

Chạy script tổng tại thư mục `game_examples`:

```bash
cd game_examples
./build_all.sh
```

Tất cả các file thực thi sẽ được tạo sẵn trong thư mục `build/`:
* `./build/game_boot/game_boot` *(Arcade Hub)*
* `./build/tetris/tetris`
* `./build/space_invader/space_invader`
* `./build/snake/snake`
* `./build/flappy/flappy`
* `./build/fighter/fighter`
