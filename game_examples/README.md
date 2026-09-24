# 🎮 Raylib Game Examples

Tuyển tập 2 trò chơi cổ điển được xây dựng bằng **C** và thư viện đồ họa **Raylib**, chia tách mã nguồn rõ ràng theo chuẩn `src/` và `inc/`, hỗ trợ cấu hình CMake độc lập hoặc toàn cục, kèm script `build_and_run.sh` cho từng game.

---

## 📁 Cấu Trúc Thư Mục

```text
game_examples/
├── CMakeLists.txt              # CMake tổng để cấu hình & build tất cả game cùng lúc
├── build_all.sh                # Script build toàn bộ các game vào thư mục build/
├── README.md                   # Tài liệu hướng dẫn
│
├── tetris/                     # Game 1: Xếp Gạch Cổ Điển Hiện Đại
│   ├── CMakeLists.txt          # CMake độc lập cho Tetris
│   ├── build_and_run.sh        # Script build và khởi chạy riêng cho Tetris
│   ├── inc/                    # Thư mục Header files (.h)
│   │   ├── tetris_types.h      # Khai báo kiểu dữ liệu, struct bảng, mảnh ghép
│   │   ├── tetris_piece.h      # Ma trận 7 khối tetromino, thuật toán 7-bag
│   │   ├── tetris_board.h      # Xử lý va chạm, lock piece, xóa hàng, bóng ghost
│   │   ├── tetris_audio.h      # Tạo âm thanh 8-bit procedural waveform
│   │   └── tetris_game.h       # Vòng lặp game, điểm số, combo, hạt particle
│   └── src/                    # Thư mục Source files (.c)
│       ├── main.c              # Hàm main, khởi tạo cửa sổ Raylib
│       ├── tetris_piece.c
│       ├── tetris_board.c
│       ├── tetris_audio.c
│       └── tetris_game.c
│
└── space_invader/              # Game 2: Bắn Ruồi / Space Invaders Cổ Điển
    ├── CMakeLists.txt          # CMake độc lập cho Space Invader
    ├── build_and_run.sh        # Script build và khởi chạy riêng cho Space Invader
    ├── inc/                    # Thư mục Header files (.h)
    │   ├── space_types.h       # Định nghĩa tàu, 55 quái ngoài hành tinh, khiên, đạn
    │   ├── space_player.h      # Pháo phòng thủ người chơi, di chuyển, mạng
    │   ├── space_aliens.h      # Pixel-art bitmaps, hàng ngũ quái, đĩa bay UFO
    │   ├── space_bunker.h      # 4 boongke phòng thủ phá hủy từng khối nhỏ
    │   ├── space_audio.h       # Âm thanh laser, nổ, nhịp hành quân 4 nốt
    │   └── space_game.h        # Vòng lặp game, va chạm, sao vũ trụ, hiệu ứng CRT
    └── src/                    # Thư mục Source files (.c)
        ├── main.c              # Hàm main, khởi tạo cửa sổ Raylib
        ├── space_player.c
        ├── space_aliens.c
        ├── space_bunker.c
        ├── space_audio.c
        └── space_game.c
```

---

## 🚀 Cách Build Và Chạy Từng Game

### 1. Game Tetris
Vào thư mục `game_examples/tetris` và chạy script:

```bash
cd game_examples/tetris
./build_and_run.sh
```

Hoặc build thủ công bằng CMake:
```bash
cd game_examples/tetris
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/tetris
```

**🎮 Phím điều khiển Tetris:**
- **Mũi tên Trái / Phải** hoặc **A / D**: Di chuyển mảnh ghép sang trái/phải
- **Mũi tên Lên / W / X**: Xoay mảnh ghép thuận chiều kim đồng hồ
- **Z**: Xoay ngược chiều kim đồng hồ
- **Mũi tên Xuống / S**: Thả nhanh (Soft Drop)
- **Space**: Thả rơi tức thì và khóa mảnh (Hard Drop)
- **C** hoặc **Left Shift**: Giữ mảnh ghép (Hold Piece)
- **P** hoặc **ESC**: Tạm dừng (Pause / Resume)
- **M**: Bật / Tắt âm thanh (Mute)
- **Enter** hoặc **R**: Chơi lại khi Game Over

---

### 2. Game Space Invader
Vào thư mục `game_examples/space_invader` và chạy script:

```bash
cd game_examples/space_invader
./build_and_run.sh
```

Hoặc build thủ công bằng CMake:
```bash
cd game_examples/space_invader
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/space_invader
```

**🎮 Phím điều khiển Space Invader:**
- **Mũi tên Trái / Phải** hoặc **A / D**: Di chuyển pháo phòng thủ
- **Phím Cách (Space) / W / Mũi tên Lên**: Bắn đạn laser
- **M**: Bật / Tắt âm thanh
- **Enter / Space / R**: Bắt đầu game từ Menu / Chơi lại khi Game Over

---

## ⚡ Build Cả 2 Game Bằng CMake Tổng

Nếu bạn muốn build cả hai game cùng một lúc từ thư mục `game_examples`:

```bash
cd game_examples
./build_all.sh
```

File thực thi của 2 game sẽ được tạo tại:
- `./build/tetris/tetris`
- `./build/space_invader/space_invader`
