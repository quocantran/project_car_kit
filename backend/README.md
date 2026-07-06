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
