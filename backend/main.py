"""
FastAPI BLE Bridge — Main Application

REST API + WebSocket server that bridges the Next.js frontend
with the Elegoo Smart Robot Car V3.0 Plus via Bluetooth Low Energy.

Run with:
    cd backend && uvicorn main:app --reload --host 0.0.0.0 --port 8000

Swagger docs:
    http://localhost:8000/docs
"""

import asyncio
import json
import logging
from contextlib import asynccontextmanager
from typing import Optional

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware

from config import ALLOWED_ORIGINS, SERVER_HOST, SERVER_PORT
from ble_service import BLEService
from car_protocol import (
    build_mode_command,
    build_reset_command,
    build_servo_command,
)
from models import (
    BLEStatus,
    CommandResponse,
    ConnectRequest,
    ControlCommand,
    DistanceResponse,
    ModeCommand,
    ServoCommand,
    SpeedUpdate,
    StatusResponse,
    TelemetryData,
)

# ── Logging Setup ────────────────────────────────────────────
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s │ %(name)-12s │ %(levelname)-7s │ %(message)s",
    datefmt="%H:%M:%S",
)
logger = logging.getLogger("main")

# ── Shared BLE Service Instance ──────────────────────────────
ble_service = BLEService()


# ── App Lifespan ─────────────────────────────────────────────
@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application startup and shutdown events."""
    logger.info("🚗 FastAPI BLE Bridge starting...")
    logger.info("   Swagger UI: http://localhost:8000/docs")
    logger.info("   WebSocket:  ws://localhost:8000/ws")

    # Attempt auto-connect on startup (non-blocking)
    asyncio.create_task(_auto_connect())

    yield

    # Shutdown: disconnect BLE
    logger.info("Shutting down — disconnecting BLE...")
    await ble_service.disconnect()
    logger.info("Goodbye!")


async def _auto_connect():
    """Try to auto-connect to HC-08 on startup."""
    await asyncio.sleep(1)  # Give server a moment to start
    try:
        result = await ble_service.connect()
        if result:
            logger.info("✅ Auto-connected to HC-08")
        else:
            logger.info("⚠️  HC-08 not found — use POST /api/connect to connect manually")
    except Exception as e:
        logger.info(f"⚠️  Auto-connect failed: {e} — use POST /api/connect")


# ── FastAPI App ──────────────────────────────────────────────
app = FastAPI(
    title="Robot Car BLE Bridge",
    description="FastAPI backend that bridges the web dashboard with the Elegoo Smart Robot Car V3.0 via Bluetooth Low Energy.",
    version="1.0.0",
    lifespan=lifespan,
)

# CORS middleware for Next.js frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=ALLOWED_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ══════════════════════════════════════════════════════════════
#  REST API ENDPOINTS
# ══════════════════════════════════════════════════════════════


@app.get("/", tags=["Health"])
async def root():
    """Health check endpoint."""
    return {
        "service": "Robot Car BLE Bridge",
        "version": "1.0.0",
        "ble_connected": ble_service.is_connected,
    }


# ── BLE Connection ───────────────────────────────────────────

@app.get("/api/status", response_model=StatusResponse, tags=["Status"])
async def get_status():
    """Get current BLE connection status and telemetry data."""
    telemetry = ble_service.get_telemetry()
    ble_status = ble_service.get_ble_status()
    return StatusResponse(
        ble=BLEStatus(**ble_status),
        telemetry=TelemetryData(**telemetry),
    )


@app.post("/api/connect", response_model=CommandResponse, tags=["Connection"])
async def connect_ble(request: ConnectRequest = ConnectRequest()):
    """Scan for and connect to the HC-08 BLE module on the robot."""
    result = await ble_service.connect(request.device_address)
    if result:
        return CommandResponse(
            success=True,
            message=f"Connected to {ble_service.device_name} [{ble_service.device_address}]",
        )
    raise HTTPException(
        status_code=503,
        detail="Failed to connect to HC-08. Make sure the robot is powered on and Bluetooth is enabled on this computer.",
    )


@app.post("/api/disconnect", response_model=CommandResponse, tags=["Connection"])
async def disconnect_ble():
    """Disconnect from the robot's BLE module."""
    await ble_service.disconnect()
    return CommandResponse(success=True, message="Disconnected from BLE device")


