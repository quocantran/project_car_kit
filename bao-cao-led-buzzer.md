# Báo cáo điều khiển đèn LED và buzzer của xe robot

## 1. Mục tiêu báo cáo

Báo cáo này tổng hợp cơ chế điều khiển **đèn LED giao thông** và **buzzer/còi cảnh báo** trong dự án xe robot. Nội dung được phân tích từ ba lớp chính của hệ thống:

1. **Firmware Arduino**: nhận lệnh điều khiển qua Serial, quyết định cho phép/dừng xe theo trạng thái đèn giao thông, phát sự kiện bật/tắt buzzer khi tránh vật cản.
2. **Backend Raspberry Pi/FastAPI**: điều khiển GPIO vật lý cho LED đỏ, LED xanh, buzzer và nút nhấn; đồng bộ trạng thái với Arduino qua giao thức JSON Serial; phát telemetry qua WebSocket.
3. **Frontend Web Dashboard**: cung cấp giao diện đổi đèn đỏ/xanh và bật/tắt buzzer từ xa.

---

## 2. Kiến trúc tổng thể

```text
[Web Dashboard - Next.js]
        |
        | WebSocket / REST
        v
[Backend FastAPI trên Raspberry Pi]
        |
        | GPIO BCM: LED đỏ, LED xanh, buzzer, button
        v
[Mạch LED/Buzzer trên breadboard]

[Backend FastAPI trên Raspberry Pi]
        |
        | USB Serial 115200 baud - JSON command
        v
[Arduino Uno + SmartCar Shield]
        |
        | Điều khiển motor, servo, sensor, logic traffic_light_green
        v
[Xe robot]
```

Hệ thống tách phần **hiển thị/cảnh báo vật lý** sang Raspberry Pi GPIO, còn Arduino chịu trách nhiệm điều khiển chuyển động. Khi người dùng đổi trạng thái đèn giao thông trên web:

- Raspberry Pi đổi LED vật lý ngay lập tức.
- Backend gửi lệnh Serial `N=50` xuống Arduino.
- Arduino cập nhật biến `traffic_light_green`.
- Nếu đèn đỏ, Arduino ép xe dừng trong các chế độ di chuyển.

---

## 3. Các thành phần liên quan

### 3.1. Frontend Web Dashboard

Frontend có hai khối giao diện chính liên quan:

| Thành phần | Vai trò |
|---|---|
| `TrafficLight` | Hiển thị đèn đỏ/xanh, cho phép click đổi trạng thái giao thông |
| `QuickActions` | Có nút `Buzzer`, gửi yêu cầu bật/tắt còi |
| `useRobotWebSocket` | Đóng gói hàm gửi WebSocket `traffic_light` và `buzzer` |

Luồng điều khiển trên frontend:

```text
Người dùng click đèn đỏ/xanh
        |
        v
TrafficLight.onChange(state)
        |
        v
sendTrafficLight(state)
        |
        v
WebSocket message: {"type":"traffic_light","state":"red|green"}
```

Với buzzer:

```text
Người dùng click Quick Actions -> Buzzer
        |
        v
sendBuzzer(on)
        |
        v
WebSocket message: {"type":"buzzer","on":true|false}
```

### 3.2. Backend FastAPI trên Raspberry Pi

Backend có các file chính:

| File | Vai trò |
|---|---|
| `backend/config.py` | Khai báo chân GPIO, baudrate Serial, ngưỡng còi tự động |
| `backend/gpio_service.py` | Thiết lập GPIO, bật/tắt LED đỏ, LED xanh, buzzer, đọc button |
| `backend/serial_service.py` | Gửi lệnh Serial tới Arduino, nhận telemetry và sự kiện buzzer |
| `backend/car_protocol.py` | Sinh chuỗi JSON lệnh điều khiển, gồm `N=50` cho traffic light |
| `backend/main.py` | REST API và WebSocket endpoint |

### 3.3. Firmware Arduino

