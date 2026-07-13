# Tài Liệu Hướng Dẫn: Chế Độ Theo Dõi Bàn Tay (Hand Tracking Mode - Phase 1)

Chế độ **Hand Tracking** (Theo dõi bàn tay) là một chế độ lái xe tự động được tích hợp vào hệ thống điều khiển của xe robot Elegoo Smart Robot Car V3.0 Plus. Ở chế độ này, xe sử dụng cảm biến siêu âm (ultrasonic sensor) gắn phía trước để đo cự ly bàn tay và tự động điều khiển động cơ tiến/lùi nhằm duy trì một khoảng cách cố định.

---

## 1. Nguyên Lý Hoạt Động

Xe hoạt động dựa trên vòng lặp phản hồi khoảng cách thời gian thực:
* **Khoảng cách mục tiêu (Target Distance):** Mặc định là `20cm`.
* **Vùng chết (Deadzone):** Khoảng `±4cm` xung quanh khoảng cách mục tiêu (`16cm - 24cm`). Trong vùng này, xe sẽ đứng yên.
* **Xe tự động lùi:** Khi bàn tay tiến lại gần xe (`< 16cm`).
* **Xe tự động tiến:** Khi bàn tay rút ra xa xe (`> 24cm`).
* **Dừng khẩn cấp:** Khi cảm biến mất vật cản đột ngột hoặc bàn tay rút ra ngoài phạm vi quét (`> 50cm` hoặc tín hiệu phản hồi bằng `0`).

```
                    [LÙI KHẨN CẤP]            [VÙNG CHẾT]             [TIẾN THEO TAY]
  <--- Lùi Nhanh ---|--- Lùi Tỷ Lệ ---|------- Đứng Yên -------|------- Tiến Tỷ Lệ -------|---> Dừng Xe (Mất tay)
 0cm               10cm              16cm                    24cm                      50cm
```

---

## 2. Các Thuật Toán Tối Ưu Hóa & Chống Dao Động

Để đảm bảo xe chạy mượt mà và không bị giật, tiến lùi liên tục (oscillation), hệ thống sử dụng 4 thuật toán bổ trợ:

### A. Bộ lọc nhiễu số EMA (Exponential Moving Average)
Tránh việc cảm biến đọc sai giá trị tức thời gây giật xe:
$$\text{Distance}_{\text{filtered}} = \alpha \times \text{Distance}_{\text{raw}} + (1 - \alpha) \times \text{Distance}_{\text{filtered\_previous}}$$
* Hệ số $\alpha = 0.45$ được tối ưu hóa để vừa lọc nhiễu tốt, vừa loại bỏ trễ pha giúp phản hồi nhạy bén.

### B. Bù trễ động (Dynamic Lag Compensation)
Do xe có quán tính trượt bánh và bộ lọc có độ trễ truyền dẫn nhỏ, xe dễ bị dừng trễ dẫn đến đâm lố đà (overshoot). 
* **Khi đang lùi:** Ngưỡng dừng dưới nâng lên thành **`18cm`** (dừng sớm hơn 2cm).
* **Khi đang tiến:** Ngưỡng dừng trên hạ xuống thành **`22cm`** (dừng sớm hơn 2cm).
* Khi xe phanh sớm, lực quán tính sẽ đẩy xe trượt thêm một khoảng ngắn và dừng lại **vừa vặn đúng vị trí mục tiêu 20cm**.

### C. Hysteresis hướng (Trễ trạng thái)
Tránh hiện tượng xe vừa dừng đã lập tức chạy lại do nhiễu cảm biến nhỏ:
* Khi xe vừa dừng từ trạng thái lùi, vùng chết phía gần được mở rộng thêm 3cm (xuống `13cm`). Xe chỉ lùi tiếp nếu tay tiến sát hơn `13cm`.
* Khi xe vừa dừng từ trạng thái tiến, vùng chết phía xa được mở rộng thêm 3cm (lên `27cm`). Xe chỉ tiến tiếp nếu tay rút xa hơn `27cm`.

