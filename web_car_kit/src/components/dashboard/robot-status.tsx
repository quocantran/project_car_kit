"use client";

import { motion } from "framer-motion";
import {
  Battery,
  Wifi,
  WifiOff,
  Power,
  Gauge,
  MapPin,
  Activity,
} from "lucide-react";
import { Button } from "@/components/ui/button";
import { Progress } from "@/components/ui/progress";
import { Badge } from "@/components/ui/badge";
import type { RobotStatus, ConnectionStatus } from "@/types";

interface RobotStatusCardProps {
  status: RobotStatus;
  connectionStatus: ConnectionStatus;
}

const modeLabels: Record<string, string> = {
  manual: "Manual",
  "line-follow": "Line Follow",
  "obstacle-avoid": "Obstacle Avoid",
  "auto-patrol": "Auto Patrol",
};

export function RobotStatusCard({
  status,
  connectionStatus,
}: RobotStatusCardProps) {
  const isConnected = connectionStatus === "connected";

  return (
    <motion.div
      initial={{ opacity: 0, x: -12 }}
      animate={{ opacity: 1, x: 0 }}
      transition={{ duration: 0.4, delay: 0.1 }}
      className="rounded-2xl border border-border/50 bg-card p-4"
    >
      <div className="mb-3 flex items-center justify-between">
        <h3 className="text-xs font-semibold uppercase tracking-wider text-muted-foreground">
          Robot Status
        </h3>
        <Badge
          variant={isConnected ? "default" : "secondary"}
          className={
            isConnected
              ? "bg-success/10 text-success hover:bg-success/15 border-0 text-[10px] font-semibold"
              : "text-[10px] font-semibold"
          }
        >
          <span
            className={`mr-1 inline-block h-1.5 w-1.5 rounded-full ${isConnected ? "bg-success" : "bg-muted-foreground"}`}
          />
          {isConnected ? "Online" : "Offline"}
        </Badge>
      </div>

      <div className="space-y-3">
        {/* Mode */}
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2 text-xs text-muted-foreground">
            <Activity className="h-3.5 w-3.5" />
            <span>Mode</span>
          </div>
          <span className="text-xs font-semibold text-foreground">
            {modeLabels[status.mode]}
          </span>
        </div>

        {/* Speed */}
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2 text-xs text-muted-foreground">
            <Gauge className="h-3.5 w-3.5" />
            <span>Speed</span>
          </div>
          <span className="text-xs font-semibold text-foreground">
            {status.speed} cm/s
          </span>
        </div>

        {/* Battery */}
        <div className="space-y-1.5">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2 text-xs text-muted-foreground">
              <Battery className="h-3.5 w-3.5" />
              <span>Battery</span>
            </div>
            <span className="text-xs font-semibold text-foreground">
              {status.battery}%
            </span>
          </div>
          <Progress
            value={status.battery}
            className="h-1.5"
            aria-label={`Battery: ${status.battery}%`}
          />
        </div>

        {/* IP Address */}
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2 text-xs text-muted-foreground">
            <MapPin className="h-3.5 w-3.5" />
            <span>IP Address</span>
          </div>
          <span className="font-mono text-[11px] font-medium text-foreground">
            {status.ipAddress}
          </span>
        </div>
      </div>
    </motion.div>
  );
}

/* ── Header bar ──────────────────────────────── */

interface DashboardHeaderProps {
  connectionStatus: ConnectionStatus;
  battery: number;
  onDisconnect: () => void;
  /** Whether BLE is connected to the robot (controls Connect/Disconnect button) */
  isConnected?: boolean;
}

export function DashboardHeader({
  connectionStatus,
  battery,
  onDisconnect,
  isConnected: bleConnected,
}: DashboardHeaderProps) {
  const isConnected = connectionStatus === "connected";

  return (
    <header className="flex items-center justify-between" aria-label="Dashboard header">
      <div>
        <h2 className="text-xl font-bold tracking-tight text-foreground">
          Dashboard
        </h2>
        <p className="mt-0.5 text-sm text-muted-foreground">
          Real-time control and monitoring of your Smart Robot Car
        </p>
      </div>

      <div className="flex items-center gap-3">
        {/* Connection status */}
        <div className="flex items-center gap-2 rounded-xl border border-border/50 bg-muted/30 px-3 py-1.5">
          {isConnected ? (
            <Wifi className="h-4 w-4 text-success" />
          ) : (
            <WifiOff className="h-4 w-4 text-muted-foreground" />
          )}
          <span className="text-xs font-medium text-foreground">
            {isConnected ? "Connected" : "Disconnected"}
          </span>
        </div>

        {/* Battery */}
        <div className="flex items-center gap-1.5 rounded-xl border border-border/50 bg-muted/30 px-3 py-1.5">
          <Battery
            className={`h-4 w-4 ${battery > 20 ? "text-success" : "text-danger"}`}
          />
          <span className="text-xs font-semibold">{battery}%</span>
        </div>

        {/* Connect / Disconnect */}
        <Button
          variant={bleConnected ? "destructive" : "default"}
          size="sm"
          onClick={onDisconnect}
          className="gap-1.5 rounded-xl"
          aria-label={bleConnected ? "Disconnect from robot" : "Connect to robot"}
        >
          <Power className="h-3.5 w-3.5" />
          {bleConnected ? "Disconnect" : "Connect"}
        </Button>
      </div>
    </header>
  );
}
