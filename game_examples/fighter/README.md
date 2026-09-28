# Đấu Sĩ — game đối kháng 2D (C++17)

Game đối kháng theo phong cách arcade cổ điển: hai đấu sĩ, ba hiệp, thanh máu
vàng chạy về giữa màn hình, đồng hồ đếm ngược và chữ **K.O.** khi kết liễu.

Khác với các example còn lại trong repo (viết bằng C thuần), game này viết bằng
**C++17 hướng đối tượng**, vì bài toán "nhiều nhân vật, mỗi người một bộ chiêu"
chính là chỗ kế thừa và hàm ảo phát huy tác dụng.

Toàn bộ hình hoạ là **sprite pixel-art có sẵn, giấy phép CC0** (xem
`../assets/fighter/LICENSES.md`), không còn vẽ tay bằng lệnh `DrawCircle`.

---

## Điều khiển

**Chơi 1 người / luyện tập** — dùng bộ nào cũng được:

| | Bộ WASD | Bộ mũi tên |
|---|---|---|
| Đi / nhảy / ngồi | `A` `D` / `W` / `S` | `←` `→` / `↑` / `↓` |
| Đòn nhẹ · vừa · mạnh | `J` `K` `L` | `Z` `X` `C` |
| Chiêu nhanh (Special) | `I` | `V` |
| Siêu chiêu | `U` | `B` |
| Đỡ đòn | **giữ hướng lùi** (`Shift` trái) | **giữ hướng lùi** (`Shift` phải) |

**Chơi 2 người** — người 1 chỉ dùng bộ WASD, mũi tên thuộc về người 2:

| | Người 1 | Người 2 |
|---|---|---|
| Đi / nhảy / ngồi | `A` `D` / `W` / `S` | `←` `→` / `↑` / `↓` |
| Đòn nhẹ · vừa · mạnh | `J` `K` `L` | `Num1` `Num2` `Num3` |
| Chiêu nhanh · Siêu chiêu | `I` · `U` | `Num4` · `Num5` |
| Đỡ đòn | giữ lùi (`Shift`) | giữ lùi (`Num0`) |

`P`/`Esc` tạm dừng (có **bảng chiêu**) · `F1` khung va chạm · `F11` toàn màn hình.

### Hệ thống chiến đấu (theo Street Fighter)

| Thao tác | Cách bấm |
|---|---|
| Đỡ đứng / đỡ ngồi | Giữ `←` chặn đòn giữa + đòn trên (đòn nhảy); giữ `↙` chặn đòn giữa + đòn thấp (quét chân) |
| Đòn ngồi / đòn nhảy | `↓` + đòn / đang nhảy + đòn (đòn nhảy là **đòn trên**, phải đứng đỡ) |
| Nối đòn (chain) | `J → K → L` khi đang trúng / bị đỡ |
| Hủy đòn (cancel) | Đòn thường trúng → lệnh chiêu đặc biệt → `U` siêu chiêu |
| Vật / phá vật | `J + K` cùng lúc sát người; bị vật thì bấm `J + K` kịp để phá |
| Lướt | `→ →` / `← ←` (lướt lùi có vài frame bất tử) |
| Chiêu đặc biệt | `↓↘→`, `↓↙←`, `→↓↘`, `↓ ↓` + đòn — lực L/M/H đổi tốc độ, tầm xa |
| Chiêu nhanh | `I`, `→+I`, `↓+I`, `←+I` (cho người chưa quen quay tay) |
| Siêu chiêu | `U` hoặc `↓↘→↓↘→` + đòn — tốn 1 trong 2 thanh Super, đứng hình + cut-in |

Ngoài ra: counter hit (đánh trúng lúc đối thủ đang vung đòn), giảm sát thương
theo độ dài combo, chip damage khi đỡ chiêu đặc biệt, tung hứng có giới hạn,
ngã–đứng dậy bất tử, hai quả đạn va nhau thì triệt tiêu, camera bám theo trên
sàn rộng gấp gần 2 lần màn hình, dồn góc tường.

Nhân vật luôn **quay mặt về phía đối thủ**; đi lùi phát animation chạy ngược
nên chân bước lùi khớp với hướng di chuyển. Nhảy qua đầu đối thủ thì tới lúc
chạm đất mới quay lại.