@app.get("/api/scan", tags=["Connection"])
async def scan_ble_devices():
    """Scan for nearby BLE devices."""
    devices = await ble_service.scan_devices()
    return {"devices": devices, "count": len(devices)}


# ── Car Control ──────────────────────────────────────────────

@app.post("/api/control", response_model=CommandResponse, tags=["Control"])
async def control_car(command: ControlCommand):
    """
    Control the car's movement direction and speed.

    Directions: forward, back, left, right, stop, forward_left, back_left, forward_right, back_right.
    Speed: 0-255 (PWM duty cycle).
    """
    if not ble_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to robot")

    await ble_service.move(command.direction, command.speed)
    return CommandResponse(
        success=True,
        message=f"Moving {command.direction} at speed {command.speed}",
    )


@app.post("/api/speed", response_model=CommandResponse, tags=["Control"])
async def update_speed(update: SpeedUpdate):
    """Update the car's speed without changing direction."""
    if not ble_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to robot")

    await ble_service.set_speed(update.speed)
    return CommandResponse(
        success=True,
        message=f"Speed updated to {update.speed} ({int(update.speed / 255 * 100)}%)",
    )


@app.post("/api/stop", response_model=CommandResponse, tags=["Control"])
async def stop_car():
    """Emergency stop — immediately stops all motors."""
    if not ble_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to robot")

    await ble_service.stop()
    return CommandResponse(success=True, message="Car stopped")


@app.post("/api/servo", response_model=CommandResponse, tags=["Control"])
async def control_servo(command: ServoCommand):
    """Set the ultrasonic sensor servo angle (5-175 degrees)."""
    if not ble_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to robot")

    cmd = build_servo_command(command.angle)
    response = await ble_service.send_command(cmd)
    return CommandResponse(
        success=True,
        message=f"Servo set to {command.angle}°",
        arduino_response=str(response) if response else None,
    )


@app.post("/api/mode", response_model=CommandResponse, tags=["Control"])
async def set_mode(command: ModeCommand):
    """Switch to autonomous driving mode (line tracking or obstacle avoidance)."""
    if not ble_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to robot")

    cmd = build_mode_command(command.mode)
    ble_service.current_mode = command.mode
    response = await ble_service.send_command(cmd)
    return CommandResponse(
        success=True,
        message=f"Mode set to {command.mode}",
        arduino_response=str(response) if response else None,
    )


@app.post("/api/reset", response_model=CommandResponse, tags=["Control"])
async def reset_car():
    """Reset all functions and stop the car (N=5 command)."""
    if not ble_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to robot")

    cmd = build_reset_command()
    response = await ble_service.send_command(cmd)
    ble_service.current_direction = "stop"
    ble_service.current_speed = 0
    ble_service.current_mode = "idle"
    return CommandResponse(
        success=True,
        message="All functions reset",
        arduino_response=str(response) if response else None,
    )


# ── Sensor Data ──────────────────────────────────────────────

@app.get("/api/distance", response_model=DistanceResponse, tags=["Sensors"])
async def get_distance():
    """Read the ultrasonic distance sensor (cm)."""
    if not ble_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to robot")

    distance = await ble_service.get_distance()
    return DistanceResponse(
        distance_cm=distance,
        has_obstacle=distance < 35,
    )


