# Hand Tracking Mode — Tài liệu kỹ thuật

## 1. Tổng quan

Xe robot sử dụng **cảm biến siêu âm** (HC-SR04, chân TRIG=A5, ECHO=A4) hướng về phía trước để đo khoảng cách đến bàn tay. Hệ thống chạy vòng lặp phản hồi mỗi **50ms** (`HT_UPDATE_INTERVAL`), liên tục đọc khoảng cách → lọc nhiễu → tính hướng & tốc độ → điều khiển motor.

**Mục tiêu:** Xe luôn cố gắng giữ khoảng cách **20cm** (`HT_TARGET_DIST`) với bàn tay. Tay đưa lại gần → xe lùi. Tay rút xa → xe tiến theo.

## 2. Kích hoạt chế độ

- Backend gửi lệnh JSON `{N: 3, D1: 3}` qua Serial/Bluetooth.
- Arduino set `func_mode = HandTracking` (line 1089).
- Hàm `hand_tracking_mode()` được gọi mỗi vòng `loop()`, nhưng chỉ thực thi khi `func_mode == HandTracking`.

## 3. Khởi tạo lần đầu (`ht_first_entry`)

Khi vào chế độ lần đầu:
1. Xoay servo về **90°** (hướng thẳng phía trước).
2. Đọc 1 mẫu khoảng cách thô làm giá trị khởi tạo cho bộ lọc EMA.
3. Reset toàn bộ biến trạng thái: `ht_current_speed = 0`, `ht_active_direction = 0` (dừng), `ht_change_counter = 0`, `ht_last_moving_dir = 0`.
4. Gọi `stop()` để đảm bảo motor tắt.

Khi thoát chế độ (`func_mode != HandTracking`), flag `ht_first_entry` được set lại `true` để lần vào sau sẽ khởi tạo lại.

## 4. Bộ lọc EMA (Exponential Moving Average)

Cảm biến siêu âm dễ bị nhiễu (dội sóng ảo, mất tín hiệu). Bộ lọc EMA làm mượt giá trị:

```
ema_distance = α × raw_dist + (1 - α) × ema_distance_cũ
```

- **α = 0.45** (`HT_EMA_ALPHA`): Cân bằng giữa phản hồi nhanh và chống nhiễu.
- `dist = (int)ema_distance` được dùng cho mọi quyết định điều khiển.

**Xử lý mất tín hiệu:** Nếu `raw_dist == 0` (không nhận echo) hoặc `raw_dist > 50cm` (ngoài phạm vi), EMA được reset về 20cm (= `HT_TARGET_DIST`) → xe dừng ngay, tránh bị kéo lùi do giá trị EMA cũ.

## 5. Phân vùng khoảng cách & trạng thái

Dựa trên `dist` (giá trị đã lọc EMA), hệ thống xác định **hướng mong muốn** (`desired_direction`) và **tốc độ mục tiêu** (`target_speed`):

| Vùng | Khoảng cách | Hướng | Tốc độ PWM | Mô tả |
|------|-------------|-------|------------|-------|
| Mất tín hiệu | `raw = 0` hoặc `> 50cm` | DỪNG (0) | 0 | Rút tay khỏi vùng kiểm soát |
| Nguy hiểm | `< 10cm` | LÙI (2) | Cố định **140** | Lùi tốc độ cao (nhưng giới hạn ở 140, không dùng max 180 để giảm trượt) |
| Gần | `10cm → lower_bound` | LÙI (2) | Tỷ lệ **80-140** | `error = lower_bound - dist`, map tuyến tính |
| Deadzone | `lower_bound → upper_bound` | DỪNG (0) | 0 | Xe đứng yên |
| Xa | `upper_bound → 50cm` | TIẾN (1) | Tỷ lệ **80-180** | `error = dist - upper_bound`, map tuyến tính |

> **Lưu ý:** `lower_bound` và `upper_bound` **không cố định** mà thay đổi theo trạng thái (xem mục 6).

## 6. Bù trễ động & Hysteresis

### 6.1 Bù trễ động (Dynamic Lag Compensation)

Khi xe **đang di chuyển**, do quán tính cơ học + trễ bộ lọc EMA, xe thường trôi quá mục tiêu. Giải pháp: **thu hẹp deadzone** để xe phanh sớm hơn.

| Trạng thái xe | lower_bound | upper_bound | Giải thích |
|---------------|-------------|-------------|-----------|
| Đang lùi (`ht_active_direction = 2`) | 16 + 2 = **18cm** | 24cm | Phanh sớm 2cm trước khi vào deadzone |
| Đang tiến (`ht_active_direction = 1`) | 16cm | 24 - 2 = **22cm** | Phanh sớm 2cm trước khi vào deadzone |
| Đang dừng | 16cm | 24cm | Giữ nguyên (không bù) |

### 6.2 Hysteresis chống dao động

Khi xe **vừa dừng lại**, bàn tay thường vẫn rung nhẹ quanh biên deadzone → xe giật tiến/lùi liên tục. Giải pháp: **mở rộng deadzone** CHỈ về phía hướng xe vừa chạy.