Firmware chính nằm trong thư mục `robot_car_kit/SmartCar_Core_20210127`. Các phần liên quan:

| Thành phần firmware | Vai trò |
|---|---|
| `traffic_light_green` | Biến toàn cục quyết định xe được chạy hay phải dừng |
| Lệnh Serial `N=50` | Nhận trạng thái đèn từ Raspberry Pi: `D1=1` xanh, `D1=0` đỏ |
| `CMD_Telemetry_Plus()` | Trả telemetry JSON có trường `tl` biểu diễn traffic light |
| `obstacles_avoidance_mode()` | Khi xe cần thoát vật cản, Arduino gửi event `buzzer_on`/`buzzer_off` lên backend |

---

## 4. Mapping chân GPIO và chân Arduino

### 4.1. Raspberry Pi GPIO cho LED, buzzer và nút nhấn

Backend dùng chuẩn đánh số **BCM**.

| Thiết bị | GPIO BCM | Chân vật lý Raspberry Pi | Trạng thái mặc định | Vai trò |
|---|---:|---:|---|---|
| LED đỏ | GPIO17 | Pin 11 | LOW | Báo trạng thái STOP/đèn đỏ |
| LED xanh | GPIO27 | Pin 13 | HIGH | Báo trạng thái GO/đèn xanh |
| Buzzer | GPIO22 | Pin 15 | LOW | Cảnh báo âm thanh |
| Button | GPIO23 | Pin 16 | INPUT pull-up | Nút nhấn đảo trạng thái đèn |

Ý nghĩa logic:

| Trạng thái | LED xanh | LED đỏ | Buzzer | Xe |
|---|---|---|---|---|
| `green` | ON | OFF | Theo điều khiển/cảnh báo | Được phép chạy |
| `red` | OFF | ON | Theo điều khiển/cảnh báo | Arduino ép dừng |

### 4.2. Chân Arduino liên quan tới xe

| Thiết bị | Chân Arduino | Vai trò |
|---|---|---|
| LED onboard | D13 | Được định nghĩa là `LED_Pin`, chủ yếu là LED trạng thái/onboard |
| Ultrasonic ECHO | A4 | Đo phản hồi cảm biến siêu âm |
| Ultrasonic TRIG | A5 | Phát xung trigger siêu âm |
| Servo | D3 | Quay cảm biến siêu âm |
| Motor ENA | D5 PWM | Tốc độ motor A |
| Motor ENB | D6 PWM | Tốc độ motor B |
| Motor IN1/IN2 | D8/D7 | Hướng motor A |
| Motor IN3/IN4 | D11/D9 | Hướng motor B |
| IR receiver | D12 | Nhận remote hồng ngoại |
| Line tracking L/M/R | D2/D4/D10 | Cảm biến dò line |

Lưu ý: Trong firmware Arduino có `#define LED_Pin 13`, nhưng phần đèn giao thông đỏ/xanh trong dự án hiện được điều khiển bằng Raspberry Pi GPIO, không phải bằng Arduino D13.

---

## 5. Nguyên lý điều khiển đèn giao thông

### 5.1. Trạng thái trên Backend

Backend duy trì trạng thái đèn trong `SerialService` và `GpioService`:

```text
serial_service.traffic_light = "green" | "red"
gpio_service.traffic_light   = "green" | "red"
```

Khi nhận lệnh đổi đèn:

1. Backend gọi `gpio_service.set_traffic_light(state)` để đổi LED vật lý.
2. Nếu Arduino đang kết nối, backend gọi `serial_service.set_traffic_light(state)`.
3. `serial_service` tạo lệnh JSON `N=50` gửi xuống Arduino.
4. Backend broadcast telemetry/WebSocket để frontend cập nhật lại trạng thái.

### 5.2. Lệnh Serial gửi xuống Arduino

Lệnh traffic light có dạng:

```json
{"N":50,"D1":1,"H":12}
```

