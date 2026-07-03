# 🚗 Robot Car BLE Bridge — FastAPI Backend

Backend server đóng vai trò **cầu nối** giữa giao diện web Next.js và xe robot Elegoo Smart Car V3.0 Plus qua Bluetooth Low Energy (BLE).

## Kiến trúc

```
Next.js Frontend ◄──HTTP/WS──► FastAPI Server ◄──BLE──► HC-08 ◄──UART──► Arduino UNO
   :3000                          :8000                                    Robot Car
```

## Yêu cầu

- **Python 3.10+**
- **Bluetooth adapter** trên PC/laptop (hỗ trợ BLE 4.0+)
- Xe robot **bật nguồn** (module HC-08 phải hoạt động)

## Cài đặt

```bash
# 1. Di chuyển vào thư mục backend
cd backend

# 2. Tạo virtual environment (khuyến nghị)
python -m venv venv
venv\Scripts\activate       # Windows
# source venv/bin/activate  # Linux/macOS

# 3. Cài đặt dependencies
pip install -r requirements.txt
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
Mở browser: **http://localhost:8000/docs**

### REST API

| Method | Path | Mô tả |
|--------|------|--------|
| `GET` | `/api/status` | Trạng thái BLE + telemetry |
| `POST` | `/api/connect` | Kết nối BLE tới HC-08 |
| `POST` | `/api/disconnect` | Ngắt kết nối BLE |
| `GET` | `/api/scan` | Scan BLE devices |
| `POST` | `/api/control` | Điều khiển hướng xe |
| `POST` | `/api/speed` | Cập nhật tốc độ |
| `POST` | `/api/stop` | Dừng khẩn cấp |
| `POST` | `/api/servo` | Điều khiển servo |
| `POST` | `/api/mode` | Chuyển chế độ tự hành |
| `POST` | `/api/reset` | Reset tất cả |
| `GET` | `/api/distance` | Đọc khoảng cách siêu âm |

### WebSocket
- **URL:** `ws://localhost:8000/ws`
- **Gửi lệnh:** `{"type": "control", "direction": "forward", "speed": 200}`
- **Nhận telemetry:** `{"type": "telemetry", "data": {...}}`

## Cấu hình

Sửa file `config.py`:
- `BLE_DEVICE_ADDRESS`: MAC address HC-08 (để `None` sẽ auto-scan)
- `BLE_DEVICE_NAME`: Tên thiết bị để scan (mặc định: `"HC-08"`)
- `TELEMETRY_INTERVAL`: Tần suất polling sensor (giây)

## Lưu ý

- **Battery**: Giá trị pin là **ước tính** (không có cảm biến phần cứng)
- **Speed**: Hiển thị **PWM duty cycle** (0-255), không phải tốc độ vật lý
- **Windows**: Đảm bảo Bluetooth được bật trong Settings → Bluetooth & devices
