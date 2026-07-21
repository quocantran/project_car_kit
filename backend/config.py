"""
Configuration for the FastAPI Serial Bridge server (Raspberry Pi 4).
Arduino is connected via USB Serial cable.
"""

# ── Serial Configuration ─────────────────────────────────────
SERIAL_PORT: str | None = None  # Set to "/dev/ttyACM0" or "/dev/ttyUSB0". None = auto-detect
SERIAL_BAUDRATE = 115200
SERIAL_TIMEOUT = 1.0  # Seconds for serial read timeout
SERIAL_RECONNECT_DELAY = 3.0  # Seconds between reconnect attempts

# Auto-detect patterns (scanned in order)
SERIAL_PORT_PATTERNS = [
    "/dev/ttyACM*",   # Arduino Uno, Mega (ATmega16U2)
    "/dev/ttyUSB*",   # Arduino Nano, clones (CH340, CP2102)
]

# ── Telemetry Polling ────────────────────────────────────────
TELEMETRY_INTERVAL = 0.4  # Seconds between telemetry polls

# ── Server Configuration ─────────────────────────────────────
SERVER_HOST = "0.0.0.0"
SERVER_PORT = 8000

# ── CORS ─────────────────────────────────────────────────────
ALLOWED_ORIGINS = [
    "http://localhost:3000",
    "http://127.0.0.1:3000",
    "http://localhost:3001",
    "http://localhost:8000",
    "http://192.168.50.254:3000",
    "*",  # Allow any device on local WiFi network
]

# ── GPIO Configuration (Raspberry Pi 4) ──────────────────────
# BCM numbering — matches breadboard wiring via GPIO Extension Board
GPIO_LED_RED_PIN = 17      # Pin 11 → LED Đỏ
GPIO_LED_GREEN_PIN = 27    # Pin 13 → LED Xanh
GPIO_BUZZER_PIN = 22       # Pin 15 → Buzzer
GPIO_BUTTON_PIN = 23       # Pin 16 → Button (pull-up)

# ── Arduino Protocol Defaults ────────────────────────────────
DEFAULT_SPEED = 200  # Default PWM speed (0-255)
MAX_SPEED = 255
MIN_SPEED = 0
SERVO_MIN_ANGLE = 5
SERVO_MAX_ANGLE = 175
SERIAL_RESPONSE_TIMEOUT = 2.0  # Seconds to wait for Arduino response
