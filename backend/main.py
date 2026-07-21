"""
FastAPI Serial Bridge — Main Application

REST API + WebSocket server that bridges the Next.js frontend
with the Elegoo Smart Robot Car V3.0 Plus via USB Serial on Raspberry Pi 4.

Run with:
    cd backend && uvicorn main:app --reload --host 0.0.0.0 --port 8000

Swagger docs:
    http://<pi-ip>:8000/docs
"""

import asyncio
import json
import logging
from contextlib import asynccontextmanager
from typing import Optional

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware

from config import ALLOWED_ORIGINS, SERVER_HOST, SERVER_PORT
from serial_service import SerialService
from gpio_service import GpioService
from car_protocol import (
    build_mode_command,
    build_reset_command,
    build_servo_command,
    build_traffic_light_command,
)
from models import (
    CommandResponse,
    ConnectionStatus,
    ConnectRequest,
    ControlCommand,
    DistanceResponse,
    ModeCommand,
    ServoCommand,
    SpeedUpdate,
    StatusResponse,
    TelemetryData,
    TrafficLightCommand,
)

# ── Logging Setup ────────────────────────────────────────────
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s │ %(name)-12s │ %(levelname)-7s │ %(message)s",
    datefmt="%H:%M:%S",
)
logger = logging.getLogger("main")

# ── Shared Service Instances ─────────────────────────────────
serial_service = SerialService()
gpio_service = GpioService()


# ── App Lifespan ─────────────────────────────────────────────
@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application startup and shutdown events."""
    logger.info("🚗 FastAPI Serial Bridge starting on Raspberry Pi...")
    logger.info("   Swagger UI: http://0.0.0.0:8000/docs")
    logger.info("   WebSocket:  ws://0.0.0.0:8000/ws")

    # Initialize GPIO for traffic light LEDs
    gpio_service.setup(button_callback=_on_button_toggle)
    serial_service.set_gpio_service(gpio_service)

    # Attempt auto-connect on startup (non-blocking)
    asyncio.create_task(_auto_connect())

    yield

    # Shutdown: disconnect serial, cleanup GPIO
    logger.info("Shutting down — disconnecting serial, cleaning up GPIO...")
    await serial_service.disconnect()
    gpio_service.cleanup()
    logger.info("Goodbye!")


async def _auto_connect():
    """Try to auto-connect to Arduino on startup."""
    await asyncio.sleep(1)  # Give server a moment to start
    try:
        result = await serial_service.connect()
        if result:
            logger.info(f"✅ Auto-connected to Arduino on {serial_service.serial_port}")
        else:
            logger.info("⚠️  Arduino not found — use POST /api/connect to connect manually")
    except Exception as e:
        logger.info(f"⚠️  Auto-connect failed: {e} — use POST /api/connect")


def _on_button_toggle():
    """Callback from GPIO button press — toggle traffic light."""
    new_state = gpio_service.traffic_light
    logger.info(f"🔘 Button pressed — traffic light is now {new_state}")
    # The GPIO service already toggled the LEDs, now sync with Arduino
    asyncio.create_task(_sync_traffic_light(new_state))


async def _sync_traffic_light(state: str):
    """Sync traffic light state to Arduino + broadcast to WebSocket clients."""
    if serial_service.is_connected:
        await serial_service.set_traffic_light(state)


# ── FastAPI App ──────────────────────────────────────────────
app = FastAPI(
    title="Robot Car Serial Bridge",
    description="FastAPI backend that bridges the web dashboard with the Elegoo Smart Robot Car V3.0 via USB Serial on Raspberry Pi 4.",
    version="2.0.0",
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
        "service": "Robot Car Serial Bridge",
        "version": "2.0.0",
        "platform": "Raspberry Pi 4",
        "serial_connected": serial_service.is_connected,
        "serial_port": serial_service.serial_port,
    }


# ── Serial Connection ───────────────────────────────────────

@app.get("/api/status", response_model=StatusResponse, tags=["Status"])
async def get_status():
    """Get current serial connection status and telemetry data."""
    telemetry = serial_service.get_telemetry()
    conn_status = serial_service.get_connection_status()
    return StatusResponse(
        connection=ConnectionStatus(**conn_status),
        telemetry=TelemetryData(**telemetry),
    )


@app.post("/api/connect", response_model=CommandResponse, tags=["Connection"])
async def connect_serial(request: ConnectRequest = ConnectRequest()):
    """Connect to Arduino via USB Serial. Auto-detects port if not specified."""
    result = await serial_service.connect(request.serial_port)
    if result:
        return CommandResponse(
            success=True,
            message=f"Connected to Arduino on {serial_service.serial_port} at {serial_service.baudrate} baud",
        )
    raise HTTPException(
        status_code=503,
        detail="Failed to connect to Arduino. Make sure Arduino is connected via USB cable to Raspberry Pi.",
    )


@app.post("/api/disconnect", response_model=CommandResponse, tags=["Connection"])
async def disconnect_serial():
    """Disconnect from Arduino serial port."""
    await serial_service.disconnect()
    return CommandResponse(success=True, message="Disconnected from Arduino")


@app.get("/api/ports", tags=["Connection"])
async def list_ports():
    """List all available serial ports on the Raspberry Pi."""
    ports = serial_service.list_serial_ports()
    return {"ports": ports, "count": len(ports)}


# ── Car Control ──────────────────────────────────────────────

@app.post("/api/control", response_model=CommandResponse, tags=["Control"])
async def control_car(command: ControlCommand):
    """
    Control the car's movement direction and speed.

    Directions: forward, back, left, right, stop, forward_left, back_left, forward_right, back_right.
    Speed: 0-255 (PWM duty cycle).
    """
    if not serial_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to Arduino")

    await serial_service.move(command.direction, command.speed)
    return CommandResponse(
        success=True,
        message=f"Moving {command.direction} at speed {command.speed}",
    )


@app.post("/api/speed", response_model=CommandResponse, tags=["Control"])
async def update_speed(update: SpeedUpdate):
    """Update the car's speed without changing direction."""
    if not serial_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to Arduino")

    await serial_service.set_speed(update.speed)
    return CommandResponse(
        success=True,
        message=f"Speed updated to {update.speed} ({int(update.speed / 255 * 100)}%)",
    )