# ══════════════════════════════════════════════════════════════
#  WEBSOCKET ENDPOINT
# ══════════════════════════════════════════════════════════════

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """
    WebSocket endpoint for real-time bidirectional communication.

    Server → Client messages:
        {type: "telemetry", data: {...}}
        {type: "connection_status", data: {...}}
        {type: "command_response", data: {...}}
        {type: "error", data: {message: "..."}}

    Client → Server messages:
        {type: "control", direction: "forward", speed: 200}
        {type: "speed", speed: 150}
        {type: "servo", angle: 90}
        {type: "mode", mode: "line_tracking"}
        {type: "reset"}
        {type: "ping"}
    """
    await websocket.accept()
    logger.info("WebSocket client connected")

    # Register broadcast listener for this WebSocket
    async def ws_listener(message: dict):
        try:
            await websocket.send_json(message)
        except Exception:
            pass

    ble_service.add_listener(ws_listener)

    # Send initial status
    await websocket.send_json({
        "type": "connection_status",
        "data": ble_service.get_ble_status(),
    })
    await websocket.send_json({
        "type": "telemetry",
        "data": ble_service.get_telemetry(),
    })

    try:
        while True:
            # Receive message from client
            raw = await websocket.receive_text()
            try:
                msg = json.loads(raw)
            except json.JSONDecodeError:
                await websocket.send_json({
                    "type": "error",
                    "data": {"message": "Invalid JSON"},
                })
                continue

            msg_type = msg.get("type", "")

            # Handle different message types
            if msg_type == "control":
                direction = msg.get("direction", "stop")
                speed = msg.get("speed", 200)
                if ble_service.is_connected:
                    await ble_service.move(direction, speed)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Moving {direction} at {speed}"},
                    })
                else:
                    await websocket.send_json({
                        "type": "error",
                        "data": {"message": "Not connected to robot"},
                    })

            elif msg_type == "speed":
                speed = msg.get("speed", 200)
                if ble_service.is_connected:
                    await ble_service.set_speed(speed)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Speed set to {speed}"},
                    })

            elif msg_type == "servo":
                angle = msg.get("angle", 90)
                if ble_service.is_connected:
                    cmd = build_servo_command(angle)
                    await ble_service.send_command(cmd)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Servo set to {angle}°"},
                    })

            elif msg_type == "mode":
                mode = msg.get("mode", "line_tracking")
                if ble_service.is_connected:
                    cmd = build_mode_command(mode)
                    ble_service.current_mode = mode
                    await ble_service.send_command(cmd)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Mode: {mode}"},
                    })

            elif msg_type == "reset":
                if ble_service.is_connected:
                    cmd = build_reset_command()
                    await ble_service.send_command(cmd)
                    ble_service.current_direction = "stop"
                    ble_service.current_speed = 0
                    ble_service.current_mode = "idle"
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": "Reset complete"},
                    })

            elif msg_type == "connect":
                address = msg.get("device_address")
                result = await ble_service.connect(address)
                await websocket.send_json({
                    "type": "connection_status",
                    "data": ble_service.get_ble_status(),
                })

            elif msg_type == "disconnect":
                await ble_service.disconnect()
                await websocket.send_json({
                    "type": "connection_status",
                    "data": ble_service.get_ble_status(),
                })

            elif msg_type == "ping":
                await websocket.send_json({"type": "pong", "data": {}})

            else:
                await websocket.send_json({
                    "type": "error",
                    "data": {"message": f"Unknown message type: {msg_type}"},
                })

    except WebSocketDisconnect:
        logger.info("WebSocket client disconnected")
    except Exception as e:
        logger.error(f"WebSocket error: {e}")
    finally:
        ble_service.remove_listener(ws_listener)


# ══════════════════════════════════════════════════════════════
#  MAIN ENTRY POINT
# ══════════════════════════════════════════════════════════════

if __name__ == "__main__":
    import uvicorn

    uvicorn.run(
        "main:app",
        host=SERVER_HOST,
        port=SERVER_PORT,
        reload=True,
        log_level="info",
    )
