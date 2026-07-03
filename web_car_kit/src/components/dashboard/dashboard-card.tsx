"use client";

import { motion } from "framer-motion";
import { cn } from "@/lib/utils";

interface DashboardCardProps {
  children: React.ReactNode;
  className?: string;
  title?: string;
  subtitle?: string;
  headerAction?: React.ReactNode;
  noPadding?: boolean;
}

export function DashboardCard({
  children,
  className,
  title,
  subtitle,
  headerAction,
  noPadding = false,
}: DashboardCardProps) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.4, ease: "easeOut" }}
      className={cn(
        "rounded-2xl border border-border/60 bg-card shadow-[0_1px_3px_0_rgb(0_0_0/0.04)] transition-shadow hover:shadow-[0_4px_16px_0_rgb(0_0_0/0.06)]",
        className
      )}
    >
      {(title || headerAction) && (
        <div className="flex items-center justify-between border-b border-border/50 px-5 py-3.5">
          <div>
            {title && (
              <h3 className="text-sm font-semibold text-foreground">
                {title}
              </h3>
            )}
            {subtitle && (
              <p className="mt-0.5 text-xs text-muted-foreground">{subtitle}</p>
            )}
          </div>
          {headerAction && <div>{headerAction}</div>}
        </div>
      )}
      <div className={cn(!noPadding && "p-5")}>{children}</div>
    </motion.div>
  );
}
