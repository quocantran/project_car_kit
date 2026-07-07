# Elegoo Smart Robot Car Kit V3.0 Plus — Tài Liệu Kỹ Thuật Toàn Diện

## 1. Tổng Quan Hệ Thống

Xe robot Elegoo V3.0 Plus là một nền tảng robot di động dựa trên **Arduino UNO R3** (ATmega328P),
được điều khiển qua Bluetooth (app), hồng ngoại (remote), hoặc tự hành (dò line, tránh vật cản).
Hệ thống gồm 7 module chính kết nối với nhau qua board mở rộng (shield).

---

## 2. Danh Sách Linh Kiện

### 2.1 Bộ xử lý trung tâm

| Linh kiện | Mô tả |
|---|---|
| **Arduino UNO R3** | Vi điều khiển ATmega328P, xung nhịp 16MHz, 14 chân digital I/O, 6 chân analog, 6 chân PWM (3,5,6,9,10,11) |

### 2.2 Module điều khiển động cơ

| Linh kiện | Mô tả |
|---|---|
| **L298N H-Bridge** | IC cầu H kép, điều khiển 2 motor DC độc lập. Điện áp logic 5V, điện áp motor 5-35V, dòng tối đa 2A/kênh |
| **4x Motor DC TT** | Motor giảm tốc 3-6V, tốc độ ~200 RPM, gắn trực tiếp vào bánh xe |

### 2.3 Cảm biến

| Linh kiện | Chức năng |
|---|---|
| **HC-SR04 Ultrasonic** | Đo khoảng cách bằng sóng siêu âm (2cm - 400cm), độ chính xác ±3mm |
| **3x IR Tracking Sensor** | Cảm biến hồng ngoại dò line (trái, giữa, phải), phát hiện vạch đen trên nền trắng |
| **IR Receiver (VS1838B)** | Thu tín hiệu hồng ngoại từ remote, giao thức NEC 38KHz |

### 2.4 Module truyền thông

| Linh kiện | Mô tả |
|---|---|
| **HC-08 Bluetooth 4.0 BLE** | Module Bluetooth Low Energy, giao tiếp UART (TX/RX), tốc độ mặc định 9600 baud |

### 2.5 Cơ cấu chấp hành

| Linh kiện | Mô tả |
|---|---|
| **SG90 Servo Motor** | Servo micro 180°, dùng để xoay đầu cảm biến siêu âm quét trái/phải |

### 2.6 Khác

| Linh kiện | Mô tả |
|---|---|
| **IR Remote Control** | Điều khiển từ xa hồng ngoại NEC, 21 phím |
| **Pin 18650 x2** | Nguồn lithium 3.7V x2 = 7.4V, cấp qua jack DC cho shield |
| **LED (pin 13)** | LED onboard Arduino, dùng làm đèn báo trạng thái |
| **Expansion Shield** | Board mở rộng chồng lên Arduino, tập trung tất cả đầu nối module |

---

## 3. Sơ Đồ Kết Nối Chân (Pin Mapping)

### 3.1 Module L298N ← Arduino

```
┌──────────────────────────────────────────────────────────┐
│                    L298N H-Bridge                        │
│                                                          │
│  ENA ←── Pin 5 (PWM)    Tốc độ Motor A (bánh trái)      │
│  IN1 ←── Pin 8          Hướng Motor A                    │
│  IN2 ←── Pin 7          Hướng Motor A                    │
│  IN3 ←── Pin 11         Hướng Motor B (bánh phải)        │
│  IN4 ←── Pin 9          Hướng Motor B                    │
│  ENB ←── Pin 6 (PWM)    Tốc độ Motor B (bánh phải)      │
│                                                          │
│  Motor A ──→ 2 bánh trái  (IN1/IN2 điều hướng)          │
│  Motor B ──→ 2 bánh phải  (IN3/IN4 điều hướng)          │
└──────────────────────────────────────────────────────────┘
```

> **Lưu ý:** Motor A điều khiển 2 bánh bên trái (mắc song song), Motor B điều khiển 2 bánh bên phải.

### 3.2 Cảm biến siêu âm HC-SR04

