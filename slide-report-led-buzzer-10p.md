# Nội dung báo cáo slide 10 phút: Điều khiển LED và Buzzer cho xe robot

## 1. Mục tiêu báo cáo

Trình bày cơ chế điều khiển **đèn giao thông LED đỏ/xanh** và **buzzer cảnh báo** trong hệ thống xe robot.

Mục tiêu chính:

- Điều khiển trạng thái **đèn đỏ/đèn xanh** từ giao diện web.
- Khi đèn đỏ, xe bị ép dừng để đảm bảo an toàn.
- Điều khiển **buzzer** thủ công hoặc tự động khi xe gặp vật cản.
- Làm rõ sơ đồ nối dây để không nhầm giữa **GPIO BCM** và **số chân vật lý Raspberry Pi**.

---

## 2. Kiến trúc tổng thể

```text
Web Dashboard
     |
     | WebSocket / REST API
     v
Backend FastAPI trên Raspberry Pi
     |
     | GPIO điều khiển LED + buzzer + button
     v
Breadboard LED/Buzzer

Backend FastAPI
     |
     | USB Serial JSON 115200 baud
     v
Arduino Uno trên xe robot
     |
     v
Motor / Servo / Cảm biến / Logic dừng xe
```

Vai trò từng phần:

| Thành phần | Vai trò |
|---|---|
| Web Dashboard | Giao diện bật/tắt đèn, buzzer, xem trạng thái xe |
| Raspberry Pi Backend | Xử lý WebSocket/API, điều khiển GPIO, gửi lệnh xuống Arduino |
| Arduino Firmware | Điều khiển chuyển động xe, nhận trạng thái đèn đỏ/xanh |
| Breadboard | Mạch LED đỏ, LED xanh, buzzer, button |

---

## 3. Điều khiển đèn giao thông

### Luồng hoạt động

```text
Người dùng chọn RED/GREEN trên web
        |
        v
Frontend gửi WebSocket
        |
        v
Backend đổi LED vật lý qua GPIO
        |
        v
Backend gửi lệnh Serial N=50 xuống Arduino
        |
        v
Arduino cập nhật trạng thái traffic_light_green
```

### Lệnh gửi xuống Arduino

```json
{"N":50,"D1":1,"H":1}
```

- `D1 = 1`: đèn xanh, xe được phép chạy.
- `D1 = 0`: đèn đỏ, xe bị ép dừng.

### Ý nghĩa an toàn

Khi đèn đỏ:

- Arduino gọi lệnh dừng xe.
- Các chế độ điều khiển như manual, line tracking, obstacle avoidance, hand tracking đều bị chặn chuyển động.
- Đèn đỏ không chỉ là hiển thị, mà là một lớp **khóa an toàn phần mềm**.

---

## 4. Điều khiển buzzer

Buzzer được điều khiển bằng Raspberry Pi qua GPIO.

Có 3 cách bật buzzer:

1. **Thủ công từ web**: người dùng nhấn nút Buzzer.
2. **Tự động từ backend**: nếu xe ở chế độ tránh vật cản và khoảng cách nhỏ hơn 25 cm.
3. **Sự kiện từ Arduino**: khi thuật toán tránh vật cản cần cảnh báo, Arduino gửi event `buzzer_on` hoặc `buzzer_off`.

Luồng buzzer thủ công:

```text
Click Buzzer trên web
        |
        v
WebSocket: {"type":"buzzer","on":true}
        |
        v
Backend set GPIO22 = HIGH
        |
        v
Buzzer kêu
```

Luồng buzzer tự động:

```text
Xe phát hiện vật cản gần
        |
        v
Arduino / Backend xác định cần cảnh báo
        |
        v
GPIO22 bật buzzer
        |
        v
Frontend nhận telemetry buzzer=true
```

---

## 5. Sơ đồ nối dây quan trọng

Backend dùng chuẩn **BCM GPIO**, không phải số chân vật lý.

