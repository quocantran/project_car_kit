"use client";

import { motion } from "framer-motion";
import {
  Activity,
  AlertTriangle,
  CheckCircle2,
  Info,
  Trash2,
} from "lucide-react";
import { Button } from "@/components/ui/button";
import { DashboardCard } from "./dashboard-card";
import type { SystemLogEntry } from "@/types";

interface SystemLogProps {
  logs: SystemLogEntry[];
  onClear: () => void;
}

const iconMap = {
  info: Info,
  warning: AlertTriangle,
  error: AlertTriangle,
  success: CheckCircle2,
};

const colorMap = {
  info: "text-info",
  warning: "text-warning",
  error: "text-danger",
  success: "text-success",
};

export function SystemLog({ logs, onClear }: SystemLogProps) {
  return (
    <DashboardCard
      title="System Log"
      headerAction={
        logs.length > 0 ? (
          <Button
            variant="ghost"
            size="xs"
            onClick={onClear}
            className="gap-1.5 text-danger hover:text-danger hover:bg-danger/5"
            aria-label="Clear log"
          >
            <Trash2 className="h-3 w-3" />
            Clear
          </Button>
        ) : undefined
      }
    >
      <div
        className="max-h-[280px] space-y-1 overflow-y-auto"
        role="log"
        aria-label="System activity log"
      >
        {logs.length === 0 ? (
          <div className="flex flex-col items-center gap-2 py-6 text-center">
            <Activity className="h-5 w-5 text-muted-foreground/30" />
            <p className="text-xs text-muted-foreground/60">
              No activity recorded
            </p>
          </div>
        ) : (
          logs.map((log, index) => {
            const Icon = iconMap[log.type];
            return (
              <motion.div
                key={log.id}
                initial={{ opacity: 0, x: -8 }}
                animate={{ opacity: 1, x: 0 }}
                transition={{ delay: index * 0.03 }}
                className="flex items-start gap-2.5 rounded-lg px-2.5 py-2 transition-colors hover:bg-muted/30"
              >
                <Icon
                  className={`mt-0.5 h-3.5 w-3.5 shrink-0 ${colorMap[log.type]}`}
                />
                <div className="min-w-0 flex-1">
                  <p className="truncate text-xs font-medium text-foreground">
                    {log.message}
                  </p>
                  <p className="mt-0.5 text-[10px] text-muted-foreground">
                    {log.timestamp}
                  </p>
                </div>
              </motion.div>
            );
          })
        )}
      </div>
    </DashboardCard>
  );
}
