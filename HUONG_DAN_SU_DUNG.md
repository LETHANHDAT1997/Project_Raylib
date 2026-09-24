# ⚡ Raylib CMake Build Manager — Hướng Dẫn Sử Dụng

> **Công cụ GUI** giúp bạn cấu hình và build thư viện [raylib](https://github.com/raysan5/raylib) thông qua CMake một cách trực quan, không cần gõ lệnh thủ công.

---

## 📋 Mục Lục

- [Yêu cầu hệ thống](#yeu-cau)
- [Cách chạy](#cach-chay)
- [Giao diện tổng quan](#giao-dien)
- [Khu vực thư mục](#thu-muc)
- [Tab 1 — Cấu Hình Chính](#tab-core)
  - [Nền tảng — PLATFORM](#platform)
  - [Phiên bản OpenGL — OPENGL_VERSION](#opengl)
  - [Kiểu Build — CMAKE_BUILD_TYPE](#build-type)
  - [Shared Library — BUILD_SHARED_LIBS](#shared)
  - [Build Examples — BUILD_EXAMPLES](#examples)
  - [Module Âm thanh — USE_AUDIO](#audio)
  - [PIC — WITH_PIC](#pic)
- [Tab 2 — GLFW & Hiển Thị](#tab-glfw)
  - [GLFW là gì?](#glfw-intro)
  - [External GLFW — USE_EXTERNAL_GLFW](#external-glfw)
  - [X11 và Wayland](#x11-wayland)
- [Tab 3 — Debug & Tùy Chọn Thêm](#tab-advanced)
  - [Sanitizers là gì?](#sanitizers-intro)
  - [AddressSanitizer — ENABLE_ASAN](#asan)
  - [UndefinedBehaviorSanitizer — ENABLE_UBSAN](#ubsan)
  - [MemorySanitizer — ENABLE_MSAN](#msan)
  - [Số luồng CPU — -j](#jobs)
  - [CMake Generator — -G](#generator)
  - [Extra CMake flags](#extra-flags)
- [Preview lệnh CMake](#preview)
- [Các nút hành động](#buttons)
- [Output Log](#log)
- [Workflow đề xuất](#workflow)
- [Ví dụ cấu hình phổ biến](#examples-config)
- [Câu hỏi thường gặp](#faq)

---

<a id="yeu-cau"></a>
## 🖥️ Yêu Cầu Hệ Thống

| Thành phần | Yêu cầu |
|---|---|
| Python | 3.6 trở lên |
| Tkinter | Cài sẵn cùng Python (trên Linux: `sudo apt install python3-tk`) |
| CMake | 3.11 trở lên |
| Compiler | GCC / Clang (Linux/macOS) hoặc MSVC / MinGW (Windows) |
| Git | Để clone raylib submodule |

---

<a id="cach-chay"></a>
## 🚀 Cách Chạy

```bash
# Clone project (đã có raylib submodule)
git clone --recurse-submodules <your-repo-url>
cd Project_Raylib

# Chạy GUI
python3 raylib_builder_gui.py
```

Hoặc nếu đã clone rồi mà chưa có submodule:

```bash
git submodule update --init --recursive
python3 raylib_builder_gui.py
```

---

<a id="giao-dien"></a>
## 🗺️ Giao Diện Tổng Quan

```
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
```

---

<a id="thu-muc"></a>
## 📁 Khu Vực Thư Mục

| Trường | Mô tả |
|---|---|
| **Mã nguồn (Source `-S`)** | Đường dẫn đến thư mục chứa `CMakeLists.txt` của raylib (thường là `./raylib/`) |
| **Thư mục Build (Output `-B`)** | Nơi CMake sinh ra các file build trung gian. Mặc định: `<source>/build` |
| **Duyệt...** | Mở hộp thoại chọn thư mục |
| **Xóa Build** | Xóa toàn bộ thư mục build (hỏi xác nhận trước) — dùng khi cần build sạch từ đầu |

> **Lưu ý:** Khi bạn chọn lại **Mã nguồn**, đường dẫn **Thư mục Build** sẽ tự động cập nhật thành `<source_mới>/build`.

---

<a id="tab-core"></a>
## Tab 1 — ⚙️ Cấu Hình Chính

---

<a id="platform"></a>
### 🔷 Nền tảng — `PLATFORM`

**PLATFORM** cho CMake biết bạn đang build raylib để chạy trên hệ thống nào.
Raylib hỗ trợ nhiều nền tảng, mỗi nền tảng dùng backend đồ họa và cửa sổ khác nhau.

| Giá trị | Mô tả |
|---|---|
| **Desktop** *(mặc định)* | Windows / Linux / macOS — dùng GLFW để quản lý cửa sổ và OpenGL để render |
| **Win32** | Windows 32-bit dùng WinAPI trực tiếp, không cần GLFW |
| **Web** | WebAssembly qua **Emscripten** — chạy raylib trên trình duyệt |
| **WebRGFW** | Web nhưng dùng backend RGFW thay vì GLFW |
| **Android** | Build cho Android qua NDK — render bằng OpenGL ES |
| **Raspberry Pi** | Raspberry Pi dùng dispmanx hoặc DRM |
| **DRM** | Linux DRM/KMS — render thẳng ra màn hình, không cần X11 hay Wayland |
| **SDL** | Dùng **SDL2** làm backend cửa sổ thay GLFW |
| **RGFW** | Dùng **RGFW** (thư viện nhẹ thay thế GLFW) |
| **Memory** | Headless — không có cửa sổ, dùng để test hoặc render off-screen |

---

<a id="opengl"></a>
### 🔷 Phiên bản OpenGL — `OPENGL_VERSION`

**OpenGL** là API đồ họa tiêu chuẩn mà raylib dùng để vẽ 2D/3D lên màn hình.
Macro này chọn phiên bản OpenGL mà raylib sẽ dùng khi chạy.

| Giá trị | Mô tả |
|---|---|
| **OFF** *(mặc định)* | Tự động chọn phiên bản phù hợp nhất với phần cứng |
| **4.3** | OpenGL 4.3 — PC hiện đại (NVIDIA/AMD/Intel từ ~2012) |
| **3.3** | OpenGL 3.3 — phổ biến nhất, tương thích tốt |
| **2.1** | OpenGL 2.1 — phần cứng cũ hơn |
| **1.1** | OpenGL 1.1 — rất cũ, hầu như không dùng |
| **ES 2.0** | OpenGL ES 2.0 — mobile (Android, iOS), web (WebGL 1.0) |
| **ES 3.0** | OpenGL ES 3.0 — mobile hiện đại, web (WebGL 2.0) |
| **Software** | Render bằng CPU, không cần GPU — cực chậm nhưng chạy được ở mọi nơi |

> **Khuyến nghị:** Để `OFF` cho Desktop. Chọn `ES 2.0` nếu build cho Android hoặc Web.

---

<a id="build-type"></a>
### 🔷 Kiểu Build — `CMAKE_BUILD_TYPE`

**CMAKE_BUILD_TYPE** điều khiển mức độ tối ưu hóa của compiler khi biên dịch.

| Giá trị | Compiler flag | Mô tả |
|---|---|---|
| **Release** *(mặc định)* | `-O2` hoặc `-O3` | Tối ưu tốc độ, bỏ debug info — dùng cho sản phẩm cuối |
| **Debug** | `-O0 -g` | Không tối ưu, giữ đủ debug symbols — dùng khi phát triển |
| **RelWithDebInfo** | `-O2 -g` | Tối ưu + debug info — cân bằng, dùng để profile |
| **MinSizeRel** | `-Os` | Tối ưu kích thước binary nhỏ nhất — dùng cho embedded |

---

<a id="shared"></a>
### 🔷 Shared Library — `BUILD_SHARED_LIBS`

Quyết định raylib được build thành loại thư viện nào:

| Trạng thái | Kết quả | Mô tả |
|---|---|---|
| **Tắt** *(mặc định)* | `libraylib.a` (Linux) / `raylib.lib` (Win) | **Static library** — được nhúng thẳng vào file `.exe` / binary của bạn. Dễ phân phối, không cần file `.so` đi kèm. |
| **Bật** | `libraylib.so` (Linux) / `raylib.dll` (Win) | **Dynamic/Shared library** — được load lúc runtime. Binary nhỏ hơn nhưng cần file `.so`/`.dll` đi kèm khi chạy. |

---

<a id="examples"></a>
### 🔷 Build Examples — `BUILD_EXAMPLES`

Khi bật, CMake sẽ biên dịch thêm hàng trăm chương trình ví dụ mẫu nằm trong thư mục `raylib/examples/`.
Hữu ích khi bạn muốn xem raylib hoạt động như thế nào, nhưng sẽ làm thời gian build lâu hơn đáng kể.

---

<a id="audio"></a>
### 🔷 Module Âm thanh — `USE_AUDIO`

Raylib tích hợp thư viện **miniaudio** để phát âm thanh (nhạc nền, hiệu ứng âm thanh...).

| Trạng thái | Mô tả |
|---|---|
| **Bật** *(mặc định)* | Module âm thanh được compile vào raylib |
| **Tắt** | Bỏ qua module âm thanh — build nhỏ hơn, dùng khi game không cần audio |

---

<a id="pic"></a>
### 🔷 Position-Independent Code — `WITH_PIC`

**PIC (Position-Independent Code)** là kỹ thuật biên dịch cho phép code chạy ở bất kỳ địa chỉ bộ nhớ nào, thay vì địa chỉ cố định.

| Trạng thái | Mô tả |
|---|---|
| **Tắt** *(mặc định)* | Static library thông thường |
| **Bật** | Cần thiết khi bạn muốn nhúng static library (`libraylib.a`) vào một shared library (`.so`) khác trên Linux |

> **Khi nào bật?** Nếu bạn đang viết một plugin hoặc wrapper (`.so`) mà bên trong dùng raylib static, bắt buộc phải bật PIC.

---

<a id="tab-glfw"></a>
## Tab 2 — 🖥️ GLFW & Hiển Thị

---

<a id="glfw-intro"></a>
### 🔶 GLFW là gì?

**GLFW** (Graphics Library FrameWork) là thư viện mã nguồn mở giúp tạo cửa sổ ứng dụng, nhận input (bàn phím, chuột, gamepad) và quản lý OpenGL context — trên nhiều hệ điều hành (Windows, Linux, macOS).

Raylib sử dụng GLFW ở chế độ **Desktop** để:
- Mở cửa sổ ứng dụng
- Xử lý sự kiện input
- Tạo và quản lý OpenGL context

Raylib đi kèm sẵn một bản GLFW nhúng bên trong source code của nó, nên thông thường bạn **không cần cài GLFW riêng**.

---

<a id="external-glfw"></a>
### 🔶 Dùng GLFW ngoài — `USE_EXTERNAL_GLFW`

Macro này điều khiển raylib dùng bản GLFW nào:

| Giá trị | Mô tả |
|---|---|
| **OFF** *(mặc định)* | Dùng GLFW nhúng sẵn bên trong raylib — đơn giản, không phụ thuộc |
| **IF_POSSIBLE** | Ưu tiên GLFW của hệ thống nếu tìm thấy; nếu không thì dùng bản nhúng |
| **ON** | Bắt buộc dùng GLFW của hệ thống — lỗi nếu không tìm thấy. Cần cài: `sudo apt install libglfw3-dev` |

> **Khi nào dùng `ON`?** Khi bạn muốn tất cả ứng dụng trên hệ thống dùng chung một phiên bản GLFW, tránh xung đột.

---

<a id="x11-wayland"></a>
### 🔶 X11 và Wayland — `GLFW_BUILD_X11` / `GLFW_BUILD_WAYLAND`

Trên **Linux**, có hai hệ thống hiển thị (display server) chính:

| Display Server | Mô tả |
|---|---|
| **X11** (X Window System) | Hệ thống cũ, phổ biến nhất, hầu hết distro Linux đều hỗ trợ. Ổn định, tương thích rộng. |
| **Wayland** | Thế hệ mới hơn, hiệu suất tốt hơn, bảo mật hơn. Được dùng mặc định trên Ubuntu 22.04+, Fedora, Arch. |

Hai checkbox này bật/tắt hỗ trợ từng display server khi GLFW được build:

| Tùy chọn | CMake flag | Mô tả |
|---|---|---|
| X11 | `GLFW_BUILD_X11=ON` | Hỗ trợ X11 — mặc định **bật** |
| Wayland | `GLFW_BUILD_WAYLAND=ON` | Hỗ trợ Wayland — mặc định **tắt** |

> **⚠️ Quan trọng:** Phải bật **ít nhất một** trong hai. Nếu tắt cả hai, ứng dụng sẽ không thể mở cửa sổ trên Linux.

Thư viện cần cài trước khi build:
```bash
# X11
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

# Wayland
sudo apt install libwayland-dev libxkbcommon-dev wayland-protocols
```

---

<a id="tab-advanced"></a>
## Tab 3 — 🛠️ Debug & Tùy Chọn Thêm

---

<a id="sanitizers-intro"></a>
### 🔴 Sanitizers là gì?

**Sanitizers** là các công cụ phân tích bộ nhớ và hành vi chương trình được tích hợp vào compiler (GCC/Clang). Chúng hoạt động bằng cách chèn thêm code kiểm tra tự động vào binary lúc biên dịch.

Khi chạy chương trình, Sanitizer sẽ phát hiện và báo lỗi ngay lập tức (với thông báo chi tiết) thay vì để lỗi âm thầm gây ra crash khó hiểu sau đó.

**Lưu ý:** Sanitizer chỉ dùng trong quá trình **phát triển và debug**, không dùng trong bản Release vì làm chương trình chậm hơn và dùng nhiều RAM hơn.

---

<a id="asan"></a>
### 🔴 AddressSanitizer — `ENABLE_ASAN`

**ASan** phát hiện các lỗi liên quan đến bộ nhớ:

- **Buffer overflow** — ghi/đọc ngoài giới hạn mảng
- **Use-after-free** — dùng con trỏ sau khi đã `free()`
- **Use-after-return** — dùng con trỏ đến biến local đã ra khỏi scope
- **Memory leak** — cấp phát bộ nhớ nhưng không `free()`
- **Double free** — gọi `free()` hai lần trên cùng một con trỏ

```bash
# Compiler sẽ thêm flag: -fsanitize=address -fno-omit-frame-pointer
```

---

<a id="ubsan"></a>
### 🔴 UndefinedBehaviorSanitizer — `ENABLE_UBSAN`

**UBSan** phát hiện **undefined behavior** (hành vi không xác định theo chuẩn C/C++):

- **Integer overflow** — tràn số nguyên có dấu
- **Null pointer dereference** — truy cập con trỏ NULL
- **Division by zero** — chia cho 0
- **Shift overflow** — dịch bit quá giới hạn
- **Invalid enum value** — ép kiểu enum không hợp lệ
- **Misaligned memory access** — truy cập bộ nhớ không align đúng

```bash
# Compiler sẽ thêm flag: -fsanitize=undefined
```

---

<a id="msan"></a>
### 🔴 MemorySanitizer — `ENABLE_MSAN`

**MSan** phát hiện việc **đọc bộ nhớ chưa được khởi tạo**:

- Dùng biến local chưa gán giá trị
- Đọc vùng nhớ heap chưa được ghi vào

```bash
# Compiler sẽ thêm flag: -fsanitize=memory
```

> **⚠️ Cảnh báo:** MSan **chỉ hoạt động với Clang**, không hỗ trợ GCC. Không dùng đồng thời MSan và ASan vì chúng xung đột nhau.

---

<a id="jobs"></a>
### 🔧 Số luồng CPU build — `-j`

Flag `-j N` truyền vào lệnh `cmake --build` để build song song trên N CPU cores.

| Giá trị | Ý nghĩa |
|---|---|
| Bằng số CPU (mặc định) | Tận dụng tối đa, build nhanh nhất |
| Nhỏ hơn số CPU | Máy ít bị "đơ", nhưng build chậm hơn |
| 1 | Build tuần tự — dùng khi muốn đọc log dễ hơn |

---

<a id="generator"></a>
### 🔧 CMake Generator — `-G`

**CMake Generator** quyết định CMake sẽ sinh ra loại build system nào:

| Giá trị | Build system | Lệnh build | Mô tả |
|---|---|---|---|
| **Default** | Make (Linux) / MSBuild (Win) | `make` | Mặc định của hệ thống |
| **Ninja** | [Ninja](https://ninja-build.org/) | `ninja` | Nhanh hơn Make đáng kể, phân tích dependency thông minh hơn. Cần cài: `sudo apt install ninja-build` |
| **Unix Makefiles** | GNU Make | `make` | Bắt buộc dùng Make, bất kể hệ thống |

> **Khuyến nghị:** Dùng **Ninja** nếu bạn build thường xuyên — thường nhanh hơn Make 20-40%.

---

<a id="extra-flags"></a>
### 🔧 Extra CMake flags

Nhập bất kỳ CMake `-D` flag nào chưa có trong giao diện. Ví dụ:

| Flag | Mô tả |
|---|---|
| `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | Sinh `compile_commands.json` cho IDE (VSCode, clangd...) |
| `-DCMAKE_INSTALL_PREFIX=/usr/local` | Thư mục cài đặt khi chạy `cmake --install` |
| `-DCMAKE_C_COMPILER=clang` | Dùng Clang thay GCC |
| `-DCMAKE_TOOLCHAIN_FILE=/path/to/file.cmake` | Cross-compilation toolchain (Android, Emscripten...) |
| `-DCMAKE_VERBOSE_MAKEFILE=ON` | Hiển thị toàn bộ lệnh compile trong log |

---

<a id="preview"></a>
## 📝 Preview Lệnh CMake

Khung đen bên dưới phần cấu hình hiển thị **chính xác** lệnh sẽ được thực thi, cập nhật **theo thời gian thực** khi bạn thay đổi bất kỳ tùy chọn nào.

Ví dụ output:
```
cmake -B ./raylib/build -S ./raylib -DCMAKE_BUILD_TYPE=Release -DPLATFORM=Desktop \
  -DOPENGL_VERSION=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF \
  -DUSE_AUDIO=ON -DWITH_PIC=OFF -DUSE_EXTERNAL_GLFW=OFF \
  -DGLFW_BUILD_X11=ON -DGLFW_BUILD_WAYLAND=OFF
cmake --build ./raylib/build -j 8
```

Nhấn **📋 Sao chép** để copy toàn bộ lệnh vào clipboard.

---

<a id="buttons"></a>
## ▶️ Các Nút Hành Động

| Nút | Mô tả |
|---|---|
| **1. Configure (CMake)** | Chỉ chạy bước `cmake -B ... -S ...` — sinh ra Makefile/Ninja files |
| **2. Build** | Chỉ chạy bước `cmake --build ...` — biên dịch mã nguồn |
| **▶ Configure & Build Ngay** | Chạy cả hai bước liên tiếp tự động *(khuyến nghị)* |
| **⏹ Dừng (Stop)** | Gửi tín hiệu `SIGTERM` để dừng tiến trình đang chạy |
| **Dọn Log** | Xóa nội dung trong cửa sổ Output Log |

---

<a id="log"></a>
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

<a id="workflow"></a>
## 🔄 Workflow Đề Xuất

### Lần đầu build (từ submodule)

```
1. Mở ứng dụng: python3 raylib_builder_gui.py
2. [📁 Mã nguồn] → Duyệt → chọn thư mục ./raylib/
3. [📁 Thư mục Build] → giữ mặc định (./raylib/build)
4. Tab "⚙️ Cấu Hình Chính":
   - Platform: Desktop
   - Build Type: Release
5. Nhấn [▶ Configure & Build Ngay]
6. Chờ đến khi log hiện "🎉 HOÀN THÀNH THÀNH CÔNG!"
```

### Build lại sau khi sửa code raylib
```
Chỉ nhấn [2. Build] — không cần Configure lại
```

### Build sạch từ đầu
```
1. Nhấn [Xóa Build] → Xác nhận
2. Nhấn [▶ Configure & Build Ngay]
```

---

<a id="examples-config"></a>
## 💡 Ví Dụ Cấu Hình Phổ Biến

### Build Release tối ưu (mặc định)
- Platform: `Desktop` | Build Type: `Release` | Shared Libs: `tắt`
- → Tạo `libraylib.a` tĩnh, nhúng vào game của bạn

### Build Debug + phát hiện lỗi bộ nhớ
- Build Type: `Debug` | AddressSanitizer: `bật` | Generator: `Ninja`
- → Build nhanh, phát hiện lỗi bộ nhớ khi chạy

### Build Shared Library để chia sẻ
- Shared Libs: `bật` | WITH_PIC: `bật`
- → Tạo `libraylib.so`, nhiều chương trình dùng chung

### Build cho Web (Emscripten)
- Platform: `Web`
- Extra flags: `-DCMAKE_TOOLCHAIN_FILE=/path/to/emsdk/.../Emscripten.cmake`
- → Tạo `.wasm` chạy trên trình duyệt

### Build với Ninja (nhanh hơn)
- Generator: `Ninja` | Jobs: `<số CPU>`
- → Giảm thời gian build 20-40% so với Make

---

<a id="faq"></a>
## ❓ Câu Hỏi Thường Gặp

**Q: Tại sao nút "2. Build" báo lỗi "Thư mục build chưa tồn tại"?**  
A: Cần chạy **Configure** trước để tạo thư mục build. Nhấn "1. Configure" hoặc "▶ Configure & Build Ngay".

**Q: Chọn thư mục nguồn nhưng bị báo "Không tìm thấy CMakeLists.txt"?**  
A: Đảm bảo chọn đúng thư mục **raylib** (chứa file `CMakeLists.txt`), không phải thư mục gốc của project bạn.

**Q: Build thất bại với lỗi "X11 development files not found"?**  
A: Cài thư viện X11 trên Ubuntu/Debian:
```bash
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

**Q: Muốn dùng Ninja thay Make?**  
A: Cài Ninja (`sudo apt install ninja-build`), sau đó chọn **Generator: Ninja** trong Tab 3.

**Q: ASAN báo lỗi nhưng tôi không hiểu output?**  
A: Chạy với biến môi trường sau để có output dễ đọc hơn:
```bash
ASAN_OPTIONS=symbolize=1 ASAN_SYMBOLIZER_PATH=$(which llvm-symbolizer) ./your_program
```

**Q: Copy lệnh từ Preview rồi chạy trong terminal được không?**  
A: Được! Lệnh trong Preview hoàn toàn tương đương với những gì GUI thực thi.

---

*Tài liệu này được tạo cho [`raylib_builder_gui.py`](./raylib_builder_gui.py) — Raylib CMake Build GUI Tool.*
