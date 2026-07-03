"""
Configuration for the FastAPI BLE Bridge server.
Adjust BLE_DEVICE_ADDRESS if you know the MAC address of your ELEGOO BT16 module.
"""

# ── BLE Configuration ────────────────────────────────────────
BLE_DEVICE_NAME = "ELEGOO BT16"  # Name to scan for (case-insensitive match)
BLE_DEVICE_ADDRESS: str | None = None  # Set MAC address to skip scanning, e.g. "AA:BB:CC:DD:EE:FF"

# ELEGOO BT16 BLE UART Service & Characteristic UUIDs
# BT16 uses SEPARATE characteristics for Notify vs Write (unlike HC-08 which uses FFE1 for both)
BLE_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
BLE_CHAR_NOTIFY_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"  # Subscribe to notifications (RX from Arduino)
BLE_CHAR_WRITE_UUID = "0000ffe2-0000-1000-8000-00805f9b34fb"   # Write commands (TX to Arduino)

# ELEGOO BT16 BLE MTU / Write Chunking
# BT16 default MTU is 23 bytes → max payload per write is 20 bytes.
# Commands longer than this must be split into chunks.
BLE_MTU_SIZE = 23
BLE_WRITE_CHUNK_SIZE = 20  # Safe payload size per BLE write
BLE_CHUNK_DELAY = 0.05  # Seconds between chunked writes

# ── Telemetry Polling ────────────────────────────────────────
TELEMETRY_INTERVAL = 1.0  # Seconds between distance sensor polls

# ── Server Configuration ─────────────────────────────────────
SERVER_HOST = "0.0.0.0"
SERVER_PORT = 8000

# ── CORS ─────────────────────────────────────────────────────
ALLOWED_ORIGINS = [
    "http://localhost:3000",
    "http://127.0.0.1:3000",
    "http://localhost:3001",
]

# ── Arduino Protocol Defaults ────────────────────────────────
DEFAULT_SPEED = 200  # Default PWM speed (0-255)
MAX_SPEED = 255
MIN_SPEED = 0
SERVO_MIN_ANGLE = 5
SERVO_MAX_ANGLE = 175
BLE_RESPONSE_TIMEOUT = 2.0  # Seconds to wait for Arduino response
BLE_SCAN_TIMEOUT = 10.0  # Seconds to scan for BLE devices