@app.post("/api/stop", response_model=CommandResponse, tags=["Control"])
async def stop_car():
    """Emergency stop — immediately stops all motors."""
    if not serial_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to Arduino")

    await serial_service.stop()
    return CommandResponse(success=True, message="Car stopped")


@app.post("/api/servo", response_model=CommandResponse, tags=["Control"])
async def control_servo(command: ServoCommand):
    """Set the ultrasonic sensor servo angle (5-175 degrees)."""
    if not serial_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to Arduino")

    cmd = build_servo_command(command.angle)
    response = await serial_service.send_command(cmd)
    return CommandResponse(
        success=True,
        message=f"Servo set to {command.angle}°",
        arduino_response=str(response) if response else None,
    )


@app.post("/api/mode", response_model=CommandResponse, tags=["Control"])
async def set_mode(command: ModeCommand):
    """Switch to autonomous driving mode (line tracking or obstacle avoidance)."""
    if not serial_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to Arduino")

    cmd = build_mode_command(command.mode)
    serial_service.current_mode = command.mode
    response = await serial_service.send_command(cmd)
    return CommandResponse(
        success=True,
        message=f"Mode set to {command.mode}",
        arduino_response=str(response) if response else None,
    )


@app.post("/api/reset", response_model=CommandResponse, tags=["Control"])
async def reset_car():
    """Reset all functions and stop the car (N=5 command)."""
    if not serial_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to Arduino")

    cmd = build_reset_command()
    response = await serial_service.send_command(cmd)
    serial_service.current_direction = "stop"
    serial_service.current_speed = 0
    serial_service.current_mode = "idle"
    return CommandResponse(
        success=True,
        message="All functions reset",
        arduino_response=str(response) if response else None,
    )


# ── Sensor Data ──────────────────────────────────────────────

@app.get("/api/distance", response_model=DistanceResponse, tags=["Sensors"])
async def get_distance():
    """Read the ultrasonic distance sensor (cm)."""
    if not serial_service.is_connected:
        raise HTTPException(status_code=503, detail="Not connected to Arduino")

    distance = await serial_service.get_distance()
    return DistanceResponse(
        distance_cm=distance,
        has_obstacle=distance < 35,
    )


# ── Traffic Light ────────────────────────────────────────────

@app.get("/api/traffic-light", tags=["Traffic Light"])
async def get_traffic_light():
    """Get current traffic light state."""
    return {
        "state": serial_service.traffic_light,
        "gpio_state": gpio_service.traffic_light,
    }