### D. Kiểm soát gia tốc và Phanh khẩn cấp lập tức
* **Khi tăng tốc:** Sử dụng bước tăng tốc `HT_RAMP_STEP = 25` mỗi chu kỳ quét để xe tăng tốc mượt mà, bảo vệ động cơ và nguồn điện.
* **Khi phanh dừng (Speed = 0):** Ngắt PWM động cơ về `0` lập tức để dừng xe ngay, loại bỏ hoàn toàn hiện tượng trượt quá đà.
* **Khi đảo hướng (Tiến ↔ Lùi):** Xe bắt buộc phải đọc tín hiệu ổn định liên tục `3 lần` cùng hướng mới và phải dừng bánh hẳn trước khi quay bánh hướng ngược lại.

---

## 3. Các Thông Số Cấu Chỉ (Calibration)

Các thông số này được định nghĩa dưới dạng `#define` ở ngay đầu file [SmartCar_Core_20210127.ino](file:///home/ddd/project_car_kit/robot_car_kit/SmartCar_Core_20210127/SmartCar_Core_20210127.ino). Bạn có thể dễ dàng thay đổi chúng và nạp lại code:

| Tên Hằng Số | Giá Trị Mặc Định | Đơn Vị | Giải Thích |
|---|---|---|---|
| `HT_TARGET_DIST` | `20` | cm | Khoảng cách lý tưởng xe muốn giữ với bàn tay. |
| `HT_DEADZONE` | `4` | cm | Vùng chết cơ bản (xe đứng yên từ `Target - Deadzone` đến `Target + Deadzone`). |
| `HT_HYSTERESIS` | `3` | cm | Biên độ trễ cộng thêm khi xe đã dừng để chống dao động. |
| `HT_SAFE_MIN` | `10` | cm | Khoảng cách an toàn tối thiểu. Gần hơn mức này xe sẽ lùi tốc độ tối đa. |
| `HT_MAX_RANGE` | `50` | cm | Phạm vi phát hiện tay tối đa. Vượt quá mức này xe coi như mất vật cản. |
| `HT_SPEED_MIN` | `80` | PWM | Tốc độ động cơ tối thiểu (0-255) khi bắt đầu di chuyển. |
| `HT_SPEED_MAX` | `180` | PWM | Tốc độ động cơ tối đa (0-255) khi bám đuổi tay. |
| `HT_EMA_ALPHA` | `0.45` | Hệ số | Độ nhạy bộ lọc (0.0 - 1.0). Càng lớn phản hồi càng nhanh, càng nhỏ càng mượt. |
| `HT_RAMP_STEP` | `25` | PWM | Mức tăng tốc độ tối đa mỗi chu kỳ quét (giúp xe tăng tốc êm ái). |
| `HT_UPDATE_INTERVAL` | `50` | ms | Chu kỳ quét cảm biến siêu âm. |
| `HT_STABLE_COUNT` | `3` | Lần | Số lần đọc ổn định hướng mới trước khi cho phép đảo chiều bánh xe. |

---

## 4. Cách Nạp và Kiểm Tra Chế Độ

1. **Nạp Firmware Arduino:**
   ```bash
   cd robot_car_kit
   pio run -t upload
   ```
2. **Khởi động Backend Serial Bridge:**
   ```bash
   cd backend
   venv/bin/python main.py
   ```
3. **Chạy Frontend Dashboard:**
   ```bash
   cd web_car_kit
   npm run dev
   ```
4. **Mở Web UI:** Truy cập vào Dashboard trên trình duyệt, kết nối cổng Serial, sau đó chuyển sang tab **"Hand Track"** trên bảng điều khiển. Bảng D-Pad điều hướng sẽ được ẩn đi và thay thế bằng panel thông báo trạng thái cảm biến thời gian thực.
5. **Thử nghiệm:** Đưa bàn tay ra phía trước cảm biến siêu âm ở khoảng cách từ 10cm - 40cm và di chuyển tay tiến/lùi để quan sát chuyển động bám đuổi mượt mà của xe.
