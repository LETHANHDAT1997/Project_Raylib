# Cờ Caro — năm quân liền hàng (C)

Cờ caro (gomoku) trên bàn gỗ sồi 15×15 hoặc 19×19. Đấu với máy ở ba mức **Dễ / Thường / Khó**
hoặc hai người chung một máy. Quân X / O được dựng bằng **shader SDF** nên nét luôn mịn ở mọi độ
phân giải, có bóng đổ mềm và hoạt ảnh "vẽ nét" khi đặt quân.

Vân gỗ là texture CC0 của Poly Haven, âm thanh là asset CC0 của Kenney — xem
`../assets/caro/LICENSES.md`.

---

## Điều khiển

| Phím | Tác dụng |
|---|---|
| Chuột trái | Đặt quân vào ô đang trỏ |
| `←` `↑` `→` `↓` / `W` `A` `S` `D` | Di chuyển con trỏ trên bàn |
| `Enter` / `Space` | Đặt quân tại con trỏ |
| `U` / `Backspace` | Hoàn tác (đấu máy: lùi về lượt của bạn) |
| `H` | Gợi ý nước đi |
| `N` | Ván mới (giữ nguyên tuỳ chọn) |
| `Esc` / `P` | Tạm dừng (Tiếp tục · Ván mới · Về menu · Âm thanh) |
| `M` | Bật / tắt âm thanh |
| Menu: `↑` `↓` chọn dòng, `←` `→` đổi lựa chọn, `1` `2` `3` độ khó | |

## Tuỳ chọn ván

| Tuỳ chọn | Giá trị |
|---|---|
| Chế độ | Đấu với máy · 2 người |
| Độ khó | Dễ · Thường · Khó |
| Bàn cờ | 15 × 15 · 19 × 19 |
| Luật thắng | **Tự do**: đủ 5 quân liền (hoặc hơn) là thắng · **Chặn 2 đầu**: hàng 5 bị quân đối phương chặn kín hai đầu không tính (mép bàn không tính là bị chặn) |
| Đi trước | Bạn cầm X đi trước · Máy cầm X đi trước |

## Máy chơi thế nào

AI chạy trên **luồng riêng** (`caro_ai.c`) nên giao diện không bao giờ đứng hình khi máy nghĩ.
Nền tảng là đánh giá theo **bộ năm ô**: mọi đoạn 5 ô liên tiếp trên bàn, đoạn chỉ còn giá trị với
một bên khi bên kia chưa có quân trong đó. Đặt một quân chỉ đổi tối đa 20 đoạn nên cập nhật rất rẻ.

| Mức | Cách chơi |
|---|---|
| Dễ | Tham lam theo điểm bộ năm nhưng xem nhẹ phòng thủ, chọn ngẫu nhiên trong 5 nước tốt nhất, đôi khi bỏ sót nước thắng / nước chặn |
| Thường | Tham lam đầy đủ công + thủ, luôn thắng khi có thể và luôn chặn hàng 4 |
| Khó | Tìm chuỗi **tứ liên tiếp buộc thắng (VCF)**, sau đó **alpha-beta đào sâu dần** (độ sâu chẵn, tỉa còn 7–14 nước ứng viên mỗi tầng) trong ~1,6 giây |

Kiểm thử máy đấu máy: Thường thắng Dễ 6-0, Khó thắng Thường 6-0.

## Thống kê

Số ván thắng / thua / hoà với máy lưu riêng từng độ khó trong file `caro.txt` của thư mục dữ liệu
chung (khoá `win.easy`, `loss.normal`, …). Khoá `best` là **tổng số ván thắng máy** — Arcade Hub
hiển thị con số này ở dòng "Thắng máy".

## Cấu trúc mã

```text
caro/
├── inc/  caro_types.h   Kiểu dữ liệu (bàn cờ, trạng thái game, hạt hiệu ứng)
│         caro_rules.h   Luật: đặt / hoàn tác / kiểm tra thắng (không phụ thuộc đồ hoạ)
│         caro_ai.h      AI trên luồng riêng: gửi yêu cầu - hỏi kết quả
│         caro_assets.h  Vân gỗ, shader quân X/O, âm thanh
│         caro_draw.h    Vẽ + mọi hình chữ nhật bấm được (dùng chung với logic)
│         caro_game.h    Vòng đời game, input
│         caro_runner.h  API cho Arcade Hub
└── src/  (cùng tên) + main.c (bản chạy độc lập, bỏ qua khi build qua Hub)
```

Build và chạy riêng:

```bash
./build_and_run.sh
```