@app.post("/api/traffic-light", response_model=CommandResponse, tags=["Traffic Light"])
async def set_traffic_light(command: TrafficLightCommand):
    """
    Set traffic light state.

    - "green": Allow car movement, green LED ON, red LED OFF
    - "red": Force stop, red LED ON, green LED OFF
    """
    # Update GPIO LEDs
    gpio_service.set_traffic_light(command.state)

    # Send command to Arduino
    if serial_service.is_connected:
        await serial_service.set_traffic_light(command.state)

    return CommandResponse(
        success=True,
        message=f"Traffic light set to {command.state}",
    )


@app.post("/api/traffic-light/toggle", response_model=CommandResponse, tags=["Traffic Light"])
async def toggle_traffic_light():
    """Toggle traffic light between green and red."""
    new_state = "red" if serial_service.traffic_light == "green" else "green"

    gpio_service.set_traffic_light(new_state)
    if serial_service.is_connected:
        await serial_service.set_traffic_light(new_state)

    return CommandResponse(
        success=True,
        message=f"Traffic light toggled to {new_state}",
    )


# ── Buzzer ───────────────────────────────────────────────────

@app.post("/api/buzzer", response_model=CommandResponse, tags=["Buzzer"])
async def set_buzzer_api(on: bool):
    """Turn the physical buzzer on or off."""
    gpio_service.set_buzzer(on)
    serial_service.buzzer = on
    await serial_service._broadcast_telemetry()
    return CommandResponse(
        success=True,
        message=f"Buzzer set to {'ON' if on else 'OFF'}",
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

    serial_service.add_listener(ws_listener)

    # Send initial status
    await websocket.send_json({
        "type": "connection_status",
        "data": serial_service.get_connection_status(),
    })
    await websocket.send_json({
        "type": "telemetry",
        "data": serial_service.get_telemetry(),
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
                logger.info(f"📥 Control command: direction={direction}, speed={speed}, serial_connected={serial_service.is_connected}")
                if serial_service.is_connected:
                    await serial_service.move(direction, speed)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Moving {direction} at {speed}"},
                    })
                else:
                    logger.warning("⚠️  Cannot move — Serial NOT connected to Arduino!")
                    await websocket.send_json({
                        "type": "error",
                        "data": {"message": "Not connected to Arduino. Use connect button or check USB cable."},
                    })

            elif msg_type == "speed":
                speed = msg.get("speed", 200)
                if serial_service.is_connected:
                    await serial_service.set_speed(speed)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Speed set to {speed}"},
                    })

            elif msg_type == "servo":
                angle = msg.get("angle", 90)
                if serial_service.is_connected:
                    cmd = build_servo_command(angle)
                    await serial_service.send_command(cmd)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Servo set to {angle}°"},
                    })

            elif msg_type == "mode":
                mode = msg.get("mode", "line_tracking")
                if serial_service.is_connected:
                    cmd = build_mode_command(mode)
                    serial_service.current_mode = mode
                    await serial_service.send_command(cmd)
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": f"Mode: {mode}"},
                    })

            elif msg_type == "reset":
                if serial_service.is_connected:
                    cmd = build_reset_command()
                    await serial_service.send_command(cmd)
                    serial_service.current_direction = "stop"
                    serial_service.current_speed = 0
                    serial_service.current_mode = "idle"
                    await websocket.send_json({
                        "type": "command_response",
                        "data": {"success": True, "message": "Reset complete"},
                    })

            elif msg_type == "connect":
                port = msg.get("serial_port")
                result = await serial_service.connect(port)
                await websocket.send_json({
                    "type": "connection_status",
                    "data": serial_service.get_connection_status(),
                })

            elif msg_type == "disconnect":
                await serial_service.disconnect()
                await websocket.send_json({
                    "type": "connection_status",
                    "data": serial_service.get_connection_status(),
                })

            elif msg_type == "ping":
                await websocket.send_json({"type": "pong", "data": {}})

            elif msg_type == "traffic_light":
                state = msg.get("state", "green")
                if state in ("green", "red"):
                    gpio_service.set_traffic_light(state)
                    if serial_service.is_connected:
                        await serial_service.set_traffic_light(state)
                    await websocket.send_json({
                        "type": "traffic_light_update",
                        "data": {"state": state},
                    })

            elif msg_type == "buzzer":
                on = msg.get("on", False)
                gpio_service.set_buzzer(on)
                serial_service.buzzer = on
                await serial_service._broadcast_telemetry()
                await websocket.send_json({
                    "type": "command_response",
                    "data": {"success": True, "message": f"Buzzer {'ON' if on else 'OFF'}"},
                })

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
        serial_service.remove_listener(ws_listener)


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