hoặc:

```json
{"N":50,"D1":0,"H":13}
```

Ý nghĩa:

| Trường | Ý nghĩa |
|---|---|
| `N=50` | Mã lệnh điều khiển đèn giao thông |
| `D1=1` | Đèn xanh, cho phép xe chạy |
| `D1=0` | Đèn đỏ, ép xe dừng |
| `H` | Số thứ tự command để phản hồi `{H_ok}` |

### 5.3. Xử lý trong Arduino

Arduino nhận `N=50` trong hàm đọc Serial. Khi `D1=1`, biến `traffic_light_green = true`; khi `D1=0`, biến `traffic_light_green = false`. Nếu chuyển sang đỏ, Arduino gọi `stop()` và đặt `mov_mode = STOP`.

Logic này tác động trực tiếp tới các chế độ:

| Chế độ | Hành vi khi đèn đỏ |
|---|---|
| Bluetooth/manual | Gọi `stop()` và bỏ qua lệnh chạy |
| IR remote | Gọi `stop()`, sau 500 ms đặt `mov_mode = STOP` |
| Line tracking | Gọi `stop()` và không dò line tiếp |
| Obstacle avoidance | Gọi `stop()` và không thực hiện tránh vật cản |
| Hand tracking | Gọi `stop()`, đặt `mov_mode = STOP`, reset tốc độ tracking |

Nhờ đó, đèn đỏ không chỉ là hiển thị mà là một **interlock an toàn**: xe bị vô hiệu hóa chuyển động ở tầng firmware.

### 5.4. Telemetry phản hồi lên Backend/Frontend

Arduino có lệnh telemetry `N=100`, phản hồi dạng:

```json
{"d":35,"s":200,"dir":3,"m":3,"tl":1}
```

Trong đó:

| Trường | Ý nghĩa |
|---|---|
| `d` | Khoảng cách siêu âm hiện tại |
| `s` | Tốc độ PWM hiện tại |
| `dir` | Mã hướng chuyển động |
| `m` | Mã chế độ hoạt động |
| `tl` | Traffic light: `1` xanh, `0` đỏ |

Backend parse trường `tl` thành `traffic_light = "green"` hoặc `"red"`, rồi gửi lên frontend qua WebSocket telemetry.

---

## 6. Nguyên lý điều khiển buzzer

Buzzer trong dự án được điều khiển ở Raspberry Pi qua GPIO22. Có ba nguồn kích hoạt chính:

1. Người dùng bật/tắt từ Web Dashboard.
2. Backend tự bật buzzer khi telemetry cho thấy xe ở chế độ tránh vật cản, khoảng cách nhỏ hơn ngưỡng.
3. Arduino gửi sự kiện `buzzer_on`/`buzzer_off` khi thuật toán tránh vật cản cần cảnh báo.

### 6.1. Điều khiển thủ công từ Web Dashboard

Luồng:

```text
Người dùng click Buzzer
        |
        v
Frontend gửi WebSocket: {"type":"buzzer","on":true}
        |
        v
Backend gọi gpio_service.set_buzzer(true)
        |
        v
GPIO22 = HIGH -> buzzer kêu
        |
        v
Backend broadcast telemetry: buzzer=true
```

Khi tắt:

```text
{"type":"buzzer","on":false}
        |
        v
GPIO22 = LOW -> buzzer tắt
```

### 6.2. Buzzer tự động từ Backend

Trong backend có hằng số:

```text
AUTO_BUZZER_DISTANCE_THRESHOLD = 25 cm
```

Trong telemetry loop, backend kiểm tra:

- Xe đang ở chế độ `obstacle_avoidance`.
- Khoảng cách `0 < distance < 25 cm`.
- Xe đang lùi hoặc đang rẽ/tránh vật cản.

Nếu thỏa điều kiện, backend gọi `_start_auto_buzzer(2.0)`, bật buzzer trong 2 giây bằng GPIO22. Nếu điều kiện không còn đúng, backend tắt buzzer tự động.