```
HC-SR04          Arduino
────────         ──────
  VCC    ←────→  5V
  TRIG   ←────→  A5 (Digital OUTPUT)
  ECHO   ←────→  A4 (Digital INPUT)
  GND    ←────→  GND
```

### 3.3 Servo SG90

```
Servo SG90       Arduino
──────────       ──────
  Signal ←────→  Pin 3 (PWM)
  VCC    ←────→  5V
  GND    ←────→  GND
```

### 3.4 Module dò line (IR Tracking)

```
IR Tracking      Arduino         Loại tín hiệu
───────────      ──────          ──────────────
  Left   ←────→  Pin 2  (INPUT)   Digital: HIGH = không có line, LOW = có line
  Middle ←────→  Pin 4  (INPUT)   Digital: HIGH = không có line, LOW = có line
  Right  ←────→  Pin 10 (INPUT)   Digital: HIGH = không có line, LOW = có line
```

> **Lưu ý:** Code đảo logic bằng `!digitalRead()`, nên trong code: `true` = phát hiện vạch đen.

### 3.5 IR Receiver (VS1838B)

```
IR Receiver      Arduino
───────────      ──────
  Signal ←────→  Pin 12 (INPUT)   Digital: Giải mã NEC protocol
  VCC    ←────→  5V
  GND    ←────→  GND
```

### 3.6 Bluetooth HC-08

```
HC-08            Arduino
──────           ──────
  TXD   ←────→  Pin 0 (RX)   ← HC-08 truyền dữ liệu vào Arduino
  RXD   ←────→  Pin 1 (TX)   ← Arduino truyền dữ liệu ra HC-08
  VCC   ←────→  5V
  GND   ←────→  GND
```

> Bluetooth HC-08 chia sẻ cổng **Hardware Serial** (UART) với cổng USB.
> Khi nạp code cần **tháo module BT** để tránh xung đột.

---

## 4. Phân Loại Tín Hiệu Digital vs Analog

### 4.1 Tín hiệu Digital OUTPUT

| Chân | Chức năng | Giá trị |
|---|---|---|
| Pin 7 (IN2) | Hướng Motor A | HIGH (5V) hoặc LOW (0V) |
| Pin 8 (IN1) | Hướng Motor A | HIGH hoặc LOW |
| Pin 9 (IN4) | Hướng Motor B | HIGH hoặc LOW |
| Pin 11 (IN3) | Hướng Motor B | HIGH hoặc LOW |
| A5 (TRIG) | Kích siêu âm | Xung HIGH 10μs để phát sóng |
| Pin 13 | LED trạng thái | HIGH = bật, LOW = tắt |

### 4.2 Tín hiệu PWM (Analog OUTPUT) — Thực chất là Digital điều chế xung

| Chân | Chức năng | Dải giá trị | Ý nghĩa |
|---|---|---|---|
| Pin 5 (ENA) | Tốc độ Motor A | 0 - 255 | 0 = dừng, 255 = tốc độ tối đa |
| Pin 6 (ENB) | Tốc độ Motor B | 0 - 255 | 0 = dừng, 255 = tốc độ tối đa |
| Pin 3 (Servo) | Góc servo | Xung 544-2400μs | Thư viện Servo xử lý tự động |

> **PWM (Pulse Width Modulation):** Arduino tạo sóng vuông tần số ~490Hz (pin 5,6) hoặc ~980Hz.
> `analogWrite(ENA, 150)` → Duty cycle = 150/255 ≈ 59% → Motor quay ở 59% công suất.

### 4.3 Tín hiệu Digital INPUT

| Chân | Nguồn | Mô tả |
|---|---|---|
| Pin 2 | IR Tracking Left | HIGH/LOW: phát hiện vạch đen |
| Pin 4 | IR Tracking Middle | HIGH/LOW: phát hiện vạch đen |
| Pin 10 | IR Tracking Right | HIGH/LOW: phát hiện vạch đen |
| Pin 12 | IR Receiver | Chuỗi xung mã hóa NEC từ remote |
| A4 (ECHO) | HC-SR04 | Xung HIGH, độ rộng tỷ lệ với khoảng cách |

### 4.4 Giao tiếp Serial (UART)

