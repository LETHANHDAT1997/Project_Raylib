# ⚡ Raylib CMake Build Manager — Hướng Dẫn Sử Dụng

> **Công cụ GUI** giúp bạn cấu hình và build thư viện [raylib](https://github.com/raysan5/raylib) thông qua CMake một cách trực quan, không cần gõ lệnh thủ công.

---

## 📋 Mục Lục

- [Yêu cầu hệ thống](#-yêu-cầu-hệ-thống)
- [Cách chạy](#-cách-chạy)
- [Giao diện tổng quan](#-giao-diện-tổng-quan)
- [Tab 1 — Cấu Hình Chính](#tab-1--cấu-hình-chính)
- [Tab 2 — GLFW & Hiển Thị](#tab-2--glfw--hiển-thị)
- [Tab 3 — Debug & Tùy Chọn Thêm](#tab-3--debug--tùy-chọn-thêm)
- [Khu vực thư mục](#-khu-vực-thư-mục)
- [Preview lệnh CMake](#-preview-lệnh-cmake)
- [Các nút hành động](#-các-nút-hành-động)
- [Output Log](#-output-log)
- [Workflow đề xuất](#-workflow-đề-xuất)
- [Ví dụ cấu hình phổ biến](#-ví-dụ-cấu-hình-phổ-biến)
- [Câu hỏi thường gặp](#-câu-hỏi-thường-gặp)

---

## 🖥️ Yêu Cầu Hệ Thống

| Thành phần | Yêu cầu |
|---|---|
| Python | 3.6 trở lên |
| Tkinter | Cài sẵn cùng Python (trên Linux: `sudo apt install python3-tk`) |
| CMake | 3.11 trở lên |
| Compiler | GCC / Clang (Linux/macOS) hoặc MSVC / MinGW (Windows) |
| Git | Để clone raylib submodule |

---

## 🚀 Cách Chạy

\`\`\`bash
# Clone project (đã có raylib submodule)
git clone --recurse-submodules <your-repo-url>
cd Project_Raylib

# Chạy GUI
python3 raylib_builder_gui.py
\`\`\`

Hoặc nếu đã clone rồi mà chưa có submodule:

\`\`\`bash
git submodule update --init --recursive
python3 raylib_builder_gui.py
\`\`\`

---

## 🗺️ Giao Diện Tổng Quan

\`\`\`
┌─────────────────────────────────────────────────────────────┐
│  ⚡ Raylib CMake Build Manager   Tùy chỉnh cấu hình...      │
├─────────────────────────────────────────────────────────────┤
│  📁 Thư Mục Làm Việc (Directories)                          │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Mã nguồn (Source -S): [__________] [Duyệt...]       │   │
│  │ Thư mục Build (-B):   [__________] [Duyệt...][Xóa]  │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌──────────────┬──────────────────┬────────────────────┐  │
│  │ ⚙️ Cấu Hình  │ 🖥️ GLFW & Hiển  │ 🛠️ Debug & Thêm  │  │
│  │    Chính     │     Thị          │                    │  │
│  └──────────────┴──────────────────┴────────────────────┘  │
│  📝 Lệnh CMake Sẽ Thực Thi (Preview)  [📋 Sao chép]        │
├─────────────────────────────────────────────────────────────┤
│  [1. Configure]  [2. Build]  [▶ Configure & Build Ngay]     │
│  [⏹ Dừng]                                       [Dọn Log] │
│  📜 Output Log                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ (output màu sắc hiển thị ở đây)                     │   │
│  └─────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Thanh trạng thái (Status Bar)                              │
└─────────────────────────────────────────────────────────────┘
\`\`\`

---

## 📁 Khu Vực Thư Mục

Nằm ở phần trên cùng của giao diện.

| Trường | Mô tả |
|---|---|
| **Mã nguồn (Source `-S`)** | Đường dẫn đến thư mục chứa `CMakeLists.txt` của raylib (thường là `./raylib/`) |
| **Thư mục Build (Output `-B`)** | Nơi CMake sinh ra các file build trung gian. Mặc định: `<source>/build` |
| **Duyệt...** | Mở hộp thoại chọn thư mục |
| **Xóa Build** | Xóa toàn bộ thư mục build (hỏi xác nhận trước) — dùng khi cần build sạch từ đầu |

> **Lưu ý:** Khi bạn chọn lại **Mã nguồn**, đường dẫn **Thư mục Build** sẽ tự động cập nhật thành `<source_mới>/build`.

---

## Tab 1 — ⚙️ Cấu Hình Chính

### Nền tảng (PLATFORM)

Chọn nền tảng bạn muốn build raylib cho:

| Giá trị | Mô tả |
|---|---|
| **Desktop** *(mặc định)* | Windows / Linux / macOS thông thường |
| **Win32** | Windows 32-bit |
| **Web** | WebAssembly qua Emscripten |
| **WebRGFW** | Web với backend RGFW |
| **Android** | Android NDK |
| **Raspberry Pi** | Raspberry Pi (native/dispmanx) |
| **DRM** | Linux DRM/KMS (không cần X11/Wayland) |
| **SDL** | Backend SDL2 |
| **RGFW** | Backend RGFW |
| **Memory** | Không có cửa sổ (headless) |

### Phiên bản OpenGL (OPENGL_VERSION)

| Giá trị | Mô tả |
|---|---|
| **OFF** *(mặc định)* | Tự động phát hiện phiên bản tốt nhất |
| 4.3 / 3.3 / 2.1 / 1.1 | Chỉ định phiên bản OpenGL cụ thể |
| ES 2.0 / ES 3.0 | OpenGL ES (mobile, web, embedded) |
| Software | Software renderer |

### Kiểu Build (CMAKE_BUILD_TYPE)

| Giá trị | Mô tả |
|---|---|
| **Release** *(mặc định)* | Tối ưu tốc độ, không có debug info — dùng cho phát hành |
| **Debug** | Không tối ưu, có debug symbols — dùng khi phát triển |
| **RelWithDebInfo** | Tối ưu + debug info — cân bằng |
| **MinSizeRel** | Tối ưu kích thước file |

### Các tùy chọn bổ sung (Checkbox)

| Tùy chọn | CMake flag | Mô tả |
|---|---|---|
| Build dạng Shared Library | `BUILD_SHARED_LIBS=ON` | Tạo `.so` / `.dll` thay vì `.a` / `.lib` |
| Build Examples đi kèm | `BUILD_EXAMPLES=ON` | Biên dịch các ví dụ mẫu của raylib |
| Kích hoạt Module Âm thanh | `USE_AUDIO=ON` | Bật/tắt miniaudio — mặc định **bật** |
| Bật PIC cho thư viện tĩnh | `WITH_PIC=ON` | Position-Independent Code — cần cho `.so` trên Linux |

---

## Tab 2 — 🖥️ GLFW & Hiển Thị

### Dùng GLFW ngoài (USE_EXTERNAL_GLFW)

| Giá trị | Mô tả |
|---|---|
| **OFF** *(mặc định)* | Dùng GLFW nhúng sẵn bên trong raylib |
| **IF_POSSIBLE** | Ưu tiên GLFW của hệ thống nếu tìm thấy |
| **ON** | Bắt buộc dùng GLFW hệ thống (phải cài `libglfw3-dev`) |

### Cấu hình hiển thị Linux

Chỉ ảnh hưởng khi build cho **Desktop** trên **Linux**:

| Tùy chọn | CMake flag | Mô tả |
|---|---|---|
| GLFW hỗ trợ X11 | `GLFW_BUILD_X11=ON` | Hỗ trợ màn hình X11 — mặc định **bật** |
| GLFW hỗ trợ Wayland | `GLFW_BUILD_WAYLAND=ON` | Hỗ trợ Wayland compositor |

> **⚠️ Cảnh báo:** Trên Linux, phải bật **ít nhất một** trong hai (X11 hoặc Wayland), nếu không ứng dụng sẽ không hiển thị được cửa sổ.

---

## Tab 3 — 🛠️ Debug & Tùy Chọn Thêm

### Sanitizers

Công cụ phát hiện lỗi bộ nhớ tại runtime (chỉ dùng khi **Debug**):

| Sanitizer | CMake flag | Phát hiện |
|---|---|---|
| **AddressSanitizer** | `ENABLE_ASAN=ON` | Buffer overflow, use-after-free, memory leak |
| **UndefinedBehavior** | `ENABLE_UBSAN=ON` | Undefined behavior (integer overflow, null dereference...) |
| **MemorySanitizer** | `ENABLE_MSAN=ON` | Đọc bộ nhớ chưa khởi tạo |

> **⚠️ Chú ý:** Không dùng nhiều Sanitizer cùng lúc — ASAN và MSAN xung đột với nhau. Chỉ bật một loại tại một thời điểm.

### Số luồng CPU build (-j)

Số lượng job song song khi build. Mặc định bằng số CPU của máy (`os.cpu_count()`).

- Tăng lên để build nhanh hơn (đến giới hạn CPU)
- Giảm xuống nếu máy bị đơ khi build

### CMake Generator (-G)

| Giá trị | Mô tả |
|---|---|
| **Default** | Dùng generator mặc định của hệ thống (Make trên Linux) |
| **Ninja** | Build nhanh hơn Make — cần cài `ninja-build` |
| **Unix Makefiles** | Bắt buộc dùng `make` |

### Cờ CMake bổ sung (Extra flags)

Nhập thêm bất kỳ CMake flag nào chưa có trong giao diện:

\`\`\`
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
-DCMAKE_INSTALL_PREFIX=/usr/local
-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake
\`\`\`

---

## 📝 Preview Lệnh CMake

Khung đen ở phía dưới phần cấu hình hiển thị **chính xác** lệnh sẽ được thực thi, cập nhật theo thời gian thực khi bạn thay đổi bất kỳ tùy chọn nào.

Ví dụ output:
\`\`\`
cmake -B ./raylib/build -S ./raylib -DCMAKE_BUILD_TYPE=Release -DPLATFORM=Desktop -DOPENGL_VERSION=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF -DUSE_AUDIO=ON -DWITH_PIC=OFF -DUSE_EXTERNAL_GLFW=OFF -DGLFW_BUILD_X11=ON -DGLFW_BUILD_WAYLAND=OFF
cmake --build ./raylib/build -j 8
\`\`\`

Nhấn **📋 Sao chép** để copy toàn bộ lệnh vào clipboard, tiện dùng trong terminal.

---

## ▶️ Các Nút Hành Động

| Nút | Mô tả |
|---|---|
| **1. Configure (CMake)** | Chỉ chạy bước `cmake -B ... -S ...` — sinh ra Makefile/Ninja files |
| **2. Build** | Chỉ chạy bước `cmake --build ...` — biên dịch mã nguồn |
| **▶ Configure & Build Ngay** | Chạy cả hai bước liên tiếp tự động *(khuyến nghị)* |
| **⏹ Dừng (Stop)** | Gửi tín hiệu `SIGTERM` để dừng tiến trình đang chạy |
| **Dọn Log** | Xóa nội dung trong cửa sổ Output Log |

---

## 📜 Output Log

Log được tô màu theo loại thông báo:

| Màu | Ý nghĩa |
|---|---|
| 🟣 Tím | Lệnh đang được thực thi |
| 🔵 Xanh dương | Thông tin thông thường |
| 🟢 Xanh lá (đậm) | Thành công (`Built target`, `[100%]`) |
| 🟡 Vàng | Cảnh báo (`warning:`) |
| 🔴 Đỏ (đậm) | Lỗi (`error:`, `CMake Error`, `fatal error`) |

---

## 🔄 Workflow Đề Xuất

### Lần đầu build (từ submodule)

\`\`\`
1. Mở ứng dụng: python3 raylib_builder_gui.py
2. [📁 Mã nguồn] → Duyệt → chọn thư mục ./raylib/
3. [📁 Thư mục Build] → giữ mặc định (./raylib/build)
4. Tab "⚙️ Cấu Hình Chính":
   - Platform: Desktop
   - Build Type: Release
5. Nhấn [▶ Configure & Build Ngay]
6. Chờ đến khi log hiện "🎉 HOÀN THÀNH THÀNH CÔNG!"
\`\`\`

### Build lại sau khi sửa code

\`\`\`
1. Nhấn [2. Build] (không cần Configure lại)
\`\`\`

### Build sạch từ đầu

\`\`\`
1. Nhấn [Xóa Build] → Xác nhận
2. Nhấn [▶ Configure & Build Ngay]
\`\`\`

---

## 💡 Ví Dụ Cấu Hình Phổ Biến

### Build Release tối ưu (mặc định)
- Platform: `Desktop`
- Build Type: `Release`
- Shared Libs: `tắt` → tạo file `.a` tĩnh

### Build Debug để gỡ lỗi
- Build Type: `Debug`
- AddressSanitizer: `bật`
- Generator: `Ninja` (build nhanh hơn)

### Build Shared Library
- Shared Libs: `bật` → tạo `libraylib.so`
- WITH_PIC: `bật` (bắt buộc trên Linux)

### Build cho Web (Emscripten)
- Platform: `Web`
- Extra flags: `-DCMAKE_TOOLCHAIN_FILE=/path/to/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake`

---

## ❓ Câu Hỏi Thường Gặp

**Q: Tại sao nút "2. Build" bị disable hoặc báo lỗi?**  
A: Cần chạy **Configure** trước để tạo thư mục build. Nhấn "1. Configure" hoặc "▶ Configure & Build Ngay".

**Q: Thư mục nguồn không hợp lệ?**  
A: Đảm bảo thư mục nguồn chứa file `CMakeLists.txt` của raylib (không phải thư mục gốc của project bạn).

**Q: Build thất bại với lỗi về X11/Wayland?**  
A: Cài thư viện cần thiết:
\`\`\`bash
# Ubuntu/Debian
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
# Wayland
sudo apt install libwayland-dev libxkbcommon-dev
\`\`\`

**Q: Muốn dùng Ninja thay Make?**  
A: Cài Ninja trước (`sudo apt install ninja-build`), sau đó chọn **Generator: Ninja** trong Tab 3.

**Q: Lệnh CMake trong preview có thể copy dùng trong terminal không?**  
A: Được! Nhấn **📋 Sao chép** và dán vào terminal. Lệnh hoàn toàn tương đương với những gì GUI thực thi.

---

*Tài liệu này được tạo cho [`raylib_builder_gui.py`](./raylib_builder_gui.py) — Raylib CMake Build GUI Tool.*
