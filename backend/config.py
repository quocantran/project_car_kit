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
TELEMETRY_INTERVAL = 1.0  # Seconds between telemetry polls

# ── Server Configuration ─────────────────────────────────────
SERVER_HOST = "0.0.0.0"
SERVER_PORT = 8000

# ── CORS ─────────────────────────────────────────────────────
ALLOWED_ORIGINS = [
    "http://localhost:3000",
    "http://127.0.0.1:3000",
    "http://localhost:3001",
    "http://localhost:8000",
    # Allow any device on local network to access
    # Add your Pi's IP here, e.g. "http://192.168.1.100:3000"
]

# ── Arduino Protocol Defaults ────────────────────────────────
DEFAULT_SPEED = 200  # Default PWM speed (0-255)
MAX_SPEED = 255
MIN_SPEED = 0
SERVO_MIN_ANGLE = 5
SERVO_MAX_ANGLE = 175
SERIAL_RESPONSE_TIMEOUT = 2.0  # Seconds to wait for Arduino response
