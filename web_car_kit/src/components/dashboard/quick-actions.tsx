"use client";

import { useState } from "react";
import { motion } from "framer-motion";
import {
  Lightbulb,
  Sparkles,
  Volume2,
  OctagonX,
} from "lucide-react";
import { cn } from "@/lib/utils";
import { DashboardCard } from "./dashboard-card";

interface ActionItem {
  id: string;
  label: string;
  icon: typeof Lightbulb;
  activeColor: string;
  activeBg: string;
  activeBorder: string;
}

const actions: ActionItem[] = [
  {
    id: "headlight",
    label: "Headlight",
    icon: Lightbulb,
    activeColor: "text-amber-500",
    activeBg: "bg-amber-500/10",
    activeBorder: "border-amber-500/30",
  },
  {
    id: "led",
    label: "LED",
    icon: Sparkles,
    activeColor: "text-violet-500",
    activeBg: "bg-violet-500/10",
    activeBorder: "border-violet-500/30",
  },
  {
    id: "buzzer",
    label: "Buzzer",
    icon: Volume2,
    activeColor: "text-blue-500",
    activeBg: "bg-blue-500/10",
    activeBorder: "border-blue-500/30",
  },
  {
    id: "stop",
    label: "E-Stop",
    icon: OctagonX,
    activeColor: "text-red-500",
    activeBg: "bg-red-500/10",
    activeBorder: "border-red-500/30",
  },
];

export function QuickActions() {
  const [activeActions, setActiveActions] = useState<Record<string, boolean>>(
    {}
  );

  const toggleAction = (id: string) => {
    setActiveActions((prev) => ({ ...prev, [id]: !prev[id] }));
  };

  return (
    <DashboardCard title="Quick Actions">
      <div className="grid grid-cols-4 gap-2.5">
        {actions.map((action, index) => {
          const isActive = activeActions[action.id] ?? false;
          return (
            <motion.button
              key={action.id}
              initial={{ opacity: 0, scale: 0.9 }}
              animate={{ opacity: 1, scale: 1 }}
              transition={{ delay: index * 0.05 }}
              onClick={() => toggleAction(action.id)}
              className={cn(
                "group flex flex-col items-center gap-2 rounded-xl border px-3 py-4 transition-all",
                isActive
                  ? `${action.activeBg} ${action.activeBorder} ${action.activeColor}`
                  : "border-border/40 bg-muted/15 text-muted-foreground hover:border-border hover:bg-muted/30 hover:text-foreground"
              )}
              aria-pressed={isActive}
              aria-label={`${isActive ? "Disable" : "Enable"} ${action.label}`}
            >
              <action.icon
                className={cn(
                  "h-5 w-5 transition-transform group-hover:scale-110",
                  isActive && action.activeColor
                )}
              />
              <span className="text-[11px] font-medium">{action.label}</span>
            </motion.button>
          );
        })}
      </div>
    </DashboardCard>
  );
}