| Thiết bị | GPIO BCM trong code | Chân vật lý Raspberry Pi | Chức năng |
|---|---:|---:|---|
| LED đỏ | GPIO17 | Pin 11 | Báo dừng xe |
| LED xanh | GPIO27 | Pin 13 | Báo xe được chạy |
| Buzzer | GPIO22 | Pin 15 | Cảnh báo âm thanh |
| Button | GPIO23 | Pin 16 | Đảo đỏ/xanh |
| GND | GND | Pin 6/9/14/20/... | Mass chung |

### Nối LED

```text
GPIO17 / Pin 11 ---- điện trở ---- LED đỏ ---- GND
GPIO27 / Pin 13 ---- điện trở ---- LED xanh -- GND
```

Điện trở khuyến nghị: **220Ω - 1kΩ**, thường dùng **330Ω**.

### Nối buzzer

Cách đơn giản:

```text
GPIO22 / Pin 15 ---- (+) Buzzer
GND ---------------- (-) Buzzer
```

Nếu buzzer dòng lớn, nên dùng transistor NPN để bảo vệ GPIO.

### Nối button

```text
GPIO23 / Pin 16 ---- Button ---- GND
```

Backend đã bật pull-up nội, nên:

- Không nhấn: HIGH.
- Nhấn: LOW.
- Khi nhấn, hệ thống đảo trạng thái đèn đỏ/xanh.

---

## 6. Demo đề xuất trong 10 phút

### Phân bổ thời gian

| Thời lượng | Nội dung |
|---:|---|
| 1 phút | Giới thiệu mục tiêu: LED giao thông và buzzer cảnh báo |
| 2 phút | Trình bày kiến trúc Web → Backend → GPIO/Arduino |
| 2 phút | Giải thích logic đèn đỏ/xanh và lệnh `N=50` |
| 1 phút | Giải thích buzzer thủ công/tự động |
| 1 phút | Trình bày sơ đồ nối dây GPIO |
| 3 phút | Demo thực tế |

### Kịch bản demo

1. Mở web dashboard.
2. Bấm chuyển sang **đèn đỏ**:
   - LED đỏ sáng.
   - LED xanh tắt.
   - Xe dừng hoặc không nhận lệnh chạy.
3. Bấm chuyển sang **đèn xanh**:
   - LED xanh sáng.
   - LED đỏ tắt.
   - Xe được phép chạy lại.
4. Bấm nút **Buzzer** trên web:
   - Buzzer kêu.
   - Trạng thái buzzer trên dashboard thay đổi.
5. Nếu có thời gian, demo chế độ tránh vật cản:
   - Đưa vật cản gần xe.
   - Xe xử lý tránh vật cản.
   - Buzzer tự bật cảnh báo.
6. Nhấn button vật lý:
   - Trạng thái đèn đảo giữa đỏ và xanh.

---

## 7. Điểm cần nhấn mạnh khi thuyết trình

- Hệ thống không chỉ bật/tắt LED, mà còn đồng bộ trạng thái xuống Arduino để điều khiển hành vi xe.
- Đèn đỏ là cơ chế an toàn: xe bị ép dừng ở tầng firmware.
- Buzzer có thể điều khiển thủ công hoặc tự động theo tình huống nguy hiểm.
- Raspberry Pi dùng **GPIO BCM**, vì vậy phải đối chiếu đúng với **số chân vật lý** khi nối dây.
- Thiết kế tách rõ nhiệm vụ:
  - Raspberry Pi: web, API, GPIO.
  - Arduino: điều khiển xe thời gian thực.

---

## 8. Kết luận ngắn

Phần điều khiển LED và buzzer giúp xe robot có khả năng phản hồi trực quan và âm thanh. LED đỏ/xanh đóng vai trò như hệ thống đèn giao thông điều khiển quyền di chuyển của xe, còn buzzer đóng vai trò cảnh báo khi có thao tác thủ công hoặc tình huống tránh vật cản. Toàn bộ hệ thống được điều khiển từ web, xử lý qua Raspberry Pi và đồng bộ xuống Arduino để đảm bảo xe hoạt động an toàn hơn.
