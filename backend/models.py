"""
Pydantic models for the FastAPI Serial Bridge API.
"""

from typing import Literal, Optional
from pydantic import BaseModel, Field


# ── Request Models ───────────────────────────────────────────

class ControlCommand(BaseModel):
    """Command to control car movement direction and speed."""
    direction: Literal[
        "forward", "back", "left", "right", "stop",
        "forward_left", "back_left", "forward_right", "back_right"
    ]
    speed: int = Field(200, ge=0, le=255, description="PWM speed value (0-255)")


class SpeedUpdate(BaseModel):
    """Update car speed without changing direction."""
    speed: int = Field(..., ge=0, le=255, description="PWM speed value (0-255)")


class ServoCommand(BaseModel):
    """Command to set the ultrasonic servo angle."""
    angle: int = Field(..., ge=5, le=175, description="Servo angle in degrees (5-175)")


class ModeCommand(BaseModel):
    """Command to switch autonomous driving mode."""
    mode: Literal["line_tracking", "obstacle_avoidance", "hand_tracking"]


class TrafficLightCommand(BaseModel):
    """Command to set traffic light state (controls LED on breadboard + Arduino)."""
    state: Literal["green", "red"] = Field(
        description='"green" = allow movement, "red" = force stop'
    )


class ConnectRequest(BaseModel):
    """Optional: specify serial port to connect to."""
    serial_port: Optional[str] = Field(
        None, description="Serial port path (e.g. '/dev/ttyACM0'). Auto-detect if None."
    )


# ── Response Models ──────────────────────────────────────────

class ConnectionStatus(BaseModel):
    """Serial connection status."""
    is_connected: bool
    serial_port: Optional[str] = None
    baudrate: Optional[int] = None


class TelemetryData(BaseModel):
    """Real-time telemetry data from the robot."""
    battery: int = Field(description="Battery percentage (estimated or mock)")
    speed: int = Field(description="Current PWM speed value (0-255)")
    speed_percent: int = Field(description="Speed as percentage (0-100%)")
    distance: int = Field(description="Ultrasonic distance in cm")
    is_connected: bool
    current_direction: str = "stop"
    current_mode: str = "idle"
    traffic_light: str = Field("green", description='Traffic light state: "green" or "red"')
    buzzer: bool = Field(False, description="Buzzer state: True (ON) or False (OFF)")


class StatusResponse(BaseModel):
    """Combined status response."""
    connection: ConnectionStatus
    telemetry: TelemetryData


class CommandResponse(BaseModel):
    """Response after sending a command to the robot."""
    success: bool
    message: str
    arduino_response: Optional[str] = None


class DistanceResponse(BaseModel):
    """Ultrasonic distance reading."""
    distance_cm: int
    has_obstacle: bool = Field(description="True if distance < 35cm")


# ── WebSocket Message Models ─────────────────────────────────

class WSIncomingMessage(BaseModel):
    """Message received from frontend via WebSocket."""
    type: Literal["control", "speed", "servo", "mode", "reset", "traffic_light", "buzzer", "ping"]
    direction: Optional[str] = None
    speed: Optional[int] = None
    angle: Optional[int] = None
    mode: Optional[str] = None
    state: Optional[str] = None  # For traffic_light: "green" or "red"
    on: Optional[bool] = None     # For buzzer: true or false


class WSOutgoingMessage(BaseModel):
    """Message sent to frontend via WebSocket."""
    type: Literal["telemetry", "connection_status", "command_response", "traffic_light_update", "buzzer_update", "error", "pong"]
    data: dict
