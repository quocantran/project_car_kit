"use client";

import { cn } from "@/lib/utils";

interface CircularGaugeProps {
  value: number;
  max: number;
  label: string;
  unit: string;
  size?: number;
  strokeWidth?: number;
  color?: string;
}

export function CircularGauge({
  value,
  max,
  label,
  unit,
  size = 120,
  strokeWidth = 8,
  color = "var(--primary)",
}: CircularGaugeProps) {
  const radius = (size - strokeWidth) / 2;
  const circumference = 2 * Math.PI * radius;
  const percentage = Math.min(value / max, 1);
  const offset = circumference * (1 - percentage);
  const center = size / 2;

  return (
    <div className="flex flex-col items-center gap-1.5">
      <div className="relative" style={{ width: size, height: size }}>
        <svg
          width={size}
          height={size}
          viewBox={`0 0 ${size} ${size}`}
          className="-rotate-90"
          aria-label={`${label}: ${value}${unit}`}
          role="meter"
          aria-valuenow={value}
          aria-valuemin={0}
          aria-valuemax={max}
        >
          {/* Background track */}
          <circle
            cx={center}
            cy={center}
            r={radius}
            fill="none"
            stroke="oklch(0.94 0 0)"
            strokeWidth={strokeWidth}
            strokeLinecap="round"
          />
          {/* Value arc */}
          <circle
            cx={center}
            cy={center}
            r={radius}
            fill="none"
            stroke={color}
            strokeWidth={strokeWidth}
            strokeLinecap="round"
            strokeDasharray={circumference}
            strokeDashoffset={offset}
            className="transition-all duration-1000 ease-out"
            style={
              {
                "--gauge-circumference": circumference,
                "--gauge-offset": offset,
              } as React.CSSProperties
            }
          />
        </svg>
        {/* Center value */}
        <div className="absolute inset-0 flex flex-col items-center justify-center">
          <span className="text-2xl font-bold tracking-tight text-foreground">
            {value}
            <span className="text-sm font-medium text-muted-foreground">
              {unit}
            </span>
          </span>
        </div>
      </div>
      <span className={cn("text-xs font-medium text-muted-foreground")}>
        {label}
      </span>
    </div>
  );
}