### 6.3. Buzzer event từ Arduino

Trong `obstacles_avoidance_mode()`, khi Arduino xác định tình huống cần thoát vật cản bằng cách rẽ hoặc lùi, firmware gửi:

```json
{"event":"buzzer_on"}
```

Sau khi xử lý xong pha cảnh báo/thoát vật cản, Arduino gửi:

```json
{"event":"buzzer_off"}
```

Backend đọc các JSON event này trong serial reader:

- Nếu event là `buzzer_on`: đặt `serial_service.buzzer = true`, gọi `gpio_service.set_buzzer(True)`.
- Nếu event là `buzzer_off`: đặt `serial_service.buzzer = false`, gọi `gpio_service.set_buzzer(False)`.
- Sau đó broadcast telemetry để frontend cập nhật trạng thái buzzer.

Điểm đáng chú ý: Arduino không trực tiếp nối buzzer trong firmware hiện tại; Arduino chỉ gửi **sự kiện logic**, còn Raspberry Pi mới là phần bật/tắt phần cứng buzzer.

---

## 7. REST API và WebSocket liên quan

### 7.1. REST API

| Endpoint | Phương thức | Chức năng |
|---|---|---|
| `/api/traffic-light` | GET | Lấy trạng thái đèn hiện tại |
| `/api/traffic-light` | POST | Set đèn `green` hoặc `red` |
| `/api/traffic-light/toggle` | POST | Đảo trạng thái xanh/đỏ |
| `/api/buzzer` | POST | Bật/tắt buzzer vật lý |

Ví dụ POST traffic light:

```json
{"state":"red"}
```

Ví dụ bật buzzer:

```text
POST /api/buzzer?on=true
```

### 7.2. WebSocket message

Client gửi:

```json
{"type":"traffic_light","state":"red"}
```

hoặc:

```json
{"type":"buzzer","on":true}
```

Server trả/broadcast:

```json
{"type":"traffic_light_update","data":{"state":"red"}}
```

Telemetry:

```json
{
  "type":"telemetry",
  "data":{
    "battery":100,
    "speed":0,
    "speed_percent":0,
    "distance":35,
    "is_connected":true,
    "current_direction":"stop",
    "current_mode":"idle",
    "traffic_light":"red",
    "buzzer":false
  }
}
```

---

## 8. Đề xuất nối mạch LED và buzzer

### 8.1. Nguyên tắc chung

Raspberry Pi GPIO chỉ xuất mức logic 3.3V và dòng nhỏ. Không nên nối tải lớn trực tiếp. Với LED đơn có điện trở hạn dòng thì có thể nối trực tiếp. Với buzzer, tùy loại:

- **Active buzzer loại nhỏ 3.3V/5V, dòng thấp**: có thể thử điều khiển trực tiếp qua GPIO nếu dòng rất nhỏ, nhưng vẫn khuyến nghị dùng transistor.
- **Buzzer 5V hoặc dòng lớn**: nên dùng transistor NPN hoặc MOSFET để bảo vệ GPIO.

### 8.2. Nối LED đỏ và LED xanh

Cách nối active-high phổ biến:

```text
GPIO17 ----[330Ω]----|>|---- GND
                     LED đỏ

GPIO27 ----[330Ω]----|>|---- GND
                     LED xanh
```

Trong đó:

- Chân dài LED/anode nối về phía GPIO qua điện trở.
- Chân ngắn LED/cathode nối GND.
- GPIO HIGH: LED sáng.
- GPIO LOW: LED tắt.

Giá trị điện trở đề xuất: 220Ω đến 1kΩ, thông dụng là 330Ω.

### 8.3. Nối buzzer trực tiếp đơn giản

Chỉ dùng nếu buzzer dòng thấp:

```text
GPIO22 ---- (+) Buzzer (-) ---- GND
```

