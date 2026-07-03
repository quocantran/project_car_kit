"use client";

import { motion } from "framer-motion";
import {
  Download,
  MoreHorizontal,
  Video,
  HardDrive,
  Calendar,
  Film,
} from "lucide-react";
import { Button } from "@/components/ui/button";
import { Tooltip, TooltipContent, TooltipTrigger } from "@/components/ui/tooltip";
import { DashboardCard } from "./dashboard-card";
import type { RecordedVideo } from "@/types";

interface RecordedVideosProps {
  videos: RecordedVideo[];
}

export function RecordedVideos({ videos }: RecordedVideosProps) {
  return (
    <DashboardCard
      title="Recorded Videos"
      headerAction={
        <Button variant="ghost" size="xs" className="text-primary text-xs">
          View All
        </Button>
      }
    >
      {videos.length === 0 ? (
        <div className="flex flex-col items-center gap-3 py-6 text-center">
          <div className="flex h-12 w-12 items-center justify-center rounded-2xl bg-muted/40">
            <Film className="h-6 w-6 text-muted-foreground/40" />
          </div>
          <div>
            <p className="text-xs font-semibold text-muted-foreground/70">
              No recordings yet
            </p>
            <p className="mt-0.5 text-[10px] text-muted-foreground/50">
              Start recording from the camera feed
            </p>
          </div>
        </div>
      ) : (
        <div className="flex flex-col gap-3">
          {videos.map((video, index) => (
            <motion.div
              key={video.id}
              initial={{ opacity: 0, y: 8 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: index * 0.05 }}
              className="group flex items-center gap-4 rounded-xl border border-border/40 bg-muted/5 p-3 transition-all hover:border-border hover:bg-muted/15"
            >
              {/* Thumbnail */}
              <div className="relative h-16 w-28 shrink-0 overflow-hidden rounded-lg bg-gradient-to-br from-slate-100 to-slate-200">
                <div className="absolute inset-0 flex items-center justify-center">
                  <div className="flex h-8 w-8 items-center justify-center rounded-full bg-black/10 backdrop-blur-sm transition-transform group-hover:scale-110">
                    <Video className="h-4 w-4 text-slate-500" />
                  </div>
                </div>
                {/* Duration tag */}
                <div className="absolute bottom-1 right-1 rounded bg-black/60 px-1.5 py-0.5 text-[9px] font-medium text-white backdrop-blur-sm">
                  {video.duration}
                </div>
              </div>

              {/* Info */}
              <div className="min-w-0 flex-1">
                <p
                  className="truncate text-xs font-semibold text-foreground leading-snug"
                  title={video.filename}
                >
                  {video.filename}
                </p>
                <div className="mt-1.5 flex items-center gap-3 text-[10px] text-muted-foreground">
                  <span className="flex items-center gap-1">
                    <Calendar className="h-3 w-3" />
                    {video.date}
                  </span>
                  <span className="flex items-center gap-1">
                    <HardDrive className="h-3 w-3" />
                    {video.fileSize}
                  </span>
                </div>
              </div>

              {/* Actions */}
              <div className="flex items-center gap-1">
                <Tooltip>
                  <TooltipTrigger
                    className="inline-flex h-8 w-8 items-center justify-center rounded-md text-muted-foreground transition-colors hover:bg-muted hover:text-primary"
                    aria-label={`Download ${video.filename}`}
                  >
                    <Download className="h-4 w-4" />
                  </TooltipTrigger>
                  <TooltipContent>Download</TooltipContent>
                </Tooltip>
                <Tooltip>
                  <TooltipTrigger
                    className="inline-flex h-8 w-8 items-center justify-center rounded-md text-muted-foreground transition-colors hover:bg-muted hover:text-foreground"
                    aria-label={`More actions for ${video.filename}`}
                  >
                    <MoreHorizontal className="h-4 w-4" />
                  </TooltipTrigger>
                  <TooltipContent>More</TooltipContent>
                </Tooltip>
              </div>
            </motion.div>
          ))}
        </div>
      )}
    </DashboardCard>
  );
}