| Chân | Hướng | Tốc độ | Mô tả |
|---|---|---|---|
| Pin 0 (RX) | INPUT | 9600 baud | Nhận dữ liệu JSON từ app qua Bluetooth |
| Pin 1 (TX) | OUTPUT | 9600 baud | Gửi phản hồi (khoảng cách, trạng thái) về app |

---

## 5. Cơ Chế Hoạt Động Bluetooth

### 5.1 Kiến trúc truyền thông

```
┌──────────┐    BLE 4.0     ┌──────────┐    UART 9600    ┌──────────┐
│  App     │ ──────────────→│  HC-08   │ ──────────────→│ Arduino  │
│ (Phone)  │←────────────── │ (BT)     │←────────────── │  UNO     │
└──────────┘   Không dây    └──────────┘   Có dây (RX/TX)└──────────┘
```

1. **App điện thoại** gửi lệnh JSON qua Bluetooth Low Energy
2. **HC-08** nhận sóng BLE, chuyển thành tín hiệu UART (serial)
3. **Arduino** đọc dữ liệu từ `Serial.read()`, parse JSON bằng thư viện ArduinoJson

### 5.2 Giao thức JSON — Cấu trúc lệnh

Mỗi lệnh từ app là một JSON object kết thúc bằng `}`:

```json
{"N":2, "D1":3, "D2":200, "H":1}
```

| Trường | Ý nghĩa |
|---|---|
| `N` | Mã lệnh (command code) — xác định loại điều khiển |
| `D1` | Tham số 1 (hướng, chế độ, góc...) |
| `D2` | Tham số 2 (tốc độ, giá trị phụ) |
| `D3` | Tham số 3 (tốc độ motor riêng lẻ) |
| `T` | Thời gian giới hạn (giây) |
| `H` | Số serial lệnh — dùng để phản hồi xác nhận |

### 5.3 Bảng mã lệnh (Command Code `N`)

| N | Chế độ | D1 | D2 | Mô tả |
|---|---|---|---|---|
| **1** | Motor Control | 0=cả hai, 1=trái, 2=phải | tốc độ (0-250) | Điều khiển từng motor riêng lẻ. D2=hướng(1=tiến,2=lùi,0=dừng), D3=speed |
| **2** | Bluetooth Joystick | 1-9 (hướng) | tốc độ | Điều khiển xe bằng joystick. D1: 1=trái, 2=phải, 3=tiến, 4=lùi, 5=dừng, 6=tiến-trái, 7=lùi-trái, 8=tiến-phải, 9=lùi-phải |
| **3** | Chuyển chế độ | 1 hoặc 2 | — | D1=1: dò line, D1=2: tránh vật cản |
| **4** | Car Control (có timer) | 1-4 (hướng) | tốc độ | Có giới hạn thời gian (T giây) |
| **5** | Reset | — | — | Xóa tất cả chế độ, dừng xe |
| **6** | Servo | góc (5-175) | — | Xoay đầu cảm biến siêu âm |
| **21** | Siêu âm | 1=có/không, 2=đọc cm | — | Đọc khoảng cách hoặc kiểm tra vật cản |
| **22** | Dò line | 0=trái, 1=giữa, 2=phải | — | Đọc trạng thái cảm biến tracking |
| **40** | Car Control (không timer) | 1-4 (hướng) | tốc độ | Chạy liên tục không giới hạn thời gian |

### 5.4 Luồng xử lý Bluetooth hoàn chỉnh

```
App nhấn "Tiến" → Gửi: {"N":2,"D1":3,"D2":200,"H":5}
       │
       ▼
HC-08 nhận BLE → chuyển UART → Arduino Serial buffer
       │
       ▼
getBTData_Plus() đọc Serial → Tích lũy ký tự đến '}'
       │
       ▼
deserializeJson() parse JSON → N=2, D1=3, D2=200
       │
       ▼
case 2: func_mode = Bluetooth, mov_mode = FORWARD, carSpeed_rocker = 200
       │
       ▼
loop() gọi bluetooth_mode() → switch(FORWARD) → forward(false, 200)
       │
       ▼
analogWrite(ENA, 200)    → Motor A quay 78% tốc độ
analogWrite(ENB, 200)    → Motor B quay 78% tốc độ
digitalWrite(IN1, HIGH)  → Motor A chiều tiến
digitalWrite(IN2, LOW)
digitalWrite(IN3, LOW)   → Motor B chiều tiến
digitalWrite(IN4, HIGH)
       │
       ▼
Xe chạy tiến với tốc độ 200/255 ≈ 78%
```

