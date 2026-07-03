"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { motion } from "framer-motion";
import {
  LayoutDashboard,
  Settings,
  Wifi,
  Info,
  Bot,
} from "lucide-react";
import { cn } from "@/lib/utils";
import { RobotStatusCard } from "./robot-status";
import type { RobotStatus, ConnectionStatus } from "@/types";

const navItems = [
  { label: "Dashboard", href: "/", icon: LayoutDashboard },
  { label: "Settings", href: "/settings", icon: Settings },
  { label: "Connection", href: "/connection", icon: Wifi },
  { label: "About", href: "/about", icon: Info },
];

interface SidebarProps {
  status: RobotStatus;
  connectionStatus: ConnectionStatus;
}

export function Sidebar({ status, connectionStatus }: SidebarProps) {
  const pathname = usePathname();

  return (
    <aside
      className="fixed inset-y-0 left-0 z-30 flex w-[320px] flex-col border-r border-border/50 bg-white"
      aria-label="Main navigation"
    >
      {/* Logo */}
      <div className="flex items-center gap-2.5 border-b border-border/50 px-5 py-5">
        <div>
          <h1 className="text-lg font-extrabold tracking-tight text-foreground">
            ELEG<span className="text-primary">OO</span>
          </h1>
          <p className="text-[10px] font-medium uppercase tracking-[0.2em] text-muted-foreground">
            Smart Robot Car
          </p>
        </div>
      </div>

      {/* Navigation */}
      <nav className="px-3 py-4" aria-label="Sidebar navigation">
        <ul className="flex flex-col gap-1" role="list">
          {navItems.map((item) => {
            const isActive = pathname === item.href;
            return (
              <li key={item.href}>
                <Link
                  href={item.href}
                  className={cn(
                    "group relative flex items-center gap-3 rounded-xl px-3 py-2.5 text-sm font-medium transition-all",
                    isActive
                      ? "text-primary"
                      : "text-muted-foreground hover:bg-muted/60 hover:text-foreground"
                  )}
                  aria-current={isActive ? "page" : undefined}
                >
                  {isActive && (
                    <motion.div
                      layoutId="sidebar-active"
                      className="absolute inset-0 rounded-xl bg-primary/[0.06]"
                      transition={{
                        type: "spring",
                        stiffness: 350,
                        damping: 30,
                      }}
                    />
                  )}
                  <item.icon className="relative h-[18px] w-[18px]" />
                  <span className="relative">{item.label}</span>
                </Link>
              </li>
            );
          })}
        </ul>
      </nav>

      {/* Robot image placeholder */}
      <div className="flex justify-center px-5 py-3">
        <div className="flex h-[140px] w-[140px] items-center justify-center rounded-2xl bg-gradient-to-br from-muted/30 to-muted/60">
          <Bot className="h-16 w-16 text-muted-foreground/30" />
        </div>
      </div>

      {/* Robot Status */}
      <div className="flex-1 overflow-y-auto px-3 pb-4">
        <RobotStatusCard
          status={status}
          connectionStatus={connectionStatus}
        />
      </div>

      {/* Footer */}
      <div className="border-t border-border/50 px-5 py-4">
        <p className="text-[10px] font-medium text-muted-foreground">
          Firmware v2.1.0
        </p>
        <p className="text-[10px] text-muted-foreground/70">
          © 2025 SmartCar IoT
        </p>
      </div>
    </aside>
  );
}
