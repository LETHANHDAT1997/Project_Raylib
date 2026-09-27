# Nguồn tài nguyên (Asset credits)

Toàn bộ tài nguyên trong thư mục này là **miễn phí, dùng được cho mục đích thương mại**.
Không có sprite nào được rip từ game có bản quyền (Street Fighter, KOF, ...).

## Nhân vật — LuizMelo (CC0 / Creative Commons Zero)

| Thư mục | Pack gốc | Trang tải |
|---|---|---|
| `characters/samurai` | Martial Hero      | https://luizmelo.itch.io/martial-hero |
| `characters/kenji`   | Martial Hero 2    | https://luizmelo.itch.io/martial-hero-2 |
| `characters/knight`  | Hero Knight       | https://luizmelo.itch.io/hero-knight |
| `characters/wizard`  | EVil Wizard 2     | https://luizmelo.itch.io/evil-wizard-2 |

Giấy phép gốc đi kèm mỗi pack: *"This pack is Creative Commons Zero (CC-0).
Can be used in commercial and non-commercial projects."*

Tên file đã được chuẩn hoá về chữ thường (`Take Hit.png` -> `takehit.png`) để code nạp
đồng nhất, nội dung ảnh giữ nguyên.

## Bối cảnh — ansimuz

| Thư mục | Pack gốc | Trang tải |
|---|---|---|
| `backgrounds/mountain_dusk` | Mountain Dusk Parallax Background | https://ansimuz.itch.io/mountain-dusk-parallax-background |

## Thêm nhân vật mới

1. Tải một pack có đủ 8 animation: `idle, run, jump, fall, attack1, attack2, takehit, death`.
2. Tạo thư mục `characters/<ten_moi>/` và đặt tên file theo đúng 8 tên trên.
3. Khai báo nhân vật trong `src/characters/Roster.cpp` (xem hướng dẫn trong file đó).

Sprite sheet phải là dải ngang, mỗi frame là hình vuông cạnh bằng chiều cao ảnh.
