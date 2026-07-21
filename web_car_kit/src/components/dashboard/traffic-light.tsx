"use client";

import { motion } from "framer-motion";
import { Play, Square } from "lucide-react";
import { cn } from "@/lib/utils";
import { DashboardCard } from "./dashboard-card";

interface TrafficLightProps {
  currentLight: "green" | "red";
  onChange: (state: "green" | "red") => void;
}

export function TrafficLight({ currentLight, onChange }: TrafficLightProps) {
  const isRed = currentLight === "red";
  const isGreen = currentLight === "green";

  const toggleLight = () => {
    onChange(isRed ? "green" : "red");
  };

  return (
    <DashboardCard title="Đèn Giao Thông 🚦">
      <div className="flex flex-col gap-5 sm:flex-row sm:items-center sm:justify-between">
        {/* Left Side: Interactive Traffic Light Visual */}
        <div className="flex justify-center">
          <div className="flex h-36 w-16 flex-col items-center justify-between rounded-3xl border border-neutral-700 bg-neutral-900 p-2.5 shadow-xl">
            {/* Red Light */}
            <button
              onClick={() => onChange("red")}
              className={cn(
                "relative h-11 w-11 rounded-full transition-all duration-300 focus:outline-none focus:ring-2 focus:ring-red-500/20",
                isRed
                  ? "bg-red-500 shadow-[0_0_20px_rgba(239,68,68,0.7),_inset_0_2px_4px_rgba(255,255,255,0.4)]"
                  : "bg-red-950/40 hover:bg-red-900/20"
              )}
              aria-label="Set red light"
            >
              {isRed && (
                <motion.div
                  layoutId="activeGlow"
                  className="absolute inset-0 rounded-full bg-red-500 opacity-25 blur-md animate-pulse"
                />
              )}
            </button>

            {/* Green Light */}
            <button
              onClick={() => onChange("green")}
              className={cn(
                "relative h-11 w-11 rounded-full transition-all duration-300 focus:outline-none focus:ring-2 focus:ring-green-500/20",
                isGreen
                  ? "bg-green-500 shadow-[0_0_20px_rgba(34,197,94,0.7),_inset_0_2px_4px_rgba(255,255,255,0.4)]"
                  : "bg-green-950/40 hover:bg-green-900/20"
              )}
              aria-label="Set green light"
            >
              {isGreen && (
                <motion.div
                  layoutId="activeGlow2"
                  className="absolute inset-0 rounded-full bg-green-500 opacity-25 blur-md animate-pulse"
                />
              )}
            </button>
          </div>
        </div>

        {/* Right Side: Status Display & Control Action */}
        <div className="flex flex-1 flex-col justify-center gap-3 text-center sm:text-left">
          <div className="space-y-1">
            <span className="text-[10px] font-bold uppercase tracking-widest text-muted-foreground">
              Trạng Thái Hệ Thống
            </span>
            <div className="flex items-center justify-center gap-2 sm:justify-start">
              {isRed ? (
                <>
                  <div className="h-2.5 w-2.5 rounded-full bg-red-500 animate-ping" />
                  <h4 className="text-sm font-bold text-red-500">DỪNG LẠI (RED)</h4>
                </>
              ) : (
                <>
                  <div className="h-2.5 w-2.5 rounded-full bg-green-500 animate-ping" />
                  <h4 className="text-sm font-bold text-green-500">DI CHUYỂN (GREEN)</h4>
                </>
              )}
            </div>
            <p className="text-[11px] text-muted-foreground leading-relaxed">
              {isRed
                ? "Xe đang bị dừng. Mọi chế độ di chuyển bị vô hiệu hóa."
                : "Xe được phép di chuyển và vận hành bình thường."}
            </p>
          </div>

          <button
            onClick={toggleLight}
            className={cn(
              "group flex items-center justify-center gap-2 rounded-xl border px-3 py-2 text-[11px] font-semibold shadow-sm transition-all duration-200 active:scale-95",
              isRed
                ? "border-green-500/30 bg-green-500/10 text-green-500 hover:bg-green-500/15"
                : "border-red-500/30 bg-red-500/10 text-red-500 hover:bg-red-500/15"
            )}
          >
            {isRed ? (
              <>
                <Play className="h-3.5 w-3.5 transition-transform group-hover:translate-x-0.5" />
                <span>Cho Phép Chạy (Đèn Xanh)</span>
              </>
            ) : (
              <>
                <Square className="h-3 w-3" />
                <span>Yêu Cầu Dừng (Đèn Đỏ)</span>
              </>
            )}
          </button>
        </div>
      </div>
    </DashboardCard>
  );
}