---

## Kiến trúc

```
Game ──► Scene (ảo)
           ├─ TitleScene     màn tiêu đề
           ├─ SelectScene    chọn nhân vật + độ khó + chế độ + bảng chiêu  ← "màn Settings"
           └─ BattleScene    luật trận, âm thanh, cut-in siêu chiêu, luyện tập
                  │
                  ├─ Arena            thế giới trận đấu: va chạm, đạn, hiệu ứng
                  │     ├─ Fighter (ảo) ──► Samurai · Kenji · Knight · Wizard
                  │     ├─ Projectile (ảo) ──► Wave · Shuriken · Orb · Shockwave · Pillar · Lightning
                  │     └─ ParticleSystem
                  └─ Hud              thanh máu, thanh Super, đồng hồ, combo
```

Bốn ranh giới quan trọng:

**`Scene`** — mỗi màn hình là một lớp con. `Game` chỉ giữ một `Scene*` và gọi
`Update`/`Draw`, nên thêm màn hình mới (bảng xếp hạng, chế độ luyện tập) không
phải đụng vào vòng lặp chính. Việc đổi scene được hoãn tới cuối frame để không
xoá đối tượng đang chạy dở.

**`Fighter`** — lớp cơ sở nắm mọi thứ *chung*: vật lý, máy trạng thái, hitbox,
đỡ đòn, combo, vẽ sprite. Cái *riêng* của từng nhân vật nằm ở các hàm ảo:

| Hàm ảo | Vai trò |
|---|---|
| `Special(MoveId, Strength)` | **bắt buộc** — frame data 4 chiêu đặc biệt × 3 lực |
| `SuperMove()` | **bắt buộc** — siêu chiêu |
| `Normal(MoveId)` | đòn thường; mặc định dựng từ `NormalProfile` |
| `CanUseSpecial` | ví dụ: mỗi lúc chỉ một quả đạn |
| `OnMoveStart / OnMoveActive / OnMoveUpdate / OnMoveLand` | bắn đạn, dịch chuyển, xoay... |
| `OnMoveHit` / `OnIncomingHit` | khi trúng đòn / thế phản đòn |
| `UpdateCharacter`, `DrawBehind`, `DrawFront` | hào quang, bóng mờ riêng |

**`MoveDef`** — một đòn đánh được mô tả bằng dữ liệu chứ không phải câu lệnh:
`startup` (vung tay) → `active` (hitbox bật) → `recovery` (thu đòn), kèm hitbox,
độ cao (giữa / thấp / trên / vật), quyền hủy đòn, vận tốc lao/vọt, khoảng bất
tử, giáp, hiệu ứng vệt chém. Cân bằng lại một nhân vật là sửa vài con số.

**Sự kiện** — `Arena` không phát âm thanh; nó đẩy `GameEvent` (trúng, đỡ, vật,
counter, K.O....) vào hàng đợi, `BattleScene` đọc ra để phát tiếng và hiện chữ.

**Tái dụng sprite** — pack CC0 chỉ có 8 animation, các tư thế còn thiếu được
dựng lại: ngồi = idle nén dọc 72%, đi lùi = chạy phát ngược, đứng dậy = ngã
phát ngược, đòn nhẹ = nửa đầu đòn chém, lộn nhào = xoay sprite.

**`Controller`** — người thật (`HumanController`) và máy (`AIController`) đều chỉ
sinh ra một `InputState`. `Fighter` không bao giờ gọi `IsKeyDown` trực tiếp, nên
cả hai đi chung một đường xử lý — máy không thể "gian lận" bằng đường tắt.

---

## Bốn nhân vật — 20 chiêu đặc biệt

