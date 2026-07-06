# 🔧 Tóm Tắt Các Fix Đã Áp Dụng

## Arduino Code — 3 Bug Đã Fix

### Bug 1: 🔴 Buffer Overflow `char buf[3]` (CRITICAL - Nguyên nhân chính)

```diff
- char buf[3];
+ char buf[8];  // Fixed: "176\0" = 4 bytes, cần ít nhất 4
```

**Tại sao đây là nguyên nhân chính?**

Python backend gửi H values: 176, 177, 178... (3 chữ số).  
`sprintf(buf, "%d", 176)` ghi `"176\0"` = **4 bytes** vào buffer chỉ **3 bytes**.  
→ **Stack memory corruption** → `func_mode`, `mov_mode` hoặc bất kỳ biến nào trên stack có thể bị ghi đè → Motor commands bị corrupted silently!

### Bug 2: 🔴 `getDistance()` gọi MỖI loop iteration

```diff
- CMD_Distance = getDistance(); // Mỗi loop = block 30ms-1000ms
+ if (millis() - lastDistanceMillis >= DISTANCE_INTERVAL) {
+   CMD_Distance = getDistance();
+   lastDistanceMillis = millis();
+ }
```

`pulseIn()` default timeout = **1 giây**. Gọi mỗi loop → Arduino xử lý serial cực chậm → buffer overflow (64 bytes).

### Bug 3: 🟡 `pulseIn` timeout quá lâu

```diff
- tempda_x = ((unsigned int)pulseIn(ECHO_PIN, HIGH) / 58);
+ tempda_x = ((unsigned int)pulseIn(ECHO_PIN, HIGH, 30000) / 58);  // 30ms timeout
```

## Backend Python — 2 Cải thiện

1. **Serial RX log**: `debug` → `info` — Để thấy Arduino response trên console
2. **Thêm `\n`** sau mỗi JSON write — Cleaner serial framing

## Tiếp Theo: Upload Firmware Mới

```bash
# Trên Raspberry Pi:
cd ~/robot_car_kit    # hoặc path tới robot_car_kit/

# Compile
arduino-cli compile --fqbn arduino:avr:uno ./SmartCar_Core_20210127/

# Upload
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno ./SmartCar_Core_20210127/

# Restart backend
cd ~/backend
python main.py
```

Sau khi upload, trong log Pi bạn sẽ thấy dòng debug echo từ Arduino:
```
📥 Serial RX: {"dbg":"N=2,H=1"}
```

Nếu thấy dòng này = Arduino **ĐÃ nhận và parse JSON thành công**.
