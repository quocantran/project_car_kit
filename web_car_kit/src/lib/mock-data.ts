import type {
  RobotStatus,
  TelemetryData,
  SystemLogEntry,
  RecordedVideo,
} from "@/types";

/* ── Robot Status ─────────────────────────────── */
export const robotStatus: RobotStatus = {
  isOnline: true,
  mode: "manual",
  speed: 0,
  battery: 85,
  ipAddress: "192.168.4.1",
};

/* ── Telemetry ────────────────────────────────── */
export const telemetry: TelemetryData = {
  battery: 85,
  speed: 0,
  temperature: 25,
  distance: 12.4,
  runtime: "00:15:32",
  voltage: 7.6,
};

/* ── System Log ───────────────────────────────── */
export const systemLogs: SystemLogEntry[] = [
  {
    id: "log-1",
    timestamp: "14:30:15",
    message: "Connected to robot car",
    type: "success",
  },
  {
    id: "log-2",
    timestamp: "14:30:12",
    message: "WiFi signal strength: Excellent",
    type: "info",
  },
  {
    id: "log-3",
    timestamp: "14:29:58",
    message: "Camera stream initialized",
    type: "info",
  },
  {
    id: "log-4",
    timestamp: "14:29:45",
    message: "Battery level: 85%",
    type: "info",
  },
  {
    id: "log-5",
    timestamp: "14:28:30",
    message: "Firmware version: 2.1.0",
    type: "info",
  },
  {
    id: "log-6",
    timestamp: "14:28:15",
    message: "Motor calibration complete",
    type: "success",
  },
];

/* ── Recorded Videos ──────────────────────────── */
export const recordedVideos: RecordedVideo[] = [
  {
    id: "vid-1",
    filename: "VID_20250526_143015.mp4",
    date: "May 26, 2025 14:30",
    duration: "02:15",
    fileSize: "45.2 MB",
    thumbnailUrl: "/thumbnails/vid-1.jpg",
  },
  {
    id: "vid-2",
    filename: "VID_20250526_141523.mp4",
    date: "May 26, 2025 14:15",
    duration: "01:48",
    fileSize: "38.7 MB",
    thumbnailUrl: "/thumbnails/vid-2.jpg",
  },
  {
    id: "vid-3",
    filename: "VID_20250526_140601.mp4",
    date: "May 26, 2025 14:05",
    duration: "03:02",
    fileSize: "62.1 MB",
    thumbnailUrl: "/thumbnails/vid-3.jpg",
  },
];
