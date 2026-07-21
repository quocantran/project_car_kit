"use client";

import { useState, useEffect, useRef, useCallback } from "react";
import type { TelemetryData, ConnectionStatus } from "@/types";

const WS_URL = "ws://192.168.50.254:8000/ws";
const RECONNECT_DELAY = 3000; // ms

interface RobotTelemetry {
  battery: number;
  speed: number;
  speed_percent: number;
  distance: number;
  is_connected: boolean;
  current_direction: string;
  current_mode: string;
  traffic_light: string;
  buzzer: boolean;
}

interface UseRobotWebSocketReturn {
  /** Current telemetry data from the robot */
  telemetry: TelemetryData;
  /** BLE connection status (robot ↔ backend) */
  bleConnected: boolean;
  /** WebSocket connection status (frontend ↔ backend) */
  wsStatus: ConnectionStatus;
  /** Device info */
  deviceName: string | null;
  deviceAddress: string | null;
  /** Send a movement command */
  sendControl: (direction: string, speed?: number) => void;
  /** Update speed only */
  sendSpeed: (speed: number) => void;
  /** Set servo angle */
  sendServo: (angle: number) => void;
  /** Switch autonomous mode */
  sendMode: (mode: string) => void;
  /** Reset all functions */
  sendReset: () => void;
  /** Connect to robot BLE */
  connectBLE: (address?: string) => void;
  /** Disconnect from robot BLE */
  disconnectBLE: () => void;
  /** Send traffic light control command */
  sendTrafficLight: (state: "green" | "red") => void;
  /** Current traffic light state */
  trafficLight: "green" | "red";
  /** Send buzzer toggle command */
  sendBuzzer: (on: boolean) => void;
}