---

## 6. Cơ Chế Điều Khiển Motor (H-Bridge L298N)

### 6.1 Nguyên lý cầu H

Cầu H gồm 4 transistor tạo thành hình chữ H, cho phép đảo chiều dòng điện qua motor:

```
       +V                    +V
        │                     │
    ┌───┤ Q1            Q3 ├───┐
    │   └───┐        ┌───┘   │
    │       ├─MOTOR──┤       │       IN1=HIGH, IN2=LOW  → Dòng chạy Q1→Motor→Q4 → TIẾN
    │   ┌───┘        └───┐   │       IN1=LOW,  IN2=HIGH → Dòng chạy Q3→Motor→Q2 → LÙI
    └───┤ Q2            Q4 ├───┘       IN1=IN2 (cùng mức) → Không có dòng → PHANH
        │                     │
       GND                   GND
```

### 6.2 Bảng logic điều khiển

| Hàm | IN1 | IN2 | IN3 | IN4 | ENA | ENB | Kết quả |
|---|---|---|---|---|---|---|---|
| `forward()` | HIGH | LOW | LOW | HIGH | PWM | PWM | Cả 4 bánh tiến |
| `back()` | LOW | HIGH | HIGH | LOW | PWM | PWM | Cả 4 bánh lùi |
| `left()` | LOW | HIGH | LOW | HIGH | PWM | PWM | Trái lùi + Phải tiến = xoay trái |
| `right()` | HIGH | LOW | HIGH | LOW | PWM | PWM | Trái tiến + Phải lùi = xoay phải |
| `stop()` | — | — | — | — | LOW | LOW | Tắt enable = motor trôi tự do |

### 6.3 Các giá trị tốc độ PWM trong code

| Ngữ cảnh | Giá trị | Duty Cycle |
|---|---|---|
| `carSpeed` (IR remote) | 150 | 59% |
| `carSpeed_rocker` (Bluetooth) | 250 (mặc định) | 98% |
| Line tracking | 180 | 71% |
| Obstacle avoidance (tiến) | 150 | 59% |
| Obstacle avoidance (rẽ) | 250 | 98% |
| Hàm rẽ chéo (bánh chậm) | speed/2 | ~50% của giá trị gốc |

---

## 7. Cơ Chế Đo Khoảng Cách (Ultrasonic HC-SR04)

### 7.1 Nguyên lý hoạt động

```
Arduino                HC-SR04                    Vật cản
───────                ────────                    ───────
  TRIG ──HIGH 10μs──→  Phát 8 xung 40KHz  ─────→  │
                                                    │ Phản xạ
  ECHO ←──HIGH pulse──  Nhận sóng phản hồi ←─────  │
  │
  Đo thời gian HIGH của ECHO (μs)
  Khoảng cách = thời_gian / 58 (cm)
```

### 7.2 Code xử lý

```cpp
unsigned int getDistance(void) {
    digitalWrite(TRIG_PIN, LOW);       // Reset chân TRIG
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);      // Phát xung kích 10μs
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    tempda_x = pulseIn(ECHO_PIN, HIGH) / 58;  // Đọc echo, chia 58 = cm
    if (tempda_x > 150) tempda_x = 150;       // Giới hạn max 150cm
    return tempda_x;
}
```

> **Công thức:** Khoảng cách (cm) = Thời gian echo (μs) × Tốc độ âm thanh (340m/s) / 2 / 10000
> Rút gọn: **Khoảng cách = μs / 58**

---

## 8. Chế Độ Tự Hành

### 8.1 Dò Line (Line Tracking)

Sử dụng 3 cảm biến IR phản xạ gắn dưới gầm xe:

```
         ┌─────────────────────┐
         │    Phía trước xe    │
         │                     │
    [Left]   [Middle]   [Right]     ← 3 cảm biến IR
─ ─ ─ ─ ═══════════════ ─ ─ ─ ─    ← Vạch đen trên nền trắng
```

