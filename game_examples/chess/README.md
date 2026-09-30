# Cờ Vua 3D — bộ cờ đá cẩm thạch (C)

Cờ vua quốc tế với đồ hoạ 3D thật: bộ cờ đá cẩm thạch **Poly Haven "Chess Set"** (CC0) nạp từ glTF,
ánh sáng **PBR** (GGX), **bóng đổ shadow map** lọc PCF, phản chiếu môi trường giả lập và khử răng
cưa bằng **siêu lấy mẫu 2×**. Camera xoay / phóng to tự do, có góc nhìn thẳng từ trên xuống.

Đấu với máy ở ba mức **Dễ / Thường / Khó** hoặc hai người chung một máy. Tài nguyên: xem
`../assets/chess/LICENSES.md`.

---

## Điều khiển

| Phím / chuột | Tác dụng |
|---|---|
| Chuột trái | Chọn quân, bấm ô đích để đi (bấm lại quân để bỏ chọn) |
| Chuột phải kéo | Xoay camera quanh bàn cờ |
| Lăn chuột | Phóng to / thu nhỏ (trên bảng bên phải: cuộn biên bản) |
| `←` `↑` `→` `↓` / `W` `A` `S` `D` + `Enter` | Chơi bằng bàn phím (hướng theo góc nhìn hiện tại) |
| `U` / `Backspace` | Hoàn tác (đấu máy: lùi về lượt của bạn) |
| `H` | Gợi ý nước đi (ô sáng xanh lá) |
| `F` | Xoay bàn nửa vòng · `V` Nhìn từ trên / góc 3D · `C` Đặt lại camera |
| `N` | Ván mới |
| `Esc` | Bỏ chọn quân, bấm lần nữa để tạm dừng |
| `M` | Bật / tắt âm thanh |
| Phong cấp | `1`–`4` hoặc `Q` `R` `B` `N` (Hậu · Xe · Tượng · Mã), `Esc` để huỷ |

## Luật

Đầy đủ luật cờ vua quốc tế (`chess_board.c`): nhập thành hai cánh, bắt tốt qua đường, phong cấp
bốn loại quân, chiếu hết, hết nước đi (hoà pat), hoà do **luật 50 nước**, **lặp lại 3 lần** và
**không đủ quân chiếu hết**. Biên bản ghi theo ký hiệu đại số chuẩn (SAN: `Nf3`, `exd5`, `O-O`,
`e8=Q#`…). Bộ sinh nước khớp hoàn toàn các giá trị *perft* chuẩn (thế ban đầu tới độ sâu 5,
Kiwipete tới độ sâu 4).

## Máy chơi thế nào

AI chạy trên luồng riêng (`chess_ai.c`), đánh giá bằng vật chất + bảng vị trí (trung cuộc / tàn
cuộc nội suy theo pha), cấu trúc tốt (tốt cô lập, tốt chồng, tốt thông), xe cột mở, cặp tượng và
kỹ thuật dồn vua ở tàn cuộc để biết chiếu hết.

| Mức | Cách chơi |
|---|---|
| Dễ | Nhìn 1 nước + tìm kiếm tĩnh, cộng nhiễu ±140 điểm, 12% đi nước "ngẫu hứng" |
| Thường | Alpha-beta độ sâu 3 + tìm kiếm tĩnh, nhiễu nhỏ ±18 điểm ở gốc |
| Khó | Đào sâu dần với PVS, bảng chuyển vị, nước sát thủ, lịch sử, cắt tỉa nước rỗng, giảm độ sâu nước muộn, gia hạn khi bị chiếu — ~2,5 giây mỗi nước |

Kiểm thử máy đấu máy: Thường thắng Dễ, Khó thắng Thường (đều bằng chiếu hết).

## Đồ hoạ 3D và Virtual Canvas

Bóng đổ cần đổi framebuffer, việc không làm được khi đang vẽ vào canvas ảo của game. Vì vậy cảnh
3D được dựng vào render texture riêng **ngay trong bước Update** (`ChessRenderScene`), còn bước Draw
chỉ dán ảnh đó lên canvas rồi vẽ giao diện 2D đè lên. Ống kính được dời lệch tâm để bàn cờ nằm giữa
vùng không bị bảng thông tin che.

Mô hình glTF chứa bàn cờ + 32 quân đặt sẵn; raylib nạp mỗi primitive thành một mesh đã nhân sẵn
ma trận node, nên game nhận diện từng mesh theo **khối bao** (chiều cao → loại quân, phía z → màu)
thay vì phụ thuộc thứ tự node trong file.

Trong menu có tuỳ chọn **Chất lượng đồ hoạ**: *Cao* (siêu lấy mẫu 2×, shadow map 2048) hoặc *Nhẹ*
(1×, shadow map 1024) cho máy yếu. Nếu GPU không dựng được shadow map hay không biên dịch được
shader, game tự tắt bóng đổ / dùng shader mặc định thay vì hỏng màn hình; thiếu file mô hình thì
vẽ quân bằng khối hình học đơn giản.

## Thống kê

Thắng / thua / hoà với máy lưu theo từng độ khó trong `chess.txt` của thư mục dữ liệu chung;
khoá `best` là tổng số ván thắng máy (Hub hiển thị ở dòng "Thắng máy").

## Cấu trúc mã

```text
chess/
├── inc/  chess_board.h   Luật cờ vua, make/unmake, SAN (không phụ thuộc đồ hoạ)
│         chess_ai.h      AI trên luồng riêng
│         chess_types.h   Trạng thái game, hình 3D của quân, camera
│         chess_render.h  Cảnh 3D: mô hình, shader PBR, bóng đổ, chọn ô bằng tia chuột
│         chess_hud.h     Giao diện 2D: bảng bên phải, menu, phong cấp, kết quả, tạm dừng
│         chess_game.h    Vòng đời game, input, hoạt ảnh quân
│         chess_assets.h  Đường dẫn tài nguyên + âm thanh
│         chess_runner.h  API cho Arcade Hub
└── src/  (cùng tên) + main.c (bản chạy độc lập, bỏ qua khi build qua Hub)
```

Build và chạy riêng:

```bash
./build_and_run.sh
```
