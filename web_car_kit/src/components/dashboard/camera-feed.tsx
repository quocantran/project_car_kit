"use client";

import { useState } from "react";
import {
  Video,
  Maximize2,
  Camera,
  Settings2,
} from "lucide-react";
import { Badge } from "@/components/ui/badge";
import { Tooltip, TooltipContent, TooltipTrigger } from "@/components/ui/tooltip";
import { DashboardCard } from "./dashboard-card";

export function CameraFeed() {
  const [isFullscreen, setIsFullscreen] = useState(false);

  return (
    <DashboardCard
      className="overflow-hidden"
      noPadding
    >
      <div className="relative">
        {/* Camera header overlay */}
        <div className="absolute inset-x-0 top-0 z-10 flex items-center justify-between bg-gradient-to-b from-black/50 to-transparent px-4 py-3">
          <div className="flex items-center gap-2">
            <h3 className="text-sm font-semibold text-white">
              Camera Live Feed
            </h3>
            <Badge className="border-0 bg-red-500/90 text-[10px] font-bold text-white hover:bg-red-500">
              <span className="mr-1 inline-block h-1.5 w-1.5 animate-pulse rounded-full bg-white" />
              LIVE
            </Badge>
          </div>

          <div className="flex items-center gap-1">
            <Tooltip>
              <TooltipTrigger
                className="inline-flex h-7 w-7 items-center justify-center rounded-lg text-white/80 transition-colors hover:bg-white/15 hover:text-white"
                aria-label="Camera settings"
              >
                <Settings2 className="h-4 w-4" />
              </TooltipTrigger>
              <TooltipContent>Camera Settings</TooltipContent>
            </Tooltip>

            <Tooltip>
              <TooltipTrigger
                className="inline-flex h-7 w-7 items-center justify-center rounded-lg text-white/80 transition-colors hover:bg-white/15 hover:text-white"
                aria-label="Take screenshot"
              >
                <Camera className="h-4 w-4" />
              </TooltipTrigger>
              <TooltipContent>Screenshot</TooltipContent>
            </Tooltip>

            <Tooltip>
              <TooltipTrigger
                className="inline-flex h-7 w-7 items-center justify-center rounded-lg text-white/80 transition-colors hover:bg-white/15 hover:text-white"
                onClick={() => setIsFullscreen(!isFullscreen)}
                aria-label="Toggle fullscreen"
              >
                <Maximize2 className="h-4 w-4" />
              </TooltipTrigger>
              <TooltipContent>Fullscreen</TooltipContent>
            </Tooltip>
          </div>
        </div>

        {/* Camera viewport – simulated feed */}
        <div className="relative w-full overflow-hidden bg-gradient-to-br from-slate-800 via-slate-900 to-slate-950" style={{ aspectRatio: "16 / 9.5", minHeight: "360px" }}>
          {/* Simulated camera feed background */}
          <div className="absolute inset-0 flex items-center justify-center">
            <div className="relative flex flex-col items-center gap-3">
              <div className="flex h-16 w-16 items-center justify-center rounded-2xl bg-white/5 backdrop-blur-sm">
                <Video className="h-8 w-8 text-white/30" />
              </div>
              <div className="text-center">
                <p className="text-sm font-medium text-white/50">
                  Camera Stream
                </p>
                <p className="mt-0.5 text-xs text-white/30">
                  Connect to robot to view live feed
                </p>
              </div>
            </div>
          </div>

          {/* Grid overlay for camera effect */}
          <div
            className="absolute inset-0 opacity-[0.03]"
            style={{
              backgroundImage:
                "linear-gradient(rgba(255,255,255,0.1) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.1) 1px, transparent 1px)",
              backgroundSize: "40px 40px",
            }}
          />

          {/* Corner indicators */}
          <div className="absolute left-3 top-12 h-5 w-5 border-l-2 border-t-2 border-white/20 rounded-tl-sm" />
          <div className="absolute right-3 top-12 h-5 w-5 border-r-2 border-t-2 border-white/20 rounded-tr-sm" />
          <div className="absolute bottom-3 left-3 h-5 w-5 border-b-2 border-l-2 border-white/20 rounded-bl-sm" />
          <div className="absolute bottom-3 right-3 h-5 w-5 border-b-2 border-r-2 border-white/20 rounded-br-sm" />
        </div>
      </div>
    </DashboardCard>
  );
}
