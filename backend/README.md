# 🚗 Robot Car Serial Bridge — FastAPI Backend (Raspberry Pi 4)

Backend server đóng vai trò **cầu nối** giữa giao diện web Next.js và xe robot Elegoo Smart Car V3.0 Plus qua **USB Serial** trên Raspberry Pi 4.

## Kiến trúc

```
Next.js Frontend ◄──WiFi (HTTP/WS)──► FastAPI Server (Pi 4) ◄──USB Serial──► Arduino UNO
   Phone/Laptop                          :8000                                 Robot Car
```

## Yêu cầu

- **Raspberry Pi 4** với WiFi đã setup
- **Python 3.10+** trên Pi
- **Arduino** nối USB vào Pi (Uno/Mega → `/dev/ttyACM0`, Nano/Clone → `/dev/ttyUSB0`)
- Xe robot **bật nguồn**

## Sơ đồ dây GPIO cho LED, buzzer và button

> **Quan trọng:** Code backend dùng chuẩn đánh số **BCM GPIO**, không phải số chân vật lý trên header 40 pin. Ví dụ `GPIO17` là **chân vật lý 11**, không phải chân vật lý 17.

### Bảng mapping theo code hiện tại

Các chân được khai báo trong `config.py`:

| Thiết bị | Tên trong code | GPIO BCM | Chân vật lý Raspberry Pi | Màu dây gợi ý/từ mạch | Chức năng |
|---|---|---:|---:|---|---|
| LED đỏ | `GPIO_LED_RED_PIN` | `GPIO17` | Pin 11 | Dây tín hiệu LED đỏ | Đèn đỏ: yêu cầu xe dừng |
| LED xanh | `GPIO_LED_GREEN_PIN` | `GPIO27` | Pin 13 | Dây tín hiệu LED xanh | Đèn xanh: cho phép xe chạy |
| Buzzer | `GPIO_BUZZER_PIN` | `GPIO22` | Pin 15 | Dây tín hiệu buzzer | Còi cảnh báo |
| Button | `GPIO_BUTTON_PIN` | `GPIO23` | Pin 16 | Dây nút nhấn | Nhấn để đảo đỏ/xanh |
| GND chung | - | `GND` | Pin 6/9/14/20/25/30/34/39 | Dây đen/nâu | Mass chung cho LED, buzzer, button |
| Nguồn module | - | `3V3` hoặc `5V` | Pin 1/17 hoặc Pin 2/4 | Dây đỏ nếu có | Cấp nguồn cho module nếu cần |

### Sơ đồ chân Raspberry Pi 40 pin cần dùng

Nhìn vào Raspberry Pi theo hướng USB/Ethernet ở bên phải, header 40 pin có cách đánh số vật lý như sau:

```text
 Pin vật lý lẻ bên trái        Pin vật lý chẵn bên phải
┌───────────────────────┬────────────────────────┐
│ 1  3V3                │ 2  5V                  │
│ 3  GPIO2              │ 4  5V                  │
│ 5  GPIO3              │ 6  GND                 │
│ 7  GPIO4              │ 8  GPIO14              │
│ 9  GND                │ 10 GPIO15              │
│ 11 GPIO17  -> LED đỏ  │ 12 GPIO18              │
│ 13 GPIO27  -> LED xanh│ 14 GND                 │
│ 15 GPIO22  -> Buzzer  │ 16 GPIO23 -> Button    │
│ 17 3V3                │ 18 GPIO24              │
│ 19 GPIO10             │ 20 GND                 │
└───────────────────────┴────────────────────────┘
```

### Nối LED đỏ và LED xanh

Mỗi LED cần có điện trở hạn dòng, khuyến nghị `220Ω` đến `1kΩ`, thường dùng `330Ω`.

```text
GPIO17 / Pin vật lý 11 ----[330Ω]---- Anode LED đỏ (+)
Cathode LED đỏ (-) ------------------ GND

GPIO27 / Pin vật lý 13 ----[330Ω]---- Anode LED xanh (+)
Cathode LED xanh (-) ---------------- GND
```

Logic hoạt động trong code:

| Trạng thái | GPIO17 / LED đỏ | GPIO27 / LED xanh | Ý nghĩa |
|---|---|---|---|
| `red` | HIGH / sáng | LOW / tắt | Xe bị ép dừng |
| `green` | LOW / tắt | HIGH / sáng | Xe được phép chạy |

### Nối buzzer

Nếu buzzer là loại module nhỏ có chân `+` và `-`, có thể nối theo kiểu đơn giản:

```text
GPIO22 / Pin vật lý 15 ---- (+) Buzzer
GND ----------------------- (-) Buzzer
```

Nếu buzzer dùng dòng lớn hoặc buzzer 5V rời, nên nối qua transistor để bảo vệ GPIO:

```text
GPIO22 / Pin vật lý 15 ----[1kΩ]---- Base transistor NPN
5V ------------------------ (+) Buzzer
(-) Buzzer ---------------- Collector transistor
Emitter transistor -------- GND
GND Raspberry Pi ---------- GND nguồn 5V ngoài nếu có
```