| Hướng cuối cùng trước khi dừng | lower_bound | upper_bound | Hiệu ứng |
|-------------------------------|-------------|-------------|----------|
| Vừa lùi xong (`ht_last_moving_dir = 2`) | 16 - 3 = **13cm** | 24cm | Cần đưa tay gần hơn 13cm mới kích hoạt lùi lại |
| Vừa tiến xong (`ht_last_moving_dir = 1`) | 16cm | 24 + 3 = **27cm** | Cần rút tay xa hơn 27cm mới kích hoạt tiến lại |

## 7. Logic chuyển hướng (`ht_active_direction`)

Biến `ht_active_direction` quản lý hướng thực tế đang chạy: `0` = dừng, `1` = tiến, `2` = lùi.

**4 trường hợp xử lý:**

1. **Cùng hướng** (`desired == active`): Tiếp tục chạy, reset `ht_change_counter = 0`.

2. **Muốn dừng** (`desired == 0`):
   - Cho dừng **ngay lập tức**, không cần chờ.
   - Lưu `ht_last_moving_dir = ht_active_direction` để hysteresis biết hướng cuối.

3. **Khởi động từ dừng** (`active == 0`, `desired ≠ 0`):
   - Cho phép chạy **ngay lập tức**, KHÔNG cần chờ stable count.
   - Xóa `ht_last_moving_dir = 0` (hết hysteresis).

4. **Đảo hướng** (tiến↔lùi, `active ≠ 0`, `desired ≠ active`, `desired ≠ 0`):
   - Tăng `ht_change_counter++`.
   - Phải đếm đủ **3 lần liên tục** (`HT_STABLE_COUNT`) mới cho đảo.
   - Trong khi chờ: `target_speed = 0` (buộc dừng, chờ xác nhận).
   - Mục đích: Chống xung điện ngược nguy hiểm khi motor đảo chiều đột ngột.

## 8. Ramping — Tăng/giảm tốc mượt

Tốc độ thực tế (`ht_current_speed`) không nhảy ngay lên `target_speed` mà thay đổi dần:

- **Tăng tốc:** `+25 PWM/chu kỳ` (`HT_RAMP_STEP`) → bảo vệ bánh răng, tránh sụt áp.
- **Giảm tốc:** `-50 PWM/chu kỳ` (`HT_RAMP_STEP × 2`) → giảm nhanh hơn tăng.
- **Dừng hẳn:** Nếu `target_speed == 0` → set `ht_current_speed = 0` ngay lập tức (phanh cứng).

## 9. Thực thi motor

Sau khi tính xong `ht_active_direction` và `ht_current_speed`:

| `ht_active_direction` | Hành động | Hàm gọi |
|-----------------------|-----------|---------|
| 0 | Tắt motor, `mov_mode = STOP` | `stop()` |
| 1 | Chạy tiến, `mov_mode = FORWARD` | `forward(false, ht_current_speed)` |
| 2 | Chạy lùi, `mov_mode = BACK` | `back(false, ht_current_speed)` |

## 10. Bảng hằng số cấu hình

| Hằng số | Giá trị | Đơn vị | Ý nghĩa |
|---------|---------|--------|---------|
| `HT_TARGET_DIST` | 20 | cm | Khoảng cách mục tiêu giữ với tay |
| `HT_DEADZONE` | 4 | cm | Bán kính vùng chết (±4cm → 16-24cm) |
| `HT_HYSTERESIS` | 3 | cm | Mở rộng deadzone sau khi dừng |
| `HT_SAFE_MIN` | 10 | cm | Dưới mức này → lùi tốc độ tối đa |
| `HT_MAX_RANGE` | 50 | cm | Trên mức này → dừng xe |
| `HT_SPEED_MIN` | 80 | PWM | Tốc độ tối thiểu khi di chuyển |
| `HT_SPEED_MAX` | 180 | PWM | Tốc độ tối đa khi tiến (lùi giới hạn 140) |
| `HT_EMA_ALPHA` | 0.45 | — | Hệ số lọc EMA |
| `HT_RAMP_STEP` | 25 | PWM/chu kỳ | Bước tăng tốc mỗi 50ms |
| `HT_UPDATE_INTERVAL` | 50 | ms | Chu kỳ cập nhật |
| `HT_STABLE_COUNT` | 3 | lần | Số mẫu ổn định trước khi đảo chiều |

## 11. Sơ đồ luồng xử lý mỗi chu kỳ (50ms)

```
loop() gọi hand_tracking_mode()
  │
  ├─ func_mode ≠ HandTracking? → return
  ├─ Lần đầu? → Khởi tạo servo, EMA, biến trạng thái
  ├─ Chưa đủ 50ms? → return (rate limiting)
  │
  ├─ Đọc raw_dist từ cảm biến siêu âm
  ├─ raw_dist = 0 hoặc > 50? → Reset EMA = 20, dừng xe
  ├─ Ngược lại → Cập nhật EMA
  │
  ├─ Tính lower_bound, upper_bound (bù trễ + hysteresis)
  ├─ Xác định desired_direction & target_speed theo vùng
  │
  ├─ Logic chuyển hướng:
  │   ├─ Cùng hướng → tiếp tục
  │   ├─ Muốn dừng → dừng ngay
  │   ├─ Khởi động từ dừng → chạy ngay
  │   └─ Đảo hướng → chờ 3 lần ổn định
  │
  ├─ Ramping: tăng +25 / giảm -50 / dừng = 0 ngay
  └─ Gọi forward() / back() / stop()
```