| Nhân vật | `↓↘→` | `↓↙←` | `→↓↘` | `↓ ↓` | Siêu chiêu |
|---|---|---|---|---|---|
| **MACK** — cân bằng | Kiếm Khí (đạn) | Toàn Phong Trảm (xoáy) | Thăng Long Trảm (chém vọt, bất tử) | Phản Kiếm (thế phản đòn) | Tam Liên Trảm |
| **KENJI** — áp sát | Phi Tiêu (H: 3 chiếc) | Ảnh Bộ (lướt xuyên) | Ưng Trảo (bổ nhào, đòn trên) | Thế Thân (dịch chuyển) | Loạn Ảnh Kiếm (7 hit) |
| **GARETH** — đô vật | Khiên Xung (có giáp) | Trảm Địa (bổ + chấn động) | Nộ Kích (chém vọt có giáp) | Địa Ngục Quăng (vật lệnh) | Địa Chấn (sóng đất) |
| **MALTHUS** — giữ khoảng cách | Hắc Cầu | Hắc Trụ (cột ở xa) | Hắc Bạo (bộc phá) | Dịch Chuyển | Thiên Phạt (5 tia sét) |

Mỗi nhân vật còn 9 đòn thường (đứng/ngồi/nhảy × nhẹ/vừa/mạnh) và đòn vật.

**Chế độ:** 1 người đấu máy (4 độ khó), 2 người, và **Luyện tập** (hình nộm
đứng / ngồi / tự đỡ / nhảy, máu và Super hồi đầy, `R` đặt lại vị trí).

**Âm thanh:** xướng ngôn (ROUND 1, FIGHT, YOU WIN...), tiếng va chạm theo
lực đòn, tiếng hét khi tung chiêu / trúng đòn / gục ngã, nhạc nền. Tiếng gió
vung đòn, phép thuật, sét được tổng hợp bằng code trong `core/Audio.cpp`.

---

## Thêm một nhân vật mới

Màn chọn nhân vật tự chia trang theo `Roster::Count()`, nên **không cần sửa giao
diện**. Ba bước:

**1. Chép sprite.** Tạo `../assets/fighter/characters/<id>/` với đúng 8 file:

```
idle.png  run.png  jump.png  fall.png  attack1.png  attack2.png  takehit.png  death.png
```

Mỗi file là một dải ngang, mỗi frame là hình vuông cạnh bằng chiều cao ảnh. Số
frame được suy ra từ kích thước ảnh nên không phải khai báo.

Nguồn miễn phí hợp lệ: các pack CC0 của [LuizMelo](https://luizmelo.itch.io/)
(Martial Hero, Hero Knight, Evil Wizard, Huntress...) — đúng bố cục này.

**2. Viết lớp con.** Khai báo trong `inc/characters/Characters.hpp`, cài đặt
trong một file mới `src/characters/<Tên>.cpp` (xem `Samurai.cpp` làm mẫu):

```cpp
class Monk final : public Fighter {
public:
    Monk(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}
protected:
    MoveDef Special(MoveId id, Strength s) const override;   // bắt buộc
    MoveDef SuperMove() const override;                      // bắt buộc
    void OnMoveActive(Arena &arena, const MoveDef &m) override;  // tuỳ chọn
};
```

**3. Khai báo vào roster.** Thêm một khối `Add({...})` trong `Roster::Load()`
(`src/characters/Roster.cpp`), kèm `moveList` để bảng chiêu hiển thị — có sẵn 4
ví dụ để copy.

Hai giá trị duy nhất phải canh bằng mắt khi dùng pack mới:

* `anchor` — toạ độ *trong frame gốc* ứng với điểm giữa hai bàn chân
* `scale` — phóng to bao nhiêu lần để nhân vật cao khoảng 190 px (toạ độ thế giới)
* `normals.slashFrom` — frame đầu tiên có vệt chém vẽ sẵn trong sprite (-1 nếu không)

Mẹo: bật `F1` trong trận để thấy khung va chạm, chỉnh `anchor` cho tới khi sprite
đứng khớp với hurtbox.

---

## Build và chạy

```bash
cd game_examples/fighter && ./build_and_run.sh
```

Hoặc qua Arcade Hub (`game_examples/game_boot`), nơi game này được nạp như một
mục trong danh sách. Hub viết bằng C nên `inc/fighter_runner.h` bọc `extern "C"`
để bốn hàm `Init/Update/Draw/CloseFighterApp` gọi được từ phía C.

Game tự dò thư mục assets bằng cách đi ngược vài cấp từ thư mục chạy, nên chạy
từ `build/` hay từ gốc repo đều được.