### Nối button đảo đèn

Code đã cấu hình button ở chế độ pull-up nội, vì vậy nút chỉ cần nối giữa GPIO và GND:

```text
GPIO23 / Pin vật lý 16 ---- Button ---- GND
```

Hoạt động:
- Không nhấn: GPIO23 ở mức HIGH.
- Nhấn: GPIO23 bị kéo xuống LOW.
- Backend bắt cạnh xuống để đảo trạng thái đèn `green` ↔ `red`.

### Ghi chú theo ảnh mạch hiện tại

Ảnh mạch đang dùng breadboard với LED đỏ, LED xanh, buzzer và dây về board mở rộng GPIO. Khi kiểm tra lại dây, hãy đối chiếu theo **GPIO BCM trong code** và **chân vật lý trên header**:

- Dây điều khiển LED đỏ phải về `GPIO17`, tức **pin vật lý 11**.
- Dây điều khiển LED xanh phải về `GPIO27`, tức **pin vật lý 13**.
- Dây điều khiển buzzer phải về `GPIO22`, tức **pin vật lý 15**.
- Dây button phải về `GPIO23`, tức **pin vật lý 16**.
- Tất cả chân âm LED, âm buzzer và một đầu button phải có **GND chung** với Raspberry Pi.

Nếu dùng board breakout có in chữ `GPIO17`, `GPIO27`, `GPIO22`, `GPIO23` thì cắm theo chữ GPIO trên breakout. Nếu cắm trực tiếp vào header Raspberry Pi thì cắm theo **số chân vật lý** trong bảng trên.

## Cài đặt trên Raspberry Pi

```bash
# 1. Di chuyển vào thư mục backend
cd backend

# 2. Tạo virtual environment
python -m venv venv
source venv/bin/activate

# 3. Cài đặt dependencies
pip install -r requirements.txt
```

## Upload Firmware Arduino (từ Pi)

```bash
# Cài arduino-cli
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/

# Cài core & libraries
arduino-cli core install arduino:avr
arduino-cli lib install "ArduinoJson" "IRremote" "Servo"

# Kiểm tra board
arduino-cli board list

# Compile & Upload
arduino-cli compile --fqbn arduino:avr:uno ./robot_car_kit/SmartCar_Core_20210127.ino
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno ./robot_car_kit/SmartCar_Core_20210127.ino
```

## Chạy Server

```bash
# Development (auto-reload)
uvicorn main:app --reload --host 0.0.0.0 --port 8000

# Hoặc chạy trực tiếp
python main.py
```

## API Endpoints

### Swagger UI
Mở browser: **http://<pi-ip>:8000/docs**

### REST API

| Method | Path | Mô tả |
|--------|------|--------|
| `GET` | `/api/status` | Trạng thái kết nối + telemetry |
| `POST` | `/api/connect` | Kết nối Arduino qua USB Serial |
| `POST` | `/api/disconnect` | Ngắt kết nối Serial |
| `GET` | `/api/ports` | Liệt kê serial ports trên Pi |
| `POST` | `/api/control` | Điều khiển hướng xe |
| `POST` | `/api/speed` | Cập nhật tốc độ |
| `POST` | `/api/stop` | Dừng khẩn cấp |
| `POST` | `/api/servo` | Điều khiển servo |
| `POST` | `/api/mode` | Chuyển chế độ tự hành |
| `POST` | `/api/reset` | Reset tất cả |
| `GET` | `/api/distance` | Đọc khoảng cách siêu âm |

### WebSocket
- **URL:** `ws://<pi-ip>:8000/ws`
- **Gửi lệnh:** `{"type": "control", "direction": "forward", "speed": 200}`
- **Nhận telemetry:** `{"type": "telemetry", "data": {...}}`

## Cấu hình

Sửa file `config.py`:
- `SERIAL_PORT`: Port Arduino (để `None` sẽ auto-detect)
- `SERIAL_BAUDRATE`: Baud rate (mặc định: `115200`)
- `TELEMETRY_INTERVAL`: Tần suất polling sensor (giây)

## Kiểm tra kết nối Arduino

```bash
# Xem port Arduino
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null

# Xem USB log
dmesg | tail -20

# Test serial trực tiếp
python -c "import serial; s = serial.Serial('/dev/ttyACM0', 115200, timeout=1); print('OK:', s.name)"
```

## Lưu ý

- **Battery**: Giá trị pin là **ước tính** (không có cảm biến phần cứng)
- **Speed**: Hiển thị **PWM duty cycle** (0-255), không phải tốc độ vật lý
- **Arduino Reset**: Khi mở serial connection, Arduino sẽ **tự reset** — chờ ~2 giây
- **Permission**: Nếu gặp lỗi permission, thêm user vào group dialout:
  ```bash
  sudo usermod -a -G dialout $USER
  # Sau đó logout/login lại
  ```
