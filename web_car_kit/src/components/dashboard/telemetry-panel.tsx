"use client";

import { motion } from "framer-motion";
import { MapPin, Timer, Zap } from "lucide-react";
import { CircularGauge } from "./circular-gauge";
import { DashboardCard } from "./dashboard-card";
import type { TelemetryData } from "@/types";

interface TelemetryPanelProps {
  data: TelemetryData;
}

export function TelemetryPanel({ data }: TelemetryPanelProps) {
  return (
    <DashboardCard title="Telemetry">
      <div className="space-y-5">
        {/* Circular gauges */}
        <div className="flex items-center justify-around">
          <CircularGauge
            value={data.battery}
            max={100}
            label="Battery"
            unit="%"
            size={120}
            color="oklch(0.723 0.219 149.579)"
          />
          <CircularGauge
            value={data.speed}
            max={100}
            label="Speed"
            unit=" cm/s"
            size={120}
            color="oklch(0.546 0.245 262.881)"
          />
          <CircularGauge
            value={data.temperature}
            max={80}
            label="Temperature"
            unit="°C"
            size={120}
            color="oklch(0.734 0.165 66.934)"
          />
        </div>

        {/* Additional metrics */}
        <div className="grid grid-cols-3 gap-3">
          <motion.div
            initial={{ opacity: 0, y: 8 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.2 }}
            className="flex flex-col items-center gap-1.5 rounded-xl border border-border/40 bg-muted/20 px-3 py-3"
          >
            <MapPin className="h-4 w-4 text-muted-foreground" />
            <span className="text-lg font-bold text-foreground">
              {data.distance}
              <span className="text-xs font-normal text-muted-foreground">
                {" "}
                m
              </span>
            </span>
            <span className="text-[10px] font-medium text-muted-foreground">
              Distance
            </span>
          </motion.div>

          <motion.div
            initial={{ opacity: 0, y: 8 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.3 }}
            className="flex flex-col items-center gap-1.5 rounded-xl border border-border/40 bg-muted/20 px-3 py-3"
          >
            <Timer className="h-4 w-4 text-muted-foreground" />
            <span className="text-lg font-bold text-foreground">
              {data.runtime}
            </span>
            <span className="text-[10px] font-medium text-muted-foreground">
              Runtime
            </span>
          </motion.div>

          <motion.div
            initial={{ opacity: 0, y: 8 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.4 }}
            className="flex flex-col items-center gap-1.5 rounded-xl border border-border/40 bg-muted/20 px-3 py-3"
          >
            <Zap className="h-4 w-4 text-muted-foreground" />
            <span className="text-lg font-bold text-foreground">
              {data.voltage}
              <span className="text-xs font-normal text-muted-foreground">
                {" "}
                V
              </span>
            </span>
            <span className="text-[10px] font-medium text-muted-foreground">
              Voltage
            </span>
          </motion.div>
        </div>
      </div>
    </DashboardCard>
  );
}
