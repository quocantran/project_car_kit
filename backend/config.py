"""
Configuration for the FastAPI BLE Bridge server.
Adjust BLE_DEVICE_ADDRESS if you know the MAC address of your HC-08 module.
"""

# ── BLE Configuration ────────────────────────────────────────
BLE_DEVICE_NAME = "HC-08"  # Name to scan for (case-insensitive match)
BLE_DEVICE_ADDRESS: str | None = None  # Set MAC address to skip scanning, e.g. "AA:BB:CC:DD:EE:FF"

# HC-08 BLE UART Service & Characteristic UUIDs
BLE_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
BLE_CHAR_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"

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
