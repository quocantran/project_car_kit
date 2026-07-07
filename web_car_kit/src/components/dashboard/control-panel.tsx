"use client";

import React, { useState, useEffect } from "react";
import {
  ChevronUp,
  ChevronDown,
  ChevronLeft,
  ChevronRight,
  Hand,
  Route,
  ShieldAlert,
  Radar,
  ScanFace,
} from "lucide-react";
import { cn } from "@/lib/utils";
import { Slider } from "@/components/ui/slider";
import { DashboardCard } from "./dashboard-card";
import type { RobotMode } from "@/types";

interface ControlPanelProps {
  activeMode: RobotMode;
  onModeChange: (mode: RobotMode) => void;
  /** Called when a direction button is pressed (mouse down / touch start) */
  onDirectionPress?: (direction: string) => void;
  /** Called when a direction button is released (mouse up / touch end) */
  onDirectionRelease?: () => void;
  /** Called when the speed slider changes (0-100 percent) */
  onSpeedChange?: (percent: number) => void;
  /** Current speed percentage from telemetry */
  currentSpeed?: number;
}

const robotModes: { id: RobotMode; label: string; icon: typeof Hand }[] = [
  { id: "manual", label: "Manual", icon: Hand },
  { id: "line-follow", label: "Line Follow", icon: Route },
  { id: "obstacle-avoid", label: "Obstacle Avoid", icon: ShieldAlert },
  { id: "auto-patrol", label: "Auto Patrol", icon: Radar },
  { id: "hand-tracking", label: "Hand Tracking", icon: ScanFace },
];

