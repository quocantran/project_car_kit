"""
JSON protocol encoder/decoder for the Elegoo Smart Robot Car V3.0 Plus.

The Arduino firmware communicates via JSON commands over UART (through HC-02 BLE).
Each command is a JSON object with fields: N (command code), D1, D2, D3, T, H.

Command Reference (from SmartCar_Core_20210127.ino):
  N=1  : Motor Control       — D1=motor(0=both,1=left,2=right), D2=direction(1=fwd,2=rev,0=stop), D3=speed
  N=2  : Bluetooth Joystick  — D1=direction(1-9), D2=speed
  N=3  : Mode Switch         — D1=1(line tracking), D1=2(obstacle avoidance)
  N=4  : Car Control (timer) — D1=direction(1-4), D2=speed, T=seconds
  N=5  : Reset All           — Clear all modes, stop car
  N=6  : Servo Control       — D1=angle(5-175)
  N=21 : Ultrasonic Query    — D1=1(obstacle yes/no), D1=2(distance in cm)
  N=22 : Line Tracking Query — D1=0(left), D1=1(middle), D1=2(right)
  N=40 : Car Control (no timer) — D1=direction(1-4), D2=speed

Response format from Arduino: {H_value} e.g. {1_ok}, {1_35}, {1_true}
"""

import json
import re
from typing import Optional

# ── Direction Mapping (N=2 Bluetooth Joystick mode) ──────────
DIRECTION_MAP: dict[str, int] = {
    "left": 1,
    "right": 2,
    "forward": 3,
    "back": 4,
    "stop": 5,
    "forward_left": 6,
    "back_left": 7,
    "forward_right": 8,
    "back_right": 9,
}

# Reverse mapping for display
DIRECTION_NAMES: dict[int, str] = {v: k for k, v in DIRECTION_MAP.items()}

# ── Command ID Counter ───────────────────────────────────────
_cmd_counter: int = 0


def _next_cmd_id() -> int:
    """Get next command serial number (H field), wraps at 255."""
    global _cmd_counter
    _cmd_counter = (_cmd_counter + 1) % 256
    return _cmd_counter


# ── Command Builders ─────────────────────────────────────────

