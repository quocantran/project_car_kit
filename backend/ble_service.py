"""
BLE Service — Manages Bluetooth Low Energy connection to ELEGOO BT16 module.

Uses the `bleak` library for async BLE communication.
The ELEGOO BT16 (DX-BT16) is a BLE 4.2 transparent transmission module that
exposes two GATT characteristics under service FFE0:
  - FFE1: Notify/Read (receive data FROM Arduino)
  - FFE2: Write (send commands TO Arduino)

Key BT16 considerations:
- Default MTU is 23 bytes → max write payload is 20 bytes per packet.
- Commands longer than 20 bytes must be split into chunks.
- Uses Write Without Response on FFE2.
"""

import asyncio
import logging
from typing import Callable, Optional

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

from config import (
    BLE_CHAR_NOTIFY_UUID,
    BLE_CHAR_WRITE_UUID,
    BLE_CHUNK_DELAY,
    BLE_DEVICE_ADDRESS,
    BLE_DEVICE_NAME,
    BLE_RESPONSE_TIMEOUT,
    BLE_SCAN_TIMEOUT,
    BLE_WRITE_CHUNK_SIZE,
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

logger = logging.getLogger("ble_service")


class BLEService:
    """
    Manages the BLE connection lifecycle to the ELEGOO BT16 module on the robot car.

    Features:
    - Auto-scan for ELEGOO BT16 device by name
    - Persistent connection with auto-reconnect awareness
    - Send JSON commands via FFE2 and receive responses via FFE1
    - Chunked BLE writes for commands exceeding 20-byte MTU payload
    - Background telemetry polling loop
    - Broadcast telemetry to registered WebSocket listeners
    """

    def __init__(self):
        self.client: Optional[BleakClient] = None
        self.is_connected: bool = False
        self.device_name: Optional[str] = None
        self.device_address: Optional[str] = None

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

    # ── Connection Management ────────────────────────────────

    async def scan_devices(self) -> list[dict]:
        """Scan for nearby BLE devices and return a list."""
        logger.info("Scanning for BLE devices...")
        devices = await BleakScanner.discover(timeout=BLE_SCAN_TIMEOUT)
        result = []
        for d in devices:
            result.append({
                "name": d.name or "Unknown",
                "address": d.address,
                "rssi": d.rssi if hasattr(d, "rssi") else None,
            })
            logger.debug(f"  Found: {d.name} [{d.address}]")
        return result

    async def connect(self, device_address: Optional[str] = None) -> bool:
        """
        Connect to the HC-02 BLE module.

        Args:
            device_address: MAC address to connect to. If None, scans for HC-02.

        Returns:
            True if connection successful.
        """
        if self.is_connected and self.client:
            logger.info("Already connected, disconnecting first...")
            await self.disconnect()

        target_address = device_address or BLE_DEVICE_ADDRESS

        # If no address provided, scan for HC-02
        if not target_address:
            logger.info(f"Scanning for device named '{BLE_DEVICE_NAME}'...")
            devices = await BleakScanner.discover(timeout=BLE_SCAN_TIMEOUT)
            for d in devices:
                if d.name and BLE_DEVICE_NAME.lower() in d.name.lower():
                    target_address = d.address
                    logger.info(f"Found {d.name} at {d.address}")
                    break

            if not target_address:
                logger.error(f"Could not find device named '{BLE_DEVICE_NAME}'")
                return False

        # Connect to device
        try:
            logger.info(f"Connecting to {target_address}...")
            self.client = BleakClient(
                target_address,
                disconnected_callback=self._on_disconnected,
            )
            await self.client.connect()

            if self.client.is_connected:
                self.is_connected = True
                self.device_address = target_address

                # Try to get device name from services
                try:
                    self.device_name = self.client._device_info if hasattr(self.client, '_device_info') else BLE_DEVICE_NAME
                except Exception:
                    self.device_name = BLE_DEVICE_NAME

                # Subscribe to notifications
                await self.client.start_notify(BLE_CHAR_NOTIFY_UUID, self._on_notification)
                logger.info(f"Connected to {self.device_name} [{self.device_address}]")

                # Start telemetry polling
                self._telemetry_task = asyncio.create_task(self._telemetry_loop())

                # Broadcast connection status
                await self._broadcast({
                    "type": "connection_status",
                    "data": {
                        "is_connected": True,
                        "device_name": self.device_name,
                        "device_address": self.device_address,
                    },
                })

                return True
            else:
                logger.error("Connection failed — client reports not connected")
                return False

        except BleakError as e:
            logger.error(f"BLE connection error: {e}")
            self.is_connected = False
            return False
        except Exception as e:
            logger.error(f"Unexpected connection error: {e}")
            self.is_connected = False
            return False

    async def disconnect(self) -> bool:
        """Disconnect from the BLE device."""
        try:
            # Cancel telemetry task
            if self._telemetry_task and not self._telemetry_task.done():
                self._telemetry_task.cancel()
                try:
                    await self._telemetry_task
                except asyncio.CancelledError:
                    pass

            # Stop notifications and disconnect
            if self.client and self.client.is_connected:
                try:
                    await self.client.stop_notify(BLE_CHAR_NOTIFY_UUID)
                except Exception:
                    pass
                await self.client.disconnect()

            self.is_connected = False
            self.current_direction = "stop"
            self.current_speed = 0
            self.current_mode = "idle"

            logger.info("Disconnected from BLE device")

            # Broadcast disconnection
            await self._broadcast({
                "type": "connection_status",
                "data": {
                    "is_connected": False,
                    "device_name": self.device_name,
                    "device_address": self.device_address,
                },
            })

            return True

        except Exception as e:
            logger.error(f"Disconnect error: {e}")
            self.is_connected = False
            return False

    def _on_disconnected(self, client: BleakClient):
        """Callback when BLE connection is lost unexpectedly."""
        logger.warning("BLE device disconnected unexpectedly!")
        self.is_connected = False
        self.current_direction = "stop"
        self.current_speed = 0

        # Fire-and-forget broadcast (we're in a sync callback)
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(self._broadcast({
                "type": "connection_status",
                "data": {
                    "is_connected": False,
                    "device_name": self.device_name,
                    "device_address": self.device_address,
                },
            }))
        except RuntimeError:
            pass

    # ── Data Communication ───────────────────────────────────

    def _on_notification(self, sender, data: bytearray):
        """
        Callback when data is received from Arduino via BLE notification.
        HC-02 sends data as raw bytes representing the UART output.
        """
        try:
            decoded = data.decode("utf-8", errors="replace")
            logger.debug(f"BLE RX: {decoded}")
            self._response_buffer += decoded

            # Check if we have a complete response (ends with })
            if "}" in self._response_buffer:
                raw = self._response_buffer
                self._response_buffer = ""

                # Try to parse as telemetry JSON first (N=100 response)
                telemetry = parse_telemetry_response(raw)
                if telemetry:
                    self._last_response = {"telemetry": telemetry}
                    self.last_distance = telemetry["distance"]
                    self.current_direction = telemetry["direction"]
                    self.current_mode = telemetry["mode"]
                    # Update speed from Arduino's actual carSpeed_rocker
                    if telemetry["speed"] > 0:
                        self.current_speed = telemetry["speed"]
                else:
                    self._last_response = parse_response(raw)

                self._response_event.set()
        except Exception as e:
            logger.error(f"Notification decode error: {e}")

    async def send_command(self, command: str, wait_response: bool = True) -> Optional[dict]:
        """
        Send a JSON command string to the Arduino via BLE.

        Args:
            command: JSON string (e.g. '{"N":2,"D1":3,"D2":200,"H":1}')
            wait_response: Whether to wait for Arduino's response

        Returns:
            Parsed response dict if wait_response=True, else None.
        """
        if not self.is_connected or not self.client:
            logger.warning("Cannot send command — not connected")
            return None

        try:
            self._response_event.clear()
            self._last_response = None

            # Encode command and write via chunked BLE write
            cmd_bytes = command.encode("utf-8")
            await self._write_chunked(cmd_bytes)
            logger.debug(f"BLE TX: {command}")

            if wait_response:
                try:
                    await asyncio.wait_for(
                        self._response_event.wait(),
                        timeout=BLE_RESPONSE_TIMEOUT,
                    )
                    return self._last_response
                except asyncio.TimeoutError:
                    logger.debug(f"Response timeout for command: {command}")
                    return None
            return None

        except BleakError as e:
            logger.error(f"BLE write error: {e}")
            return None
        except Exception as e:
            logger.error(f"Send command error: {e}")
            return None

    async def _write_chunked(self, data: bytes):
        """
        Write data to BLE characteristic (FFE2) in chunks of BLE_WRITE_CHUNK_SIZE.

        ELEGOO BT16 has a default MTU of 23 bytes (20-byte payload). Commands
        longer than 20 bytes must be split into multiple BLE write operations
        with a short delay between each chunk to allow the module to process.
        """
        for i in range(0, len(data), BLE_WRITE_CHUNK_SIZE):
            chunk = data[i : i + BLE_WRITE_CHUNK_SIZE]
            await self.client.write_gatt_char(
                BLE_CHAR_WRITE_UUID, chunk, response=False
            )
            # Add delay between chunks so BT16 can process each one
            if i + BLE_WRITE_CHUNK_SIZE < len(data):
                await asyncio.sleep(BLE_CHUNK_DELAY)

    # ── High-Level Car Control ───────────────────────────────

    async def move(self, direction: str, speed: int = 200) -> Optional[dict]:
        """Send a movement command to the car."""
        cmd = build_move_command(direction, speed)
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
        await self._broadcast({
            "type": "telemetry",
            "data": {
                "battery": int(self.battery_percent),
                "speed": self.current_speed,
                "speed_percent": int(self.current_speed / 255 * 100),
                "distance": self.last_distance,
                "is_connected": self.is_connected,
                "current_direction": self.current_direction,
                "current_mode": self.current_mode,
            },
        })

    # ── State Getters ────────────────────────────────────────

    def get_telemetry(self) -> dict:
        """Get current telemetry snapshot."""
        return {
            "battery": int(self.battery_percent),
            "speed": self.current_speed,
            "speed_percent": int(self.current_speed / 255 * 100),
            "distance": self.last_distance,
            "is_connected": self.is_connected,
            "current_direction": self.current_direction,
            "current_mode": self.current_mode,
        }

    def get_ble_status(self) -> dict:
        """Get BLE connection status."""
        return {
            "is_connected": self.is_connected,
            "device_name": self.device_name,
            "device_address": self.device_address,
        }
