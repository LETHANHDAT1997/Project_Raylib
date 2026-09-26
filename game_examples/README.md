# 🎮 Raylib Game Examples & Arcade Master Hub

Tuyển tập 3 trò chơi cổ điển được xây dựng bằng **C** và thư viện đồ họa **Raylib**, tích hợp kiến trúc **Virtual Canvas tự động thích ứng với mọi độ phân giải màn hình (HD, Full HD, 2K, 4K, Fullscreen)** và trình khởi động trung tâm **`game_boot` (Arcade Master Hub)**.

---

## 📁 Cấu Trúc Thư Mục

```text
game_examples/
├── CMakeLists.txt              # CMake tổng cấu hình & build tất cả game + Arcade Hub
├── build_all.sh                # Script build toàn bộ dự án vào thư mục build/
├── README.md                   # Tài liệu hướng dẫn
│
├── game_boot/                  # 🚀 [MASTER LAUNCHER] Trình chọn game & Canvas trung tâm
│   ├── CMakeLists.txt          # CMake cho game_boot với cờ -DIS_BUILD_ALL
│   ├── build_and_run.sh        # Script build và khởi chạy Arcade Hub
│   ├── inc/
│   │   ├── boot_types.h        # Quản lý trạng thái chuyển cảnh, struct canvas
│   │   ├── boot_canvas.h       # Quản lý Virtual Canvas, tỉ lệ scale, căn giữa màn hình
│   │   └── boot_menu.h         # Giao diện Arcade Hub chọn game, 3 card tương tác
│   └── src/
│       ├── main.c              # Hàm main của Hub, điều phối game và xử lý phím F1/F11
│       ├── boot_canvas.c       # Tính toán letterbox, tự động căn chỉnh tọa độ chuột
│       └── boot_menu.c         # Vẽ giao diện chọn game, icon pixel-art
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
└── snake/                      # Game 3: Rắn Săn Mồi Retro Edition (960x720)
    ├── CMakeLists.txt          # CMake độc lập cho Snake
    ├── build_and_run.sh        # Script build và chạy Snake độc lập
    ├── inc/                    # snake_types.h, snake_game.h, snake_runner.h, ...
    └── src/                    # main.c, snake_game.c, snake_runner.c, ...
```

---

## 🌟 Kiến Trúc "Virtual Canvas" & Macro `IS_BUILD_ALL`

1. **Virtual Canvas (Độ phân giải ảo)**:
   * Toàn bộ logic hiển thị của mỗi trò chơi được vẽ lên một tấm canvas ảo cố định độ phân giải gốc của nó (`RenderTexture2D`).
   * Cửa sổ hiển thị của Raylib được cấu hình cờ: `FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI`.
   * Canvas ảo được tự động phóng to/thu nhỏ (Scale) tỷ lệ chuẩn lên bất kỳ màn hình nào (kể cả 2K, 4K hay kéo góc cửa sổ tự do) với viền đen cân đối (*Letterbox*), không bao giờ bị méo hình.
   * Tọa độ chuột được tự động chuyển đổi qua `SetMouseOffset` và `SetMouseScale`, giúp các nút bấm giao diện luôn nhấp chuột chuẩn xác trên mọi kích thước cửa sổ.
2. **Cơ chế Macro `IS_BUILD_ALL`**:
   * Khi build riêng lẻ từng thư mục con (`tetris/`, `space_invader/`, `snake/`): Macro `IS_BUILD_ALL` không bật, file `main.c` của game đó được biên dịch thành file thực thi độc lập (đã tích hợp sẵn Virtual Canvas).
   * Khi build qua `game_boot`: Macro `-DIS_BUILD_ALL` được bật, các file `main.c` con sẽ tự động bỏ qua hàm `main()`, thay vào đó `game_boot` điều phối game thông qua các giao diện Runner (`Init...App`, `Update...App`, `Draw...App`, `Close...App`).

---

## 🚀 Cách Chạy Trình Khởi Động Trung Tâm (`game_boot`)

Khởi động Arcade Master Hub để chọn và chuyển đổi giữa tất cả các game:

```bash
cd game_examples/game_boot
./build_and_run.sh
```

**🎮 Phím điều khiển trong Arcade Hub:**
* **Mũi tên Trái / Phải** hoặc **A / D**: Chọn thẻ game
* **Phím số 1, 2, 3**: Chọn nhanh game tương ứng
* **ENTER / SPACE** hoặc **Click Chuột**: Vào chơi game đã chọn
* **F11**: Bật / Tắt chế độ Toàn Màn Hình (*Fullscreen*)
* **F1** hoặc **HOME**: Đang chơi game bất kỳ có thể nhấn để **quay lại Arcade Hub** ngay lập tức!

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

---

## ⚡ Build Toàn Bộ Dự Án Bằng Script Tổng

Chạy script tổng tại thư mục `game_examples`:

```bash
cd game_examples
./build_all.sh
```

Tất cả 4 file thực thi sẽ được tạo sẵn trong thư mục `build/`:
* `./build/game_boot/game_boot` *(Arcade Master Hub)*
* `./build/tetris/tetris`
* `./build/space_invader/space_invader`
* `./build/snake/snake`