def build_move_command(direction: str, speed: int = 200) -> str:
    """
    Build N=2 Bluetooth Joystick command.
    Direction: forward, back, left, right, stop, forward_left, etc.
    Speed: 0-255 PWM value.
    """
    d1 = DIRECTION_MAP.get(direction, 5)  # Default to stop
    cmd = {"N": 2, "D1": d1, "D2": speed, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_stop_command() -> str:
    """Build N=2 stop command (D1=5)."""
    return build_move_command("stop", 0)


def build_speed_update_command(direction: str, speed: int) -> str:
    """Build N=2 command to update speed while maintaining direction."""
    return build_move_command(direction, speed)


def build_distance_query() -> str:
    """Build N=21 D1=2 command to read ultrasonic distance in cm."""
    cmd = {"N": 21, "D1": 2, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_obstacle_query() -> str:
    """Build N=21 D1=1 command to check if obstacle detected."""
    cmd = {"N": 21, "D1": 1, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_line_tracking_query(sensor: str = "middle") -> str:
    """
    Build N=22 command to read line tracking sensor.
    sensor: "left" (D1=0), "middle" (D1=1), "right" (D1=2)
    """
    sensor_map = {"left": 0, "middle": 1, "right": 2}
    d1 = sensor_map.get(sensor, 1)
    cmd = {"N": 22, "D1": d1, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_mode_command(mode: str) -> str:
    """
    Build N=3 command to switch autonomous mode.
    mode: "line_tracking" (D1=1), "obstacle_avoidance" (D1=2)
    """
    mode_map = {"line_tracking": 1, "obstacle_avoidance": 2, "hand_tracking": 3}
    d1 = mode_map.get(mode, 1)
    cmd = {"N": 3, "D1": d1, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_servo_command(angle: int) -> str:
    """Build N=6 command to set servo angle (5-175 degrees)."""
    angle = max(5, min(175, angle))
    cmd = {"N": 6, "D1": angle, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_motor_control_command(
    motor: int = 0, direction: int = 1, speed: int = 200
) -> str:
    """
    Build N=1 command for individual motor control.
    motor: 0=both, 1=left, 2=right
    direction: 0=stop, 1=forward, 2=reverse
    speed: 0-255
    """
    cmd = {"N": 1, "D1": motor, "D2": direction, "D3": speed, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_car_control_command(direction: int, speed: int, timer: int = 0) -> str:
    """
    Build N=4 (with timer) or N=40 (without timer) car control command.
    direction: 1=left, 2=right, 3=forward, 4=back
    speed: 0-255
    timer: seconds (0 = no timer → uses N=40)
    """
    if timer > 0:
        cmd = {"N": 4, "D1": direction, "D2": speed, "T": timer, "H": _next_cmd_id()}
    else:
        cmd = {"N": 40, "D1": direction, "D2": speed, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_reset_command() -> str:
    """Build N=5 command to reset/clear all functions."""
    cmd = {"N": 5, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


def build_telemetry_query() -> str:
    """
    Build N=100 command to request full telemetry from Arduino.
    Response: {"d":distance, "s":speed, "dir":direction, "m":mode}
    """
    cmd = {"N": 100, "H": _next_cmd_id()}
    return json.dumps(cmd, separators=(",", ":"))


# ── Response Parser ──────────────────────────────────────────

def parse_response(raw: str) -> dict:
    """
    Parse Arduino response string.

    Response format: {H_value}
    Examples:
      {1_ok}      → {"cmd_id": 1, "status": "ok", "value": None}
      {1_35}      → {"cmd_id": 1, "status": "ok", "value": 35}
      {1_true}    → {"cmd_id": 1, "status": "ok", "value": True}
      {1_false}   → {"cmd_id": 1, "status": "ok", "value": False}
    """
    result = {"cmd_id": None, "status": "unknown", "value": None, "raw": raw}

    # Extract content between { and }
    match = re.search(r"\{(.+?)\}", raw)
    if not match:
        return result

    content = match.group(1)
    parts = content.split("_", 1)

    if len(parts) >= 1:
        try:
            result["cmd_id"] = int(parts[0])
        except ValueError:
            result["cmd_id"] = parts[0]

    if len(parts) >= 2:
        value_str = parts[1]
        if value_str == "ok":
            result["status"] = "ok"
        elif value_str == "true":
            result["status"] = "ok"
            result["value"] = True
        elif value_str == "false":
            result["status"] = "ok"
            result["value"] = False
        else:
            result["status"] = "ok"
            try:
                result["value"] = int(value_str)
            except ValueError:
                try:
                    result["value"] = float(value_str)
                except ValueError:
                    result["value"] = value_str

    return result


# ── Telemetry Response Parser ────────────────────────────────

# Direction code → name mapping (from Arduino CMD_Telemetry_Plus)
TELEMETRY_DIR_MAP: dict[int, str] = {
    0: "stop", 1: "left", 2: "right", 3: "forward", 4: "back",
    6: "forward_left", 7: "back_left", 8: "forward_right", 9: "back_right",
}

# Mode code → name mapping
TELEMETRY_MODE_MAP: dict[int, str] = {
    0: "idle", 1: "line_tracking", 2: "obstacle_avoidance",
    3: "bluetooth", 4: "irremote", 5: "other", 6: "hand_tracking",
}


def parse_telemetry_response(raw: str) -> Optional[dict]:
    """
    Parse the N=100 telemetry JSON response from Arduino.

    Input:  '{"d":35,"s":200,"dir":3,"m":3}'
    Output: {"distance": 35, "speed": 200, "direction": "forward", "mode": "bluetooth"}
    """
    try:
        # Find JSON object in the raw string
        start = raw.find("{")
        end = raw.rfind("}") + 1
        if start == -1 or end == 0:
            return None

        data = json.loads(raw[start:end])

        # Check if it has telemetry fields ("d", "s", "dir", "m")
        if "d" in data and "s" in data:
            return {
                "distance": data.get("d", 0),
                "speed": data.get("s", 0),
                "direction": TELEMETRY_DIR_MAP.get(data.get("dir", 0), "stop"),
                "mode": TELEMETRY_MODE_MAP.get(data.get("m", 0), "idle"),
            }
        return None
    except (json.JSONDecodeError, ValueError):
        return None
