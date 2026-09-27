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

| | Người 1 | Người 2 |
|---|---|---|
| Di chuyển | `A` `D` | `←` `→` |
| Nhảy | `W` | `↑` |
| Ngồi | `S` | `↓` |
| Đỡ đòn | `Shift` (hoặc giữ lùi + `S`) | `Numpad 0` |
| Đòn nhanh | `J` | `Numpad 1` |
| Đòn mạnh | `K` | `Numpad 2` |
| Chiêu riêng | `L` | `Numpad 3` |
| Chiêu cuối | `U` | `Numpad 5` |

`P` hoặc `Esc` tạm dừng · `F1` bật khung va chạm (debug) · `F11` toàn màn hình.

Chiêu cuối chỉ tung được khi thanh Super (thanh xanh dưới tên) đã đầy. Thanh này
dâng lên khi bạn đánh trúng, đỡ đòn hoặc ăn đòn.

---

## Kiến trúc

```
Game ──► Scene (ảo)
           ├─ TitleScene     màn tiêu đề
           ├─ SelectScene    chọn nhân vật + độ khó   ← "màn Settings"
           └─ BattleScene    luật trận: hiệp, đồng hồ, K.O., tạm dừng
                  │
                  ├─ Arena            thế giới trận đấu: va chạm, đạn, hiệu ứng
                  │     ├─ Fighter (ảo) ──► Samurai · Kenji · Knight · Wizard
                  │     ├─ Projectile (ảo) ──► OrbProjectile · ShockwaveProjectile · SlashProjectile
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
| `Move(MoveSlot)` | **bắt buộc** — frame data của 4 ô chiêu |
| `OnSpecialActivate(Arena&)` | chạy đúng lúc hitbox chiêu riêng bật lên |
| `OnSuperActivate(Arena&)` | tương tự cho chiêu cuối |
| `OnHitConfirm(...)` | hiệu ứng khi đòn chạm đối thủ |
| `UpdateCharacter(...)` | bộ đếm/hào quang riêng mỗi frame |
| `DrawBehind` / `DrawFront` | vẽ thêm lớp sau/trước sprite |

**`MoveDef`** — một đòn đánh được mô tả bằng dữ liệu chứ không phải câu lệnh:
`startup` (vung tay) → `active` (hitbox bật) → `recovery` (thu đòn), kèm hitbox,
sát thương, lực đẩy, số lần trúng, có armor hay không. Cân bằng lại một nhân vật
là sửa vài con số, không phải sửa logic.

**`Controller`** — người thật (`HumanController`) và máy (`AIController`) đều chỉ
sinh ra một `InputState`. `Fighter` không bao giờ gọi `IsKeyDown` trực tiếp, nên
cả hai đi chung một đường xử lý — máy không thể "gian lận" bằng đường tắt.

---

## Bốn nhân vật

| Nhân vật | Lối đánh | Chiêu riêng (`L`) | Chiêu cuối (`U`) |
|---|---|---|---|
| **MACK** | cân bằng, dễ chơi | Thăng Long Trảm — vọt lên, hất tung, trị người hay nhảy | Tam Liên Trảm — lao tới, 3 nhát + khí kiếm |
| **KENJI** | nhanh nhất, máu mỏng | Ảnh Bộ — lướt xuyên qua đối thủ, để lại bóng mờ | Loạn Ảnh Kiếm — 5 nhát liên hoàn |
| **GARETH** | chậm, trâu, đòn nặng có armor | Khiên Xung — húc thẳng, ăn đòn vẫn đi tiếp | Địa Chấn — sóng chạy dọc mặt sàn |
| **MALTHUS** | giữ khoảng cách, bắn từ xa | Hắc Cầu — quả cầu năng lượng | Tam Hắc Cầu — 3 quả bay so le |

Độ khó (Dễ / Thường / Khó / Siêu khó) điều chỉnh thời gian phản ứng, độ lì, khả
năng đỡ đòn, khả năng nối combo và sức chịu đòn của máy — toàn bộ nằm trong một
bảng ở `src/core/Difficulty.cpp`.

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

**2. Viết lớp con.** Trong `inc/characters/Characters.hpp` và `.cpp`:

```cpp
class Monk final : public Fighter {
public:
    Monk(const CharacterDef &def, bool facingRight) : Fighter(def, facingRight) {}
protected:
    const MoveDef &Move(MoveSlot slot) const override;   // bắt buộc
    void OnSpecialActivate(Arena &arena) override;       // tuỳ chọn
};
```

**3. Khai báo vào roster.** Thêm một khối `Add({...})` trong `Roster::Load()`
(`src/characters/Roster.cpp`) — có sẵn 4 ví dụ để copy.

Hai giá trị duy nhất phải canh bằng mắt khi dùng pack mới:

* `anchor` — toạ độ *trong frame gốc* ứng với điểm giữa hai bàn chân
* `scale` — phóng to bao nhiêu lần để nhân vật cao khoảng 190 px trên màn hình

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
