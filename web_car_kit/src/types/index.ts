/* ──────────────────────────────────────────────
 * Domain types for the Smart Robot Car dashboard
 * ────────────────────────────────────────────── */

export type ConnectionStatus = "connected" | "disconnected" | "connecting";

export type RobotMode = "manual" | "line-follow" | "obstacle-avoid" | "auto-patrol";

export interface RobotStatus {
  isOnline: boolean;
  mode: RobotMode;
  speed: number;        // cm/s
  battery: number;      // 0-100%
  ipAddress: string;
}

export interface TelemetryData {
  battery: number;       // 0-100%
  speed: number;         // cm/s
  temperature: number;   // °C
  distance: number;      // metres
  runtime: string;       // formatted HH:MM:SS
  voltage: number;       // V
}

export interface SystemLogEntry {
  id: string;
  timestamp: string;
  message: string;
  type: "info" | "warning" | "error" | "success";
}

export interface RecordedVideo {
  id: string;
  filename: string;
  date: string;
  duration: string;
  fileSize: string;
  thumbnailUrl: string;
}

export interface QuickAction {
  id: string;
  label: string;
  icon: string;
  isActive: boolean;
  color: "blue" | "yellow" | "purple" | "red";
}
