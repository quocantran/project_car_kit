import type { Metadata } from "next";
import { Inter } from "next/font/google";
import { TooltipProvider } from "@/components/ui/tooltip";
import "./globals.css";

const inter = Inter({
  variable: "--font-sans",
  subsets: ["latin"],
  display: "swap",
});

export const metadata: Metadata = {
  title: "Smart Robot Car | Dashboard",
  description:
    "Real-time control and monitoring dashboard for your IoT Smart Robot Car. Manage camera, telemetry, controls, and recorded videos.",
  keywords: [
    "robot car",
    "IoT",
    "dashboard",
    "smart car",
    "remote control",
    "telemetry",
  ],
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className={`${inter.variable} antialiased`}>
        <TooltipProvider delay={300}>{children}</TooltipProvider>
      </body>
    </html>
  );
}