Khi `GPIO22 = HIGH`, buzzer kêu. Khi `GPIO22 = LOW`, buzzer tắt.

Nhược điểm: có thể quá dòng GPIO nếu buzzer tiêu thụ lớn.

### 8.4. Nối buzzer qua transistor NPN khuyến nghị

Mạch an toàn hơn:

```text
Raspberry Pi GPIO22 ----[1kΩ]---- Base 2N2222/S8050
                                  |
5V ---- (+) Buzzer (-) ---- Collector
Emitter -------------------------- GND

GND Raspberry Pi nối chung GND nguồn 5V
```

Nếu buzzer là loại điện từ/cuộn cảm, nên mắc diode dập xung ngược song song với buzzer:

```text
Diode 1N4148/1N4007 song song buzzer
Cathode diode -> +5V
Anode diode   -> Collector/transistor side
```

Ý nghĩa:

- GPIO22 HIGH làm transistor dẫn, dòng chạy từ 5V qua buzzer xuống GND, buzzer kêu.
- GPIO22 LOW làm transistor ngắt, buzzer tắt.
- GPIO chỉ cấp dòng base nhỏ qua điện trở 1kΩ nên an toàn hơn.

### 8.5. Nối button đảo đèn

Backend cấu hình `GPIO_BUTTON_PIN = 23` ở chế độ input pull-up. Vì vậy nút nên nối như sau:

```text
GPIO23 ---- Button ---- GND
```

Hoạt động:

- Không nhấn: GPIO23 được kéo lên HIGH bởi pull-up nội.
- Nhấn: GPIO23 bị nối GND, đọc LOW.
- Backend bắt cạnh xuống `FALLING` để đảo đèn xanh/đỏ.

### 8.6. Lưu ý nối đất chung

Nếu buzzer dùng nguồn 5V riêng hoặc module nguồn ngoài, bắt buộc nối chung:

```text
GND Raspberry Pi ---- GND nguồn ngoài ---- GND mạch buzzer
```

Nếu không chung mass, tín hiệu GPIO không có điểm tham chiếu điện áp và transistor/module có thể hoạt động sai.

---

## 9. Bảng luồng hoạt động chi tiết

### 9.1. Đổi sang đèn đỏ

| Bước | Thành phần | Hành động |
|---:|---|---|
| 1 | Người dùng | Click `Yêu Cầu Dừng (Đèn Đỏ)` trên web |
| 2 | Frontend | Gửi WebSocket `{"type":"traffic_light","state":"red"}` |
| 3 | Backend | Gọi `gpio_service.set_traffic_light("red")` |
| 4 | GPIO | LED đỏ ON, LED xanh OFF |
| 5 | Backend | Gửi Serial `{"N":50,"D1":0,"H":...}` tới Arduino |
| 6 | Arduino | `traffic_light_green = false`, gọi `stop()` |
| 7 | Arduino loop | Các chế độ chạy đều kiểm tra đèn đỏ và tiếp tục ép dừng |
| 8 | Backend/Frontend | Telemetry cập nhật `traffic_light = "red"` |

### 9.2. Đổi sang đèn xanh

| Bước | Thành phần | Hành động |
|---:|---|---|
| 1 | Người dùng | Click `Cho Phép Chạy (Đèn Xanh)` |
| 2 | Frontend | Gửi WebSocket `{"type":"traffic_light","state":"green"}` |
| 3 | Backend | LED xanh ON, LED đỏ OFF |
| 4 | Backend | Gửi Serial `{"N":50,"D1":1,"H":...}` |
| 5 | Arduino | `traffic_light_green = true` |
| 6 | Xe | Các chế độ được phép thực thi chuyển động trở lại |

### 9.3. Cảnh báo buzzer khi tránh vật cản