export function useRobotWebSocket(): UseRobotWebSocketReturn {
  const [telemetry, setTelemetry] = useState<TelemetryData>({
    battery: 0,
    speed: 0,
    temperature: 25,
    distance: 0,
    runtime: "00:00:00",
    voltage: 7.4,
    trafficLight: "green",
    buzzer: false,
  });

  const [bleConnected, setBleConnected] = useState(false);
  const [wsStatus, setWsStatus] = useState<ConnectionStatus>("disconnected");
  const [deviceName, setDeviceName] = useState<string | null>(null);
  const [deviceAddress, setDeviceAddress] = useState<string | null>(null);

  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const runtimeRef = useRef(0);
  const runtimeIntervalRef = useRef<ReturnType<typeof setInterval> | null>(null);

  // Runtime counter
  useEffect(() => {
    if (bleConnected) {
      runtimeIntervalRef.current = setInterval(() => {
        runtimeRef.current += 1;
        const h = Math.floor(runtimeRef.current / 3600)
          .toString()
          .padStart(2, "0");
        const m = Math.floor((runtimeRef.current % 3600) / 60)
          .toString()
          .padStart(2, "0");
        const s = (runtimeRef.current % 60).toString().padStart(2, "0");
        setTelemetry((prev) => ({ ...prev, runtime: `${h}:${m}:${s}` }));
      }, 1000);
    } else {
      if (runtimeIntervalRef.current) {
        clearInterval(runtimeIntervalRef.current);
      }
      runtimeRef.current = 0;
    }
    return () => {
      if (runtimeIntervalRef.current) {
        clearInterval(runtimeIntervalRef.current);
      }
    };
  }, [bleConnected]);

  // WebSocket connection
  const connectWS = useCallback(() => {
    if (
      wsRef.current &&
      (wsRef.current.readyState === WebSocket.OPEN ||
        wsRef.current.readyState === WebSocket.CONNECTING)
    ) {
      return;
    }

    setWsStatus("connecting");

    const ws = new WebSocket(WS_URL);
    wsRef.current = ws;

    ws.onopen = () => {
      setWsStatus("connected");
      console.log("[WS] Connected to backend");
    };

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        handleMessage(msg);
      } catch (err) {
        console.error("[WS] Parse error:", err);
      }
    };

    ws.onclose = () => {
      setWsStatus("disconnected");
      console.log("[WS] Disconnected from backend");

      // Auto-reconnect
      reconnectTimerRef.current = setTimeout(() => {
        console.log("[WS] Attempting reconnect...");
        connectWS();
      }, RECONNECT_DELAY);
    };

    ws.onerror = (err) => {
      console.error("[WS] Error:", err);
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const handleMessage = useCallback((msg: { type: string; data: Record<string, unknown> }) => {
    switch (msg.type) {
      case "telemetry": {
        const t = msg.data as unknown as RobotTelemetry;
        setTelemetry((prev) => ({
          ...prev,
          battery: t.battery,
          speed: t.speed_percent,
          distance: t.distance / 100, // cm → m
          voltage: 7.4 * (t.battery / 100), // Estimated from battery %
          trafficLight: (t.traffic_light as "green" | "red") || "green",
          buzzer: t.buzzer || false,
        }));
        setBleConnected(t.is_connected);
        break;
      }
      case "traffic_light_update": {
        const d = msg.data as { state: "green" | "red" };
        setTelemetry((prev) => ({
          ...prev,
          trafficLight: d.state,
        }));
        break;
      }
      case "connection_status": {
        const d = msg.data as { is_connected: boolean; device_name?: string; device_address?: string };
        setBleConnected(d.is_connected);
        setDeviceName(d.device_name ?? null);
        setDeviceAddress(d.device_address ?? null);
        break;
      }
      case "command_response": {
        console.log("[WS] Command response:", msg.data);
        break;
      }
      case "error": {
        console.error("[WS] Server error:", msg.data);
        break;
      }
      case "pong":
        break;
      default:
        console.log("[WS] Unknown message:", msg);
    }
  }, []);

  // Connect on mount, cleanup on unmount
  useEffect(() => {
    connectWS();

    return () => {
      if (reconnectTimerRef.current) {
        clearTimeout(reconnectTimerRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [connectWS]);

  // ── Send Functions ────────────────────────────────────────

  const sendJSON = useCallback((data: Record<string, unknown>) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(data));
    }
  }, []);

  const sendControl = useCallback(
    (direction: string, speed?: number) => {
      sendJSON({ type: "control", direction, speed: speed ?? 200 });
    },
    [sendJSON]
  );

  const sendSpeed = useCallback(
    (speed: number) => {
      sendJSON({ type: "speed", speed });
    },
    [sendJSON]
  );

  const sendServo = useCallback(
    (angle: number) => {
      sendJSON({ type: "servo", angle });
    },
    [sendJSON]
  );

  const sendMode = useCallback(
    (mode: string) => {
      sendJSON({ type: "mode", mode });
    },
    [sendJSON]
  );

  const sendReset = useCallback(() => {
    sendJSON({ type: "reset" });
  }, [sendJSON]);

  const connectBLE = useCallback(
    (address?: string) => {
      sendJSON({ type: "connect", serial_port: address });
    },
    [sendJSON]
  );

  const disconnectBLE = useCallback(() => {
    sendJSON({ type: "disconnect" });
  }, [sendJSON]);

  const sendTrafficLight = useCallback(
    (state: "green" | "red") => {
      sendJSON({ type: "traffic_light", state });
    },
    [sendJSON]
  );

  const sendBuzzer = useCallback(
    (on: boolean) => {
      sendJSON({ type: "buzzer", on });
    },
    [sendJSON]
  );

  return {
    telemetry,
    bleConnected,
    wsStatus,
    deviceName,
    deviceAddress,
    sendControl,
    sendSpeed,
    sendServo,
    sendMode,
    sendReset,
    connectBLE,
    disconnectBLE,
    sendTrafficLight,
    trafficLight: telemetry.trafficLight,
    sendBuzzer,
  };
}
