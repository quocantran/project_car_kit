"""
GPIO Service — Manages Raspberry Pi GPIO for traffic light LEDs, buzzer, and button.

Controls the physical LEDs on the breadboard via GPIO Extension Board.
Falls back to simulation mode when RPi.GPIO is not available (dev machines).
"""

import logging
import asyncio
from typing import Callable, Optional

from config import (
    GPIO_LED_RED_PIN,
    GPIO_LED_GREEN_PIN,
    GPIO_BUZZER_PIN,
    GPIO_BUTTON_PIN,
)

logger = logging.getLogger("gpio_service")

# Safe import for dev/testing on non-Pi systems
try:
    import RPi.GPIO as GPIO
    GPIO_AVAILABLE = True
except (ImportError, RuntimeError):
    GPIO_AVAILABLE = False
    logger.warning("RPi.GPIO not available — running in simulation mode")


class GpioService:
    """
    Manages Raspberry Pi GPIO pins for the traffic light system.

    Hardware:
        - LED Red (GPIO17)  → Indicates STOP
        - LED Green (GPIO27) → Indicates GO
        - Buzzer (GPIO22)   → Audio feedback
        - Button (GPIO23)   → Physical toggle for traffic light
    """

    def __init__(self):
        self.traffic_light: str = "green"  # "green" or "red"
        self.buzzer_on: bool = False
        self._button_callback: Optional[Callable] = None
        self._initialized: bool = False
        self._polling_active: bool = False
        self._polling_thread: Optional[object] = None

    def setup(self, button_callback: Optional[Callable] = None) -> None:
        """
        Initialize GPIO pins.

        Args:
            button_callback: Async-safe callback when button is pressed.
                             Receives no arguments.
        """
        self._button_callback = button_callback

        if not GPIO_AVAILABLE:
            logger.info("GPIO simulation mode — no hardware control")
            self._initialized = True
            return

        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)

        # Output pins
        GPIO.setup(GPIO_LED_RED_PIN, GPIO.OUT, initial=GPIO.LOW)
        GPIO.setup(GPIO_LED_GREEN_PIN, GPIO.OUT, initial=GPIO.HIGH)  # Start green
        # This buzzer circuit is active-LOW: HIGH = OFF, LOW = ON.
        GPIO.setup(GPIO_BUZZER_PIN, GPIO.OUT, initial=GPIO.HIGH)

        # Input pin with pull-up (button connects to GND when pressed)
        GPIO.setup(GPIO_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

        try:
            # Button interrupt — toggle traffic light on falling edge
            GPIO.add_event_detect(
                GPIO_BUTTON_PIN,
                GPIO.FALLING,
                callback=self._on_button_press,
                bouncetime=300,
            )
            logger.info("Button interrupt configured successfully.")
        except RuntimeError as e:
            logger.warning(
                f"Failed to add edge detection interrupt ({e}). "
                "Starting background polling thread for physical button..."
            )
            import threading
            self._polling_active = True
            self._polling_thread = threading.Thread(target=self._poll_button_loop, daemon=True)
            self._polling_thread.start()

        self._initialized = True
        logger.info(
            f"GPIO initialized: LED_RED=GPIO{GPIO_LED_RED_PIN}, "
            f"LED_GREEN=GPIO{GPIO_LED_GREEN_PIN}, "
            f"BUZZER=GPIO{GPIO_BUZZER_PIN}, "
            f"BUTTON=GPIO{GPIO_BUTTON_PIN}"
        )

    def cleanup(self) -> None:
        """Clean up GPIO on shutdown."""
        self._polling_active = False
        if GPIO_AVAILABLE and self._initialized:
            try:
                GPIO.output(GPIO_LED_RED_PIN, GPIO.LOW)
                GPIO.output(GPIO_LED_GREEN_PIN, GPIO.LOW)
                GPIO.output(GPIO_BUZZER_PIN, GPIO.HIGH)
                GPIO.cleanup()
                logger.info("GPIO cleaned up")
            except Exception as e:
                logger.error(f"GPIO cleanup error: {e}")

    def set_traffic_light(self, state: str) -> None:
        """
        Set the physical traffic light LEDs.

        Args:
            state: "green" (allow movement) or "red" (force stop)
        """
        self.traffic_light = state

        if GPIO_AVAILABLE and self._initialized:
            if state == "green":
                GPIO.output(GPIO_LED_GREEN_PIN, GPIO.HIGH)
                GPIO.output(GPIO_LED_RED_PIN, GPIO.LOW)
            else:
                GPIO.output(GPIO_LED_GREEN_PIN, GPIO.LOW)
                GPIO.output(GPIO_LED_RED_PIN, GPIO.HIGH)

        logger.info(f"🚦 Traffic light → {state.upper()}")

    def set_buzzer(self, on: bool) -> None:
        """Turn the active-LOW buzzer on or off."""
        self.buzzer_on = on
        level = GPIO.LOW if on else GPIO.HIGH
        if GPIO_AVAILABLE and self._initialized:
            GPIO.output(GPIO_BUZZER_PIN, level)
            logger.info(
                "Buzzer command: on=%s -> GPIO%d=%s",
                on,
                GPIO_BUZZER_PIN,
                "HIGH" if level == GPIO.HIGH else "LOW",
            )
        else:
            logger.warning(
                "Buzzer command not sent to hardware: on=%s, gpio_available=%s, initialized=%s",
                on,
                GPIO_AVAILABLE,
                self._initialized,
            )

    def _on_button_press(self, channel: int) -> None:
        """
        Handle physical button press — toggle traffic light.
        Runs in GPIO interrupt thread, so we schedule the callback
        in the event loop if one is registered.
        """
        new_state = "red" if self.traffic_light == "green" else "green"
        self.set_traffic_light(new_state)

        if self._button_callback:
            try:
                # Try to schedule in the running event loop
                loop = asyncio.get_running_loop()
                loop.call_soon_threadsafe(self._button_callback)
            except RuntimeError:
                # No running loop — just call directly
                self._button_callback()

    def _poll_button_loop(self) -> None:
        """Poll the button state in a background thread as a fallback for edge detection."""
        import time
        prev_state = 1  # Pull-up default state (unpressed is HIGH)
        
        while GPIO_AVAILABLE and self._polling_active:
            try:
                current_state = GPIO.input(GPIO_BUTTON_PIN)
                # Falling edge detection: from high (1) to low (0)
                if prev_state == 1 and current_state == 0:
                    logger.info("Button press detected via polling fallback")
                    self._on_button_press(GPIO_BUTTON_PIN)
                    # Debounce
                    time.sleep(0.3)
                    # Update prev_state after sleep to prevent duplicate registers
                    prev_state = GPIO.input(GPIO_BUTTON_PIN)
                else:
                    prev_state = current_state
                    time.sleep(0.05)  # Poll every 50ms
            except Exception as e:
                logger.error(f"Error in button polling loop: {e}")
                time.sleep(1.0)