export function ControlPanel({
  activeMode,
  onModeChange,
  onDirectionPress,
  onDirectionRelease,
  onSpeedChange,
  currentSpeed = 50,
}: ControlPanelProps) {
  const [speed, setSpeed] = useState([currentSpeed]);
  const [trim, setTrim] = useState([0]);
  const [activeDirection, setActiveDirection] = useState<string | null>(null);

  const handleDirectionPress = (direction: string) => {
    setActiveDirection(direction);
    onDirectionPress?.(direction);
  };

  const handleDirectionRelease = () => {
    setActiveDirection(null);
    onDirectionRelease?.();
  };

  const handleSpeedChange = (val: number | readonly number[]) => {
    const arrayVal = Array.isArray(val) ? [...val] : [val];
    setSpeed(arrayVal);
    onSpeedChange?.(arrayVal[0]);
  };

  // Use refs for callbacks to avoid re-running useEffect on every render
  const pressRef = React.useRef(onDirectionPress);
  const releaseRef = React.useRef(onDirectionRelease);

  useEffect(() => {
    pressRef.current = onDirectionPress;
    releaseRef.current = onDirectionRelease;
  }, [onDirectionPress, onDirectionRelease]);

  useEffect(() => {
    if (activeMode !== "manual") return;

    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.repeat) return;
      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) return;

      switch (e.key) {
        case "ArrowUp":
        case "w":
        case "W":
          setActiveDirection("forward");
          pressRef.current?.("forward");
          break;
        case "ArrowDown":
        case "s":
        case "S":
          setActiveDirection("back");
          pressRef.current?.("back");
          break;
        case "ArrowLeft":
        case "a":
        case "A":
          setActiveDirection("left");
          pressRef.current?.("left");
          break;
        case "ArrowRight":
        case "d":
        case "D":
          setActiveDirection("right");
          pressRef.current?.("right");
          break;
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) return;

      switch (e.key) {
        case "ArrowUp":
        case "w":
        case "W":
        case "ArrowDown":
        case "s":
        case "S":
        case "ArrowLeft":
        case "a":
        case "A":
        case "ArrowRight":
        case "d":
        case "D":
          setActiveDirection(null);
          releaseRef.current?.();
          break;
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    window.addEventListener("keyup", handleKeyUp);

    return () => {
      window.removeEventListener("keydown", handleKeyDown);
      window.removeEventListener("keyup", handleKeyUp);
    };
  }, [activeMode]);

  return (
    <DashboardCard title="Control Panel">
      <div className="space-y-6">
        {/* D-Pad & Sliders Side-by-Side */}
        <div className="grid grid-cols-1 md:grid-cols-[auto_1fr] gap-8 items-center">
          {/* D-Pad (Left side) */}
          <div className="flex justify-center shrink-0">
            <div className="relative grid h-[150px] w-[150px] grid-cols-3 grid-rows-3 gap-1.5">
              {/* Forward */}
              <div className="col-start-2 row-start-1 flex items-end justify-center">
                <button
                  className={cn(
                    "dpad-btn flex h-11 w-11 items-center justify-center rounded-xl border border-border/50 bg-muted/30 text-muted-foreground shadow-sm transition-all hover:bg-primary/10 hover:text-primary hover:border-primary/30",
                    activeDirection === "forward" &&
                      "bg-primary/15 text-primary border-primary/40 shadow-md"
                  )}
                  onMouseDown={() => handleDirectionPress("forward")}
                  onMouseUp={handleDirectionRelease}
                  onMouseLeave={handleDirectionRelease}
                  onTouchStart={(e) => { e.preventDefault(); handleDirectionPress("forward"); }}
                  onTouchEnd={(e) => { e.preventDefault(); handleDirectionRelease(); }}
                  aria-label="Move forward"
                >
                  <ChevronUp className="h-5 w-5" strokeWidth={2.5} />
                </button>
              </div>

              {/* Left */}
              <div className="col-start-1 row-start-2 flex items-center justify-end">
                <button
                  className={cn(
                    "dpad-btn flex h-11 w-11 items-center justify-center rounded-xl border border-border/50 bg-muted/30 text-muted-foreground shadow-sm transition-all hover:bg-primary/10 hover:text-primary hover:border-primary/30",
                    activeDirection === "left" &&
                      "bg-primary/15 text-primary border-primary/40 shadow-md"
                  )}
                  onMouseDown={() => handleDirectionPress("left")}
                  onMouseUp={handleDirectionRelease}
                  onMouseLeave={handleDirectionRelease}
                  onTouchStart={(e) => { e.preventDefault(); handleDirectionPress("left"); }}
                  onTouchEnd={(e) => { e.preventDefault(); handleDirectionRelease(); }}
                  aria-label="Turn left"
                >
                  <ChevronLeft className="h-5 w-5" strokeWidth={2.5} />
                </button>
              </div>

              {/* Center indicator */}
              <div className="col-start-2 row-start-2 flex items-center justify-center">
                <div className="flex h-7 w-7 items-center justify-center rounded-lg bg-muted/20">
                  <div className="h-2 w-2 rounded-full bg-primary/30" />
                </div>
              </div>

              {/* Right */}
              <div className="col-start-3 row-start-2 flex items-center justify-start">
                <button
                  className={cn(
                    "dpad-btn flex h-11 w-11 items-center justify-center rounded-xl border border-border/50 bg-muted/30 text-muted-foreground shadow-sm transition-all hover:bg-primary/10 hover:text-primary hover:border-primary/30",
                    activeDirection === "right" &&
                      "bg-primary/15 text-primary border-primary/40 shadow-md"
                  )}
                  onMouseDown={() => handleDirectionPress("right")}
                  onMouseUp={handleDirectionRelease}
                  onMouseLeave={handleDirectionRelease}
                  onTouchStart={(e) => { e.preventDefault(); handleDirectionPress("right"); }}
                  onTouchEnd={(e) => { e.preventDefault(); handleDirectionRelease(); }}
                  aria-label="Turn right"
                >
                  <ChevronRight className="h-5 w-5" strokeWidth={2.5} />
                </button>
              </div>

              {/* Backward */}
              <div className="col-start-2 row-start-3 flex items-start justify-center">
                <button
                  className={cn(
                    "dpad-btn flex h-11 w-11 items-center justify-center rounded-xl border border-border/50 bg-muted/30 text-muted-foreground shadow-sm transition-all hover:bg-primary/10 hover:text-primary hover:border-primary/30",
                    activeDirection === "back" &&
                      "bg-primary/15 text-primary border-primary/40 shadow-md"
                  )}
                  onMouseDown={() => handleDirectionPress("back")}
                  onMouseUp={handleDirectionRelease}
                  onMouseLeave={handleDirectionRelease}
                  onTouchStart={(e) => { e.preventDefault(); handleDirectionPress("back"); }}
                  onTouchEnd={(e) => { e.preventDefault(); handleDirectionRelease(); }}
                  aria-label="Move backward"
                >
                  <ChevronDown className="h-5 w-5" strokeWidth={2.5} />
                </button>
              </div>
            </div>
          </div>

          {/* Sliders (Right side) */}
          <div className="space-y-5 w-full">
            {/* Speed slider */}
            <div className="space-y-2">
              <div className="flex items-center justify-between">
                <label
                  htmlFor="speed-slider"
                  className="text-xs font-semibold text-muted-foreground"
                >
                  Speed
                </label>
                <span className="text-xs font-bold text-foreground">
                  {speed[0]}%
                </span>
              </div>
              <Slider
                id="speed-slider"
                value={speed}
                onValueChange={handleSpeedChange}
                min={0}
                max={100}
                aria-label="Speed control"
              />
            </div>

            {/* Steering Trim */}
            <div className="space-y-2">
              <div className="flex items-center justify-between">
                <label
                  htmlFor="trim-slider"
                  className="text-xs font-semibold text-muted-foreground"
                >
                  Steering Trim
                </label>
                <span className="text-xs font-bold text-foreground">
                  {trim[0]}
                </span>
              </div>
              <Slider
                id="trim-slider"
                value={trim}
                onValueChange={(val) => setTrim(Array.isArray(val) ? [...val] : [val])}
                min={-50}
                max={50}
                aria-label="Steering trim"
              />
            </div>
          </div>
        </div>

        {/* Robot Modes Horizontal Tabs (Bottom) */}
        <div className="grid grid-cols-4 gap-2 pt-2 border-t border-border/50">
          {robotModes.map((mode) => {
            const isActive = activeMode === mode.id;
            return (
              <button
                key={mode.id}
                onClick={() => onModeChange(mode.id)}
                className={cn(
                  "flex items-center justify-center gap-2 rounded-xl border px-2 py-2.5 text-center transition-all",
                  isActive
                    ? "border-primary/30 bg-primary/[0.06] text-primary shadow-sm"
                    : "border-border/40 bg-muted/15 text-muted-foreground hover:border-border hover:bg-muted/30 hover:text-foreground"
                )}
                aria-pressed={isActive}
                aria-label={`${mode.label} mode`}
              >
                <mode.icon className="h-4 w-4 shrink-0" />
                <span className="text-xs font-semibold truncate">{mode.label}</span>
              </button>
            );
          })}
        </div>
      </div>
    </DashboardCard>
  );
}
