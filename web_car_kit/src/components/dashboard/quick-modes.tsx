"use client";

import { motion } from "framer-motion";
import {
  Hand,
  Route,
  ShieldAlert,
  Radar,
} from "lucide-react";
import { cn } from "@/lib/utils";
import type { RobotMode } from "@/types";

interface QuickModesProps {
  activeMode: RobotMode;
  onModeChange: (mode: RobotMode) => void;
}

const modes: { id: RobotMode; label: string; icon: typeof Hand }[] = [
  { id: "manual", label: "Manual", icon: Hand },
  { id: "line-follow", label: "Line Follow", icon: Route },
  { id: "obstacle-avoid", label: "Obstacle Avoid", icon: ShieldAlert },
  { id: "auto-patrol", label: "Auto Patrol", icon: Radar },
];

export function QuickModes({ activeMode, onModeChange }: QuickModesProps) {
  return (
    <motion.div
      initial={{ opacity: 0, x: -12 }}
      animate={{ opacity: 1, x: 0 }}
      transition={{ duration: 0.4, delay: 0.3 }}
      className="rounded-2xl border border-border/50 bg-card p-4"
    >
      <h3 className="mb-3 text-xs font-semibold uppercase tracking-wider text-muted-foreground">
        Quick Modes
      </h3>
      <div className="grid grid-cols-2 gap-2">
        {modes.map((mode) => {
          const isActive = activeMode === mode.id;
          return (
            <button
              key={mode.id}
              onClick={() => onModeChange(mode.id)}
              className={cn(
                "group flex flex-col items-center gap-1.5 rounded-xl border px-3 py-3 text-center transition-all",
                isActive
                  ? "border-primary/30 bg-primary/[0.06] text-primary shadow-sm"
                  : "border-border/40 bg-muted/20 text-muted-foreground hover:border-border hover:bg-muted/40 hover:text-foreground"
              )}
              aria-pressed={isActive}
              aria-label={`Switch to ${mode.label} mode`}
            >
              <mode.icon
                className={cn(
                  "h-4 w-4 transition-transform group-hover:scale-110",
                  isActive && "text-primary"
                )}
              />
              <span className="text-[11px] font-medium leading-tight">
                {mode.label}
              </span>
            </button>
          );
        })}
      </div>
    </motion.div>
  );
}
