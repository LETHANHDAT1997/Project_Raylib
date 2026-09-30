# Flappy Plane — vỗ cánh né mỏm đá (C)

Biến thể của Flappy Bird: điều khiển chiếc máy bay cánh kép luồn qua khe giữa
những mỏm đá nhọn mọc từ đất và treo từ trần. Mỗi cú nhấn là một nhịp "tăng ga",
thả tay là máy bay chúc mũi rơi xuống.

Toàn bộ hình ảnh và âm thanh là **asset CC0 của Kenney** (bộ *Tappy Plane*),
xem `../assets/flappy/LICENSES.md`.

---

## Điều khiển

| Phím | Tác dụng |
|---|---|
| `Space` / `↑` / `W` / click chuột | Vỗ cánh (cất cánh ở màn chờ) |
| `←` `→` (menu) | Chọn máy bay |
| `↑` `↓` hoặc `1` `2` `3` (menu) | Chọn độ khó |
| `Enter` / `Space` (menu) | Bắt đầu |
| `P` / `Esc` | Tạm dừng |
| `R` | Chơi lại (khi tạm dừng / thua) |
| `Q` | Về menu (khi tạm dừng / thua) |
| `M` | Bật / tắt âm thanh |
| `F11` | Toàn màn hình (bản chạy độc lập) |

Menu và bảng tổng kết cũng bấm được bằng chuột.

## Luật chơi

* Qua mỗi cặp đá được **+1 điểm**. Sao nằm giữa hai cặp đá: đồng **+1**, bạc **+2**, vàng **+3** —
  muốn ăn phải bay lệch khỏi đường an toàn.
* Mỗi **10 điểm** chuyển sang vùng địa hình mới (Đồng Cỏ → Hẻm Núi → Núi Tuyết → Sông Băng →
  Vách Đá → quay vòng), sắc trời đổi theo.
* Càng nhiều điểm, tốc độ cuộn càng nhanh và khe đá càng hẹp (có giới hạn theo độ khó).
  Từ 6 điểm trở đi thỉnh thoảng chỉ có một mỏm đá cao vút để đổi nhịp.
* Huy chương: **đồng 10+**, **bạc 25+**, **vàng 50+**. Kỷ lục lưu riêng cho từng độ khó
  (khoá `best.easy` / `best.normal` / `best.hard`, cùng `best` là kỷ lục tổng) trong file
  `flappy.txt` của thư mục dữ liệu chung - xem mục "Dữ liệu lưu trên đĩa" ở `../README.md`.

## Cơ chế đáng chú ý

* **Hitbox tam giác**: mỏm đá là hình nhọn chứ không phải ống nước, nên va chạm dùng
  hình tròn (máy bay) với tam giác đo từ kênh alpha của sprite, nới mũi đá vài pixel cho công bằng.
* **Va chạm mặt đất theo đường đồi**: lúc nạp asset, game quét kênh alpha của từng dải đất
  để lấy độ cao từng cột — máy bay đâm vào sườn đồi đúng chỗ nhìn thấy.
* **Canvas 1280x768, thế giới 800x480**: thế giới vẽ qua `Camera2D` zoom 1.6 để khớp kích thước
  gốc của bộ sprite, còn chữ HUD vẽ thẳng ở độ phân giải canvas nên luôn sắc nét.

## Cấu trúc mã

| File | Vai trò |
|---|---|
| `flappy_types.h` | Hằng số, struct trạng thái game |
| `flappy_assets.c` | Nạp sprite, bảng **vùng địa hình** và **máy bay** (data-driven) |
| `flappy_world.c` | Vật lý máy bay, sinh đá/sao, va chạm, hạt hiệu ứng, bảng **độ khó** |
| `flappy_game.c` | Máy trạng thái: menu → chờ → chơi → rơi → tổng kết, lưu kỷ lục |
| `flappy_draw.c` | Vẽ thế giới, HUD, menu, bảng tổng kết; bố cục nút dùng chung cho chuột |
| `flappy_audio.c` | Nạp và phát âm thanh |
| `flappy_runner.c` | 4 hàm vòng đời cho Arcade Hub |
| `main.c` | Bản chạy độc lập (bỏ qua khi build với `IS_BUILD_ALL`) |

### Thêm máy bay / vùng địa hình mới

* **Máy bay**: thêm 3 frame `sprites/planes/<ten>1..3.png` và một dòng vào `s_planeSkins[]`
  trong `flappy_assets.c`. Thẻ chọn ở menu tự co giãn theo số lượng, không cần sửa UI.
* **Vùng địa hình**: thêm một dòng vào `s_biomeDefs[]` (đá mọc, đá treo, dải đất, sắc trời).
  Sprite đá phải cùng kích thước 108x239 và dải đất 808x71 như bộ gốc.