| Trái | Giữa | Phải | Hành động |
|---|---|---|---|
| 0 | **1** | 0 | `forward()` — đi thẳng |
| 0 | 0 | **1** | `right()` — rẽ phải cho đến khi hết tín hiệu |
| **1** | 0 | 0 | `left()` — rẽ trái cho đến khi hết tín hiệu |
| 0 | 0 | 0 | `stop()` sau 150ms — mất line thì dừng |

### 8.2 Tránh Vật Cản (Obstacle Avoidance)

Servo xoay đầu siêu âm quét 3 hướng (30°, 90°, 150°) để quyết định:

```
Bước 1: Đo khoảng cách phía trước (servo 90°)
   │
   ├─ > 20cm → forward(150) — Đường thoáng, đi tiến
   │
   └─ ≤ 20cm → stop() → Quét 3 hướng:
         │
         ├─ Servo 30° (phải)  → Đo khoảng cách
         ├─ Servo 90° (giữa) → Đo khoảng cách
         ├─ Servo 150° (trái) → Đo khoảng cách
         │
         └─ Tính tổng switc_ctrl → Quyết định rẽ hướng nào thoáng nhất
              ├─ Chỉ phải bị chắn → forward
              ├─ Giữa bị chắn    → left
              ├─ Trái bị chắn    → right
              └─ Tất cả bị chắn  → back rồi right
```

---

## 9. Vòng Lặp Chính (`loop()`)

```
loop() chạy liên tục ~1000 lần/giây
  │
  ├─ getBTData_Plus()           → Đọc & parse lệnh Bluetooth (JSON)
  ├─ getIRData()                → Đọc tín hiệu IR remote (NEC)
  ├─ bluetooth_mode()           → Xử lý lệnh Bluetooth (nếu đang ở mode BT)
  ├─ irremote_mode()            → Xử lý lệnh IR (nếu đang ở mode IR)
  ├─ line_teacking_mode()       → Chạy dò line (nếu đang ở mode LT)
  ├─ obstacles_avoidance_mode() → Chạy tránh vật cản (nếu đang ở mode OA)
  ├─ getDistance()               → Đo khoảng cách liên tục
  ├─ CMD_MotorControl_Plus()    → Điều khiển motor đơn lẻ (CMD mode)
  ├─ CMD_CarControl_Plus()      → Điều khiển xe có timer (CMD mode)
  ├─ CMD_CarControl_Plusxxx()   → Điều khiển xe không timer (CMD mode)
  └─ CMD_ClearAllFunctionsXXX() → Reset nếu có lệnh clear
```

> **Cơ chế chuyển chế độ:** Biến `func_mode` (enum) quyết định chế độ hiện tại.
> Mỗi hàm mode kiểm tra `if (func_mode == ...)` trước khi thực thi.
> Tại một thời điểm chỉ có **1 chế độ duy nhất** được kích hoạt.

---

## 10. Sơ Đồ Kết Nối Tổng Thể

```
                        ┌─────────────────────────┐
                        │     ARDUINO UNO R3      │
                        │                         │
   ┌─ HC-08 BT ───────→│ Pin 0 (RX)   Pin 8 (IN1)│──→ ┌──────────┐
   │                    │ Pin 1 (TX)   Pin 7 (IN2)│──→ │  L298N   │──→ Motor A (2 bánh trái)
   │  IR Remote ──→ IR ─│ Pin 12       Pin 11(IN3)│──→ │  H-Bridge│──→ Motor B (2 bánh phải)
   │  Receiver          │              Pin 9 (IN4)│──→ │          │
   │                    │ Pin 5 (ENA)  Pin 6 (ENB)│──→ └──────────┘
   │                    │                         │
   │  Servo SG90 ←──────│ Pin 3 (PWM)             │
   │                    │                         │
   │  HC-SR04 ──────────│ A4 (ECHO)  A5 (TRIG)   │
   │                    │                         │
   │  IR Track L ───────│ Pin 2                   │
   │  IR Track M ───────│ Pin 4                   │
   │  IR Track R ───────│ Pin 10                  │
   │                    │                         │
   │  LED ──────────────│ Pin 13                  │
   │                    └─────────────────────────┘
   │                              │
   │                         ┌────┴────┐
   └── Điện thoại            │ 2x18650│
       (App Elegoo)          │  7.4V  │
                             └────────┘
```
