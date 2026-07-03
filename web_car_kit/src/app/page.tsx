"use client";

import { useState } from "react";
import {
  Sidebar,
  DashboardHeader,
  SystemLog,
  CameraFeed,
  ControlPanel,
  TelemetryPanel,
  QuickActions,
  RecordedVideos,
} from "@/components/dashboard";
import {
  systemLogs as initialLogs,
  recordedVideos,
} from "@/lib/mock-data";
import { useRobotWebSocket } from "@/lib/use-robot-websocket";
import type { RobotMode } from "@/types";

export default function DashboardPage() {
  const [activeMode, setActiveMode] = useState<RobotMode>("manual");
  const [logs, setLogs] = useState(initialLogs);

  // Real-time WebSocket connection to backend
  const {
    telemetry,
    bleConnected,
    wsStatus,
    sendControl,
    sendSpeed,
    sendMode,
    sendReset,
    connectBLE,
    disconnectBLE,
  } = useRobotWebSocket();

  // Derive connection status from both WS and BLE states
  const connectionStatus = !bleConnected
    ? wsStatus === "connected"
      ? "disconnected" // WS connected but BLE not → robot disconnected
      : "connecting" // WS not connected → still connecting to backend
    : "connected"; // Both connected

  const robotStatus = {
    isOnline: bleConnected,
    mode: activeMode,
    speed: telemetry.speed,
    battery: telemetry.battery,
    ipAddress: "BLE",
  };

  const handleClearLogs = () => setLogs([]);

  const handleDisconnect = () => {
    disconnectBLE();
  };

  const handleConnect = () => {
    connectBLE();
  };

  const handleModeChange = (mode: RobotMode) => {
    setActiveMode(mode);
    // Send mode change to robot if it's an autonomous mode
    if (mode === "line-follow") {
      sendMode("line_tracking");
    } else if (mode === "obstacle-avoid") {
      sendMode("obstacle_avoidance");
    } else if (mode === "manual") {
      sendReset(); // Return to manual/bluetooth control
    }
  };

  // Map speed percentage (0-100) to PWM (0-255)
  const handleSpeedChange = (percent: number) => {
    const pwm = Math.round((percent / 100) * 255);
    sendSpeed(pwm);
  };

  return (
    <div className="flex min-h-screen bg-background">
      {/* ── Sidebar ─────────────────────────── */}
      <Sidebar
        status={{ ...robotStatus, mode: activeMode }}
        connectionStatus={connectionStatus}
      />

      {/* ── Main Content ────────────────────── */}
      <main
        className="ml-[320px] flex-1 p-5"
        id="main-content"
        role="main"
      >
        <div className="mx-auto max-w-[1400px] space-y-5">
          {/* Header */}
          <DashboardHeader
            connectionStatus={connectionStatus}
            battery={telemetry.battery}
            onDisconnect={bleConnected ? handleDisconnect : handleConnect}
            isConnected={bleConnected}
          />

          {/* ── Two-column content area ──────── */}
          <div className="grid grid-cols-1 gap-5 xl:grid-cols-[1fr_520px]">
            {/* Left column: Camera + Control Panel + System Log */}
            <div className="space-y-5">
              <CameraFeed />
              <ControlPanel
                activeMode={activeMode}
                onModeChange={handleModeChange}
                onDirectionPress={(dir) => sendControl(dir, Math.round((telemetry.speed / 100) * 255) || 200)}
                onDirectionRelease={() => sendControl("stop", 0)}
                onSpeedChange={handleSpeedChange}
                currentSpeed={telemetry.speed}
              />
              <SystemLog logs={logs} onClear={handleClearLogs} />
            </div>

            {/* Right column: Quick Actions + Telemetry + Recorded Videos */}
            <div className="space-y-5">
              <QuickActions />
              <TelemetryPanel data={telemetry} />
              <RecordedVideos videos={recordedVideos} />
            </div>
          </div>
        </div>
      </main>
    </div>
  );
}
