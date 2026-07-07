"""
Serial Service — Manages USB Serial connection to Arduino on Raspberry Pi 4.

Replaces BLEService for direct USB Serial communication.
Arduino connects to Pi via USB cable, appearing as /dev/ttyACM0 or /dev/ttyUSB0.

Key advantages over BLE:
- No MTU chunking needed (no 20-byte limit)
- No pairing/scanning required
- Much faster and more reliable
- Direct UART-style communication
"""

import asyncio
import glob
import logging
from typing import Callable, Optional

import serial
import serial.tools.list_ports
import serial_asyncio

from config import (
    SERIAL_BAUDRATE,
    SERIAL_PORT,
    SERIAL_PORT_PATTERNS,
    SERIAL_RESPONSE_TIMEOUT,
    SERIAL_TIMEOUT,
    TELEMETRY_INTERVAL,
)
from car_protocol import (
    build_distance_query,
    build_move_command,
    build_stop_command,
    build_telemetry_query,
    parse_response,
    parse_telemetry_response,
)

logger = logging.getLogger("serial_service")


class SerialService:
    """
    Manages the USB Serial connection to Arduino on Raspberry Pi 4.

    Features:
    - Auto-detect Arduino serial port (/dev/ttyACM* or /dev/ttyUSB*)
    - Async serial read/write via pyserial-asyncio
    - Send JSON commands and receive responses
    - Background telemetry polling loop
    - Broadcast telemetry to registered WebSocket listeners
    """

    def __init__(self):
        self._serial: Optional[serial.Serial] = None
        self.is_connected: bool = False
        self.serial_port: Optional[str] = None
        self.baudrate: int = SERIAL_BAUDRATE

        # Response handling
        self._response_buffer: str = ""
        self._response_event: asyncio.Event = asyncio.Event()
        self._last_response: Optional[dict] = None

        # Current state
        self.current_speed: int = 0
        self.current_direction: str = "stop"
        self.current_mode: str = "idle"
        self.last_distance: int = 0
        self.battery_percent: int = 100  # Mock value (no hardware sensor)

        # WebSocket broadcast callbacks
        self._listeners: list[Callable] = []

        # Background tasks
        self._telemetry_task: Optional[asyncio.Task] = None
        self._reader_task: Optional[asyncio.Task] = None

    # ── Port Detection ───────────────────────────────────────

    @staticmethod
    def detect_arduino_port() -> Optional[str]:
        """
        Auto-detect Arduino serial port on Raspberry Pi.

        Scans for:
        - /dev/ttyACM* (Arduino Uno, Mega — ATmega16U2 USB chip)
        - /dev/ttyUSB* (Arduino Nano clones — CH340, CP2102 USB chip)

        Returns the first port found, or None.
        """
        for pattern in SERIAL_PORT_PATTERNS:
            ports = sorted(glob.glob(pattern))
            if ports:
                logger.info(f"Auto-detected Arduino port: {ports[0]}")
                return ports[0]

        # Fallback: use pyserial's port listing
        available = serial.tools.list_ports.comports()
        for port_info in available:
            desc = (port_info.description or "").lower()
            if any(keyword in desc for keyword in ["arduino", "ch340", "cp210", "usb serial", "acm"]):
                logger.info(f"Auto-detected Arduino port via description: {port_info.device}")
                return port_info.device

        return None

    @staticmethod
    def list_serial_ports() -> list[dict]:
        """List all available serial ports with details."""
        result = []
        available = serial.tools.list_ports.comports()
        for port_info in available:
            result.append({
                "port": port_info.device,
                "description": port_info.description or "Unknown",
                "hwid": port_info.hwid or "",
                "manufacturer": port_info.manufacturer or "",
            })
        return result

    # ── Connection Management ────────────────────────────────

    async def connect(self, port: Optional[str] = None) -> bool:
        """
        Connect to Arduino via USB Serial.

        Args:
            port: Serial port path (e.g. '/dev/ttyACM0'). Auto-detect if None.

        Returns:
            True if connection successful.
        """
        if self.is_connected and self._serial:
            logger.info("Already connected, disconnecting first...")
            await self.disconnect()

        target_port = port or SERIAL_PORT

        # Auto-detect if no port specified
        if not target_port:
            logger.info("Auto-detecting Arduino serial port...")
            target_port = self.detect_arduino_port()

        if not target_port:
            logger.error("Could not find Arduino serial port. Is Arduino connected via USB?")
            return False

        try:
            logger.info(f"Connecting to {target_port} at {self.baudrate} baud...")
            self._serial = serial.Serial(
                port=target_port,
                baudrate=self.baudrate,
                timeout=SERIAL_TIMEOUT,
                write_timeout=SERIAL_TIMEOUT,
            )

            # Wait for Arduino to reset (Arduino resets when serial connection opens)
            await asyncio.sleep(2.0)

            # Flush any startup garbage
            if self._serial.in_waiting:
                self._serial.read(self._serial.in_waiting)

            self.is_connected = True
            self.serial_port = target_port
            logger.info(f"Connected to Arduino on {target_port} at {self.baudrate} baud")

            # Start background reader task
            self._reader_task = asyncio.create_task(self._serial_reader_loop())

            # Start telemetry polling
            self._telemetry_task = asyncio.create_task(self._telemetry_loop())

            # Broadcast connection status
            await self._broadcast({
                "type": "connection_status",
                "data": {
                    "is_connected": True,
                    "serial_port": self.serial_port,
                    "baudrate": self.baudrate,
                },
            })

            return True

        except serial.SerialException as e:
            logger.error(f"Serial connection error: {e}")
            self.is_connected = False
            return False
        except Exception as e:
            logger.error(f"Unexpected connection error: {e}")
            self.is_connected = False
            return False

    async def disconnect(self) -> bool:
        """Disconnect from the Arduino serial port."""
        try:
            # Cancel background tasks
            for task in [self._telemetry_task, self._reader_task]:
                if task and not task.done():
                    task.cancel()
                    try:
                        await task
                    except asyncio.CancelledError:
                        pass

            # Close serial port
            if self._serial and self._serial.is_open:
                self._serial.close()

            self.is_connected = False
            self.current_direction = "stop"
            self.current_speed = 0
            self.current_mode = "idle"

            logger.info("Disconnected from Arduino serial port")

            # Broadcast disconnection
            await self._broadcast({
                "type": "connection_status",
                "data": {
                    "is_connected": False,
                    "serial_port": self.serial_port,
                    "baudrate": self.baudrate,
                },
            })

            return True

        except Exception as e:
            logger.error(f"Disconnect error: {e}")
            self.is_connected = False
            return False

    # ── Data Communication ───────────────────────────────────

    async def _serial_reader_loop(self):
        """
        Background task that continuously reads data from Arduino via serial.
        Runs in a thread executor to avoid blocking the async event loop.
        """
        logger.info("Serial reader loop started")
        loop = asyncio.get_running_loop()

        while self.is_connected and self._serial and self._serial.is_open:
            try:
                # Read from serial in a thread (blocking I/O)
                data = await loop.run_in_executor(None, self._read_serial_line)

                if data:
                    self._process_incoming_data(data)

            except asyncio.CancelledError:
                logger.info("Serial reader loop cancelled")
                break
            except serial.SerialException as e:
                logger.error(f"Serial read error: {e}")
                self.is_connected = False
                break
            except Exception as e:
                logger.error(f"Serial reader error: {e}")
                await asyncio.sleep(0.1)

        logger.info("Serial reader loop stopped")

    def _read_serial_line(self) -> Optional[str]:
        """Read a line from serial port (blocking — called in executor)."""
        if not self._serial or not self._serial.is_open:
            return None

        try:
            if self._serial.in_waiting > 0:
                raw = self._serial.read(self._serial.in_waiting)
                return raw.decode("utf-8", errors="replace")
            else:
                # Brief sleep to prevent busy-waiting
                import time
                time.sleep(0.05)
                return None
        except Exception:
            return None

    def _process_incoming_data(self, data: str):
        """Process data received from Arduino."""
        self._response_buffer += data
        logger.info(f"📥 Serial RX: {data.strip()}")

        # Check if we have a complete response (ends with })
        while "}" in self._response_buffer:
            # Extract the first complete message
            end_idx = self._response_buffer.index("}") + 1
            start_idx = self._response_buffer.rfind("{", 0, end_idx)

            if start_idx == -1:
                # No opening brace found, discard up to }
                self._response_buffer = self._response_buffer[end_idx:]
                continue

            raw = self._response_buffer[start_idx:end_idx]
            self._response_buffer = self._response_buffer[end_idx:]

            # Try to parse as telemetry JSON first (N=100 response)
            telemetry = parse_telemetry_response(raw)
            if telemetry:
                self._last_response = {"telemetry": telemetry}
                self.last_distance = telemetry["distance"]
                self.current_direction = telemetry["direction"]
                self.current_mode = telemetry["mode"]
                if telemetry["speed"] > 0:
                    self.current_speed = telemetry["speed"]
            else:
                self._last_response = parse_response(raw)

            self._response_event.set()

    async def send_command(self, command: str, wait_response: bool = True) -> Optional[dict]:
        """
        Send a JSON command string to the Arduino via USB Serial.

        Args:
            command: JSON string (e.g. '{"N":2,"D1":3,"D2":200,"H":1}')
            wait_response: Whether to wait for Arduino's response

        Returns:
            Parsed response dict if wait_response=True, else None.
        """
        if not self.is_connected or not self._serial or not self._serial.is_open:
            logger.warning("Cannot send command — not connected")
            return None

        try:
            self._response_event.clear()
            self._last_response = None

            # Write command directly — no chunking needed for USB Serial!
            cmd_bytes = (command + "\n").encode("utf-8")  # Add newline for cleaner framing
            self._serial.write(cmd_bytes)
            self._serial.flush()
            logger.info(f"📤 Serial TX ({len(cmd_bytes)} bytes): {command}")

            if wait_response:
                try:
                    await asyncio.wait_for(
                        self._response_event.wait(),
                        timeout=SERIAL_RESPONSE_TIMEOUT,
                    )
                    return self._last_response
                except asyncio.TimeoutError:
                    logger.debug(f"Response timeout for command: {command}")
                    return None
            return None

        except serial.SerialException as e:
            logger.error(f"Serial write error: {e}")
            return None
        except Exception as e:
            logger.error(f"Send command error: {e}")
            return None

    # ── High-Level Car Control ───────────────────────────────

    async def move(self, direction: str, speed: int = 200) -> Optional[dict]:
        """Send a movement command to the car."""
        cmd = build_move_command(direction, speed)
        logger.info(f"🚗 MOVE: direction={direction}, speed={speed}, cmd={cmd}")
        self.current_direction = direction
        self.current_speed = speed if direction != "stop" else 0
        self.current_mode = "bluetooth"
        result = await self.send_command(cmd, wait_response=False)

        # Broadcast state update
        await self._broadcast_telemetry()
        return result

    async def stop(self) -> Optional[dict]:
        """Stop the car."""
        return await self.move("stop", 0)

    async def set_speed(self, speed: int) -> Optional[dict]:
        """Update speed while maintaining current direction."""
        if self.current_direction == "stop":
            self.current_speed = speed
            await self._broadcast_telemetry()
            return None
        return await self.move(self.current_direction, speed)

    async def get_distance(self) -> int:
        """Read ultrasonic distance from the car."""
        cmd = build_distance_query()
        response = await self.send_command(cmd, wait_response=True)
        if response and response.get("value") is not None:
            try:
                self.last_distance = int(response["value"])
            except (ValueError, TypeError):
                pass
        return self.last_distance

    # ── Telemetry Loop ───────────────────────────────────────

    async def _telemetry_loop(self):
        """
        Background task that polls full telemetry via N=100 command
        and broadcasts to WebSocket clients.
        """
        logger.info("Telemetry loop started")
        while self.is_connected:
            try:
                # Poll full telemetry from Arduino (N=100)
                cmd = build_telemetry_query()
                response = await self.send_command(cmd, wait_response=True)

                # If N=100 failed, fall back to distance-only query
                if not response or "telemetry" not in response:
                    await self.get_distance()

                # Simulate battery drain (mock — no real sensor)
                if self.current_speed > 0:
                    self.battery_percent = max(0, self.battery_percent - 0.01)

                # Broadcast telemetry to all WebSocket clients
                await self._broadcast_telemetry()

            except asyncio.CancelledError:
                logger.info("Telemetry loop cancelled")
                break
            except Exception as e:
                logger.error(f"Telemetry loop error: {e}")

            await asyncio.sleep(TELEMETRY_INTERVAL)

        logger.info("Telemetry loop stopped")

    # ── WebSocket Broadcasting ───────────────────────────────

    def add_listener(self, callback: Callable):
        """Register a WebSocket broadcast callback."""
        self._listeners.append(callback)

    def remove_listener(self, callback: Callable):
        """Unregister a WebSocket broadcast callback."""
        if callback in self._listeners:
            self._listeners.remove(callback)

    async def _broadcast(self, message: dict):
        """Send a message to all registered WebSocket listeners."""
        dead_listeners = []
        for listener in self._listeners:
            try:
                await listener(message)
            except Exception:
                dead_listeners.append(listener)

        # Clean up dead listeners
        for dl in dead_listeners:
            self._listeners.remove(dl)

    async def _broadcast_telemetry(self):
        """Broadcast current telemetry state."""
        actual_speed = 0 if self.current_direction == "stop" else self.current_speed
        await self._broadcast({
            "type": "telemetry",
            "data": {
                "battery": int(self.battery_percent),
                "speed": actual_speed,
                "speed_percent": int(actual_speed / 255 * 100),
                "distance": self.last_distance,
                "is_connected": self.is_connected,
                "current_direction": self.current_direction,
                "current_mode": self.current_mode,
            },
        })

    # ── State Getters ────────────────────────────────────────

    def get_telemetry(self) -> dict:
        """Get current telemetry snapshot."""
        actual_speed = 0 if self.current_direction == "stop" else self.current_speed
        return {
            "battery": int(self.battery_percent),
            "speed": actual_speed,
            "speed_percent": int(actual_speed / 255 * 100),
            "distance": self.last_distance,
            "is_connected": self.is_connected,
            "current_direction": self.current_direction,
            "current_mode": self.current_mode,
        }

    def get_connection_status(self) -> dict:
        """Get serial connection status."""
        return {
            "is_connected": self.is_connected,
            "serial_port": self.serial_port,
            "baudrate": self.baudrate,
        }