| Bước | Thành phần | Hành động |
|---:|---|---|
| 1 | Arduino | Cảm biến siêu âm phát hiện vật cản trong obstacle avoidance |
| 2 | Arduino | Chọn hành động rẽ/lùi để thoát vật cản |
| 3 | Arduino | Gửi `{"event":"buzzer_on"}` qua Serial |
| 4 | Backend | Parse event, bật GPIO22 |
| 5 | Buzzer | Kêu cảnh báo |
| 6 | Arduino | Sau pha xử lý, gửi `{"event":"buzzer_off"}` |
| 7 | Backend | Tắt GPIO22 và cập nhật telemetry |

---

## 10. Đánh giá thiết kế

### 10.1. Ưu điểm

- **Tách trách nhiệm rõ ràng**: Raspberry Pi xử lý web/GPIO, Arduino xử lý thời gian thực của xe.
- **An toàn chuyển động**: Đèn đỏ được kiểm tra ở firmware, không chỉ dừng ở giao diện web.
- **Cập nhật thời gian thực**: WebSocket giúp frontend nhận telemetry và trạng thái buzzer/đèn nhanh.
- **Có fallback mô phỏng GPIO**: Khi không chạy trên Raspberry Pi, backend vẫn hoạt động ở simulation mode.
- **Có nhiều cơ chế điều khiển buzzer**: thủ công, tự động theo telemetry, event từ Arduino.

### 10.2. Hạn chế hiện tại

- LED onboard Arduino D13 được định nghĩa nhưng chưa thấy logic điều khiển rõ ràng trong firmware hiện tại.
- Buzzer không nối trực tiếp Arduino; nếu Raspberry Pi/backend không chạy thì event `buzzer_on` từ Arduino không tạo âm thanh.
- Quick Actions có các nút `Headlight`, `LED`, `E-Stop` nhưng hiện mới thấy `Buzzer` được nối với backend thực sự; các nút còn lại chủ yếu đổi trạng thái UI cục bộ.
- REST endpoint `/api/buzzer` nhận `on` dạng query parameter trong định nghĩa hiện tại, cần lưu ý khi gọi API.

### 10.3. Đề xuất cải tiến

1. **Bổ sung điều khiển LED onboard Arduino D13** bằng command riêng, ví dụ `N=51`, nếu muốn LED trên xe phản ánh trạng thái từ web.
2. **Thêm buzzer backup trên Arduino** nếu muốn xe vẫn cảnh báo khi mất kết nối Raspberry Pi.
3. **Chuẩn hóa Quick Actions**: gắn `Headlight`, `LED`, `E-Stop` với API/WebSocket thật hoặc ẩn nếu chưa dùng.
4. **Dùng transistor cho buzzer** để bảo vệ GPIO Raspberry Pi.
5. **Thêm diode dập xung** nếu dùng buzzer từ tính/cuộn cảm.
6. **Ghi rõ sơ đồ dây trong README chính** để người nối mạch không nhầm BCM GPIO với số chân vật lý.

---

## 11. Kết luận

Cơ chế điều khiển LED và buzzer của dự án được xây dựng theo mô hình nhiều tầng. Frontend gửi yêu cầu qua WebSocket/REST, backend xử lý GPIO và đồng bộ với Arduino, firmware Arduino áp dụng trạng thái đèn giao thông vào logic dừng/chạy của xe. Đèn đỏ/xanh đóng vai trò vừa là tín hiệu hiển thị vừa là khóa an toàn chuyển động. Buzzer đóng vai trò cảnh báo âm thanh, có thể bật thủ công hoặc tự động khi xe tránh vật cản.

Về nối mạch, LED đỏ/xanh có thể nối trực tiếp với GPIO Raspberry Pi qua điện trở hạn dòng 220Ω-1kΩ. Buzzer nên nối qua transistor NPN/MOSFET nếu dùng loại 5V hoặc dòng lớn. Button đảo đèn nối giữa GPIO23 và GND do backend đã cấu hình pull-up nội. Khi dùng nguồn ngoài cho buzzer, cần nối chung GND với Raspberry Pi để mạch hoạt động ổn định.
