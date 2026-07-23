/*
 * @Description: In User Settings Edi
 * @Author: your name
 * @Date: 2019-08-12 18:00:25
 * @LastEditTime: 2020-12-11 16:07:37
 * @LastEditors: Changhua
 */
#include "ArduinoJson-v6.11.1.h" //Use ArduinoJson Libraries
#include "HardwareSerial.h"
#include <IRremote.h>
#include <Servo.h>
#include <stdio.h>

#define f 1286666973 // FORWARD
#define b 2747854299 // BACK
#define l 1386468383 // LEFT
#define r 553536955  // RIGHT
#define s 16712445   // STOP

#define UNKNOWN_F 5316027    // FORWARD
#define UNKNOWN_B 2747854299 // BACK
#define UNKNOWN_L 1386468383 // LEFT
#define UNKNOWN_R 553536955  // RIGHT
#define UNKNOWN_S 3622325019 // STOP

#define KEY1 16738455 // Line Teacking mode
#define KEY2 16750695 // Obstacles Avoidance mode

#define KEY_STAR 16728765
#define KEY_HASH 16732845

/*Arduino pin is connected to the IR Receiver*/
#define RECV_PIN 12

/*Arduino pin is connected to the Ultrasonic sensor module*/
#define ECHO_PIN A4
#define TRIG_PIN A5
const int ObstacleDetection = 35;

/*Arduino pin is connected to the Motor drive module*/
#define ENA 5
#define ENB 6
#define IN1 8
#define IN2 7
#define IN3 11
#define IN4 9

#define LED_Pin 13

/*Arduino pin is connected to the IR tracking module*/
#define LineTeacking_Pin_Right 10
#define LineTeacking_Pin_Middle 4
#define LineTeacking_Pin_Left 2

#define LineTeacking_Read_Right !digitalRead(10) // Right
#define LineTeacking_Read_Middle !digitalRead(4) // Middle
#define LineTeacking_Read_Left !digitalRead(2)   // Left

#define carSpeed 250 // PWM(Motor speed/Speed)

unsigned int carSpeed_rocker = 250;

// Traffic Light control — Raspberry Pi controls via N=50 command
// true = green (allow movement), false = red (force stop)
boolean traffic_light_green = true;

/*══════════════════════════════════════════════════════════════
 * HAND TRACKING — Configurable Constants (dễ thay đổi)
 * Thay đổi các giá trị bên dưới để điều chỉnh hành vi theo dõi tay
 *══════════════════════════════════════════════════════════════*/
#define HT_TARGET_DIST                                                         \
  20 // Khoảng cách mục tiêu (cm) — xe giữ khoảng cách này với tay
#define HT_DEADZONE 4 // Vùng chết ±cm — trong vùng này xe đứng yên
#define HT_HYSTERESIS                                                          \
  3 // cm thêm khi xe đã dừng — tránh dao động (deadzone mở rộng = ±7cm)
#define HT_SAFE_MIN                                                            \
  10 // Khoảng cách tối thiểu an toàn (cm) — gần hơn sẽ lùi tốc độ max
#define HT_MAX_RANGE 50 // Phạm vi phát hiện tối đa (cm) — xa hơn sẽ dừng
#define HT_SPEED_MIN 80 // Tốc độ PWM tối thiểu (0-255) — thấp hơn = ít quá đà
#define HT_SPEED_MAX                                                           \
  140 // Tốc độ PWM tối đa (0-255) (giảm để tránh sụt áp nguồn)
#define HT_EMA_ALPHA                                                           \
  0.45 // Hệ số lọc EMA (0.0-1.0) — tăng lên để phản hồi nhanh hơn, giảm trễ
#define HT_RAMP_STEP                                                           \
  15 // Bước tăng/giảm tốc tối đa mỗi chu kỳ (giảm để khởi động mượt hơn, tránh
     // giật dòng)
#define HT_UPDATE_INTERVAL                                                     \
  50 // ms giữa mỗi lần đọc sensor — giảm xuống để cập nhật nhanh hơn
#define HT_STABLE_COUNT                                                        \
  3 // Số lần đọc liên tục trước khi đổi hướng — chống dao động

// ── Hand Tracking Phase 2: Search & Align ──
#define HT_SERVO_SETTLE 200 // ms chờ servo ổn định mỗi lần quay
#define HT_LOST_COUNT                                                          \
  10 // Số lần đọc liên tục mất tay trước khi SEARCH (10*50ms=500ms)
#define HT_ALIGN_SPEED                                                         \
  140 // PWM quay thân xe (đủ lực để thắng ma sát bánh cao su, giảm sụt nguồn)
#define HT_ALIGN_CHECK_INT 100 // ms giữa mỗi lần kiểm tra khi ALIGN
#define HT_ALIGN_TIMEOUT 3000  // ms tối đa trong ALIGN trước khi → SEARCH
#define HT_SCAN_COUNT 7        // Số góc scan zig-zag

// Divisor for ultrasonic sensor calibration.
// Increase it if the measured distance is larger than reality; decrease if
// smaller.
#define ULTRASONIC_DIVISOR 48

// Throttle getDistance() to avoid blocking serial reads
unsigned long lastDistanceMillis = 0;
#define DISTANCE_INTERVAL 500 // Only read distance every 500ms
#define PIN_Servo 3
Servo servo;             //  Create a DC motor drive object
IRrecv irrecv(RECV_PIN); //  Create an infrared receive drive object
decode_results results;  //  Create decoding object

unsigned long IR_PreMillis;
unsigned long LT_PreMillis;

int rightDistance = 0;  // Right distance
int leftDistance = 0;   // left Distance
int middleDistance = 0; // middle Distance

/*CMD_MotorControl: Motor Control： Motor Speed、Motor Direction、Motor Time*/
uint8_t CMD_MotorSelection;
uint8_t CMD_MotorDirection;

uint16_t CMD_MotorSpeed;
unsigned long CMD_leftMotorControl_Millis;
unsigned long CMD_rightMotorControl_Millis;

/*CMD_CarControl: Car Control：Car moving direction、Car Speed、Car moving
 * time*/
uint8_t CMD_CarDirection;
uint8_t CMD_CarSpeed;
uint16_t CMD_CarTimer;
unsigned long CMD_CarControl_Millis;

uint8_t CMD_CarDirectionxxx;
uint8_t CMD_CarSpeedxxx;
uint16_t CMD_Distance;

String CommandSerialNumber; //

enum SERIAL_mode {
  Serial_rocker,
  Serial_programming,
  Serial_CMD,
} Serial_mode = Serial_programming;

enum FUNCTIONMODE {
  IDLE,                  /*free*/
  LineTeacking,          /*Line Teacking Mode*/
  ObstaclesAvoidance,    /*Obstacles Avoidance Mode*/
  Bluetooth,             /*Bluetooth Control Mode*/
  IRremote,              /*Infrared Control Mode*/
  CMD_MotorControl,      /*Motor Control Mode*/
  CMD_CarControl,        /*Car Control Mode*/
  CMD_CarControlxxx,     /*Car Control Mode*/
  CMD_ClearAllFunctions, /*Clear All Functions Mode*/
  HandTracking,          /*Hand Tracking Mode (Phase 1)*/
} func_mode = IDLE;      /*Functional mode*/

enum MOTIONMODE {
  LEFT,    /*left*/
  RIGHT,   /*right*/
  FORWARD, /*forward*/
  BACK,    /*back*/
  STOP,    /*stop*/
  LEFT_FORWARD,
  LEFT_BACK,
  RIGHT_FORWARD,
  RIGHT_BACK,
} mov_mode = STOP; /*move mode*/

enum HT_STATE {
  HT_TRACK_DISTANCE, // Theo dõi tiến/lùi (Phase 1)
  HT_LOST_WAIT,      // Chờ xác nhận mất tay trước khi scan
  HT_SEARCH_HAND,    // Quét servo zig-zag tìm tay
  HT_ALIGN_HEADING,  // Quay thân xe căn hướng
};

void delays(unsigned long t) {
  for (unsigned long i = 0; i < t; i++) {
    getBTData_Plus(); // Bluetooth Communication Data Acquisition
    getIRData();      // Infrared Communication Data Acquisition
    delay(1);
  }
}
// Safe delay — does NOT process serial commands (prevents mode hijack during
// hand tracking)
void safe_delay(unsigned long t) { delay(t); }
/*ULTRASONIC*/
unsigned int getDistance(void) { // Getting distance
  static unsigned int tempda = 0;
  unsigned int tempda_x = 0;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Timeout 25000 microseconds (~4.2 mét). Nếu quá tầm đo, pulseIn trả về 0.
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000);

  // Method 1: If first pulse failed, retry immediately to filter transient
  // specular reflection dropouts
  if (duration == 0) {
    delayMicroseconds(500); // Small pause
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH, 25000);
  }

  if (duration == 0) {
    tempda_x = 150; // Không có vật cản / ngoài tầm đo
  } else {
    tempda_x = (unsigned int)(duration / ULTRASONIC_DIVISOR);
    if (tempda_x > 150 || tempda_x == 0) {
      tempda_x = 150;
    }
  }
  return tempda_x;
}
/*
  Control motor：Car movement forward
*/
void forward(bool debug, int16_t in_carSpeed) {
  analogWrite(ENA, in_carSpeed);
  analogWrite(ENB, in_carSpeed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  if (debug)
    Serial.println("Go forward!");
}
/*
  Control motor：Car moving backwards
*/
void back(bool debug, int16_t in_carSpeed) {
  analogWrite(ENA, in_carSpeed);
  analogWrite(ENB, in_carSpeed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  if (debug)
    Serial.println("Go back!");
}
/*
  Control motor：The car turns left and moves forward
*/
void left(bool debug, int16_t in_carSpeed) {

  analogWrite(ENA, in_carSpeed);
  analogWrite(ENB, in_carSpeed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  if (debug)
    Serial.println("Go left!");
}
/*
  Control motor：The car turns right and moves forward
*/
void right(bool debug, int16_t in_carSpeed) {
  int16_t speedA =
      in_carSpeed + 50; // Tăng công suất động cơ trái (đi lùi) để bù ma sát
  if (speedA > 255)
    speedA = 255;
  analogWrite(ENA, speedA);
  analogWrite(ENB, in_carSpeed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  if (debug)
    Serial.println("Go right!");
}

void forward_left(bool debug, int16_t in_carSpeed) {
  analogWrite(ENA, in_carSpeed);
  analogWrite(ENB, in_carSpeed / 2);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  if (debug)
    Serial.println("Go forward left!");
}

void forward_right(bool debug, int16_t in_carSpeed) {
  analogWrite(ENA, in_carSpeed / 2);
  analogWrite(ENB, in_carSpeed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  if (debug)
    Serial.println("Go forward right!");
}

void back_left(bool debug, int16_t in_carSpeed) {
  analogWrite(ENA, in_carSpeed);
  analogWrite(ENB, in_carSpeed / 2);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  if (debug)
    Serial.println("Go back left!");
}

void back_right(bool debug, int16_t in_carSpeed) {
  analogWrite(ENA, in_carSpeed / 2);
  analogWrite(ENB, in_carSpeed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  if (debug)
    Serial.println("Go back right!");
}

/*
  Stop motor control：Turn off the motor drive
*/
void stop(bool debug = false) {
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  if (debug)
    Serial.println("Stop!");
}
/*
 Servo Control angle Setting
*/
void ServoControl(uint8_t angleSetting) {
  if (angleSetting > 175) {
    angleSetting = 175;
  } else if (angleSetting < 5) {
    angleSetting = 5;
  }
  servo.attach(3);
  servo.write(angleSetting); // sets the servo position according to the  value
  delays(500);
  servo.detach();
}

/*
  Non-blocking servo write for Hand Tracking scan/align.
  Sets angle and returns immediately — caller manages timing.
*/
void servoWriteNonBlocking(uint8_t angle) {
  angle = constrain(angle, 5, 175);
  servo.attach(PIN_Servo);
  servo.write(angle);
}

/*
  Infrared Communication Data Acquisition
*/
void getIRData(void) {
  if (irrecv.decode(&results)) {
    IR_PreMillis = millis();
    switch (results.value) {
    case f:
      func_mode = IRremote;
      mov_mode = FORWARD;
      break; /*forward*/
    case b:
      func_mode = IRremote;
      mov_mode = BACK;
      break; /*back*/
    case l:
      func_mode = IRremote;
      mov_mode = LEFT;
      break; /*left*/
    case r:
      func_mode = IRremote;
      mov_mode = RIGHT;
      break; /*right*/
    case s:
    case UNKNOWN_S:
      func_mode = IRremote;
      mov_mode = STOP;
      break; /*stop*/
    case KEY1:
      func_mode = LineTeacking;
      break; /*Line Teacking Mode*/
    case KEY2:
      func_mode = ObstaclesAvoidance;
      break; /*Obstacles Avoidance Mode*/
    default:
      break;
    }
    irrecv.resume();
  }
}
/*
  Bluetooth remote control mode
*/
void bluetooth_mode() {
  if (func_mode == Bluetooth) {
    // Traffic light check: red light = force stop
    if (!traffic_light_green) {
      stop();
      return;
    }
    switch (mov_mode) {
    case LEFT:
      left(false, carSpeed_rocker);
      break;
    case RIGHT:
      right(false, carSpeed_rocker);
      break;
    case FORWARD:
      forward(false, carSpeed_rocker);
      break;
    case BACK:
      back(false, carSpeed_rocker);
      break;
    case STOP:
      stop();
      break;
    case LEFT_FORWARD:
      forward_left(false, carSpeed_rocker);
      break;
    case LEFT_BACK:
      back_left(false, carSpeed_rocker);
      break;
    case RIGHT_FORWARD:
      forward_right(false, carSpeed_rocker);
      break;
    case RIGHT_BACK:
      back_right(false, carSpeed_rocker);
      break;
    default:
      break;
    }
  }
}
/*
  Infrared remote control mode
*/
void irremote_mode(void) {
  if (func_mode == IRremote) {
    // Traffic light check: red light = force stop
    if (!traffic_light_green) {
      stop();
      if (millis() - IR_PreMillis > 500) {
        mov_mode = STOP;
        IR_PreMillis = millis();
      }
      return;
    }
    switch (mov_mode) {
    case FORWARD:
      forward(false, carSpeed);
      break;
    case BACK:
      back(false, carSpeed);
      break;
    case LEFT:
      left(false, carSpeed);
      break;
    case RIGHT:
      right(false, carSpeed);
      break;
    case STOP:
      stop();
      break;
    default:
      break;
    }
    if (millis() - IR_PreMillis > 500) {
      mov_mode = STOP;
      IR_PreMillis = millis();
    }
  }
}
/*
  Line Teacking Mode
*/
void line_teacking_mode(void) {
  if (func_mode == LineTeacking) {
    // Traffic light check: red light = force stop
    if (!traffic_light_green) {
      stop();
      return;
    }
    if (LineTeacking_Read_Middle) { // Detecting in the middle infrared tube

      forward(false, 180); // Control motor：the car moving forward
      LT_PreMillis = millis();
    } else if (LineTeacking_Read_Right) { // Detecting in the right infrared
                                          // tube

      right(false, 180); // Control motor：the car moving right
      while (LineTeacking_Read_Right) {
        getBTData_Plus(); // Bluetooth data acquisition
        getIRData();      // Infrared data acquisition
      }
      LT_PreMillis = millis();
    } else if (LineTeacking_Read_Left) { // Detecting in the left infrared tube
      left(false, 180);                  // Control motor：the car moving left
      while (LineTeacking_Read_Left) {
        getBTData_Plus(); // Bluetooth data acquisition
        getIRData();      // Infrared data acquisition
      }
      LT_PreMillis = millis();
    } else {
      if (millis() - LT_PreMillis > 150) {
        stop(); // Stop motor control：Turn off motor drive mode
      }
    }
  }
}

/*f(x) int */
static boolean function_xxx(long xd, long sd, long ed) // f(x)
{
  if (sd <= xd && xd <= ed)
    return true;
  else
    return false;
}

/*Obstacle avoidance*/

void obstacles_avoidance_mode(void) {
  static boolean first_is = true;
  uint8_t switc_ctrl = 0;
  static uint8_t get_Distance = 150;

  static unsigned long last_sweep_time = 0;
  static uint8_t sweep_state =
      0; // 0: 50, 1: 90, 2: 130, 3: 90 (Method 3: wider sweep)
  static uint8_t current_angle = 90;

  // Method 2: Monotonic Jump Filter history variables
  static uint8_t last_valid_dist = 150;
  static uint8_t spike_counter = 0;

  if (func_mode == ObstaclesAvoidance) {
    // Traffic light check: red light = force stop
    if (!traffic_light_green) {
      stop();
      return;
    }
    if (first_is == true) // Enter the mode for the first time
    {
      ServoControl(90);
      first_is = false;
      current_angle = 90;
      sweep_state = 1;
      last_sweep_time = millis();
      last_valid_dist = 150;
      spike_counter = 0;
    }

    // 1. Gentle non-blocking sweep while moving forward to capture angled wall
    // reflections
    if (millis() - last_sweep_time >= 120) { // Every 120ms
      // Measure at the current settled angle
      uint8_t measured_dist = getDistance();

      // Method 2: Monotonic Jump Filter
      // If the sensor suddenly reports 150cm (no obstacle), but the last valid
      // reading was close (< 35cm), ignore the jump for up to 3 cycles (approx
      // 360ms) to bypass temporary specular reflection dropouts.
      if (measured_dist == 150 && last_valid_dist < 35) {
        spike_counter++;
        if (spike_counter <= 3) {
          get_Distance = last_valid_dist; // Hold the last close distance
        } else {
          get_Distance = 150; // Confirm obstacle is actually gone
          last_valid_dist = 150;
          spike_counter = 0;
        }
      } else {
        get_Distance = measured_dist;
        last_valid_dist = measured_dist;
        spike_counter = 0;
      }

      // Select next sweep angle (50 -> 90 -> 130 -> 90) and move servo
      sweep_state = (sweep_state + 1) % 4;
      if (sweep_state == 0)
        current_angle = 50;
      else if (sweep_state == 2)
        current_angle = 130;
      else
        current_angle = 90;

      servoWriteNonBlocking(current_angle);
      last_sweep_time = millis();
    }

    // 2. Stop-and-Scan Decision
    if (function_xxx(get_Distance, 0, 25)) { // Obstacle detected by the sweep
      stop();
      mov_mode = STOP;

      // Perform 3-point wide scan (30, 90, 150) to make escape decision
      for (int i = 1; i < 6;
           i +=
           2) // 1, 3, 5 Omnidirectional detection of obstacle avoidance status
      {
        ServoControl(30 * i);
        get_Distance = getDistance();
        delays(200);
        if (function_xxx(get_Distance, 0, 10)) { // 10cm for backing up earlier
          switc_ctrl = 10;
          break;
        } else if (function_xxx(get_Distance, 0,
                                25)) // How many cm in the front have obstacles?
                                     // (25cm safety)
        {
          switc_ctrl += i;
        }
      }
      ServoControl(90);
      current_angle = 90; // Reset sweep state to center
      sweep_state = 1;
    } else {
      forward(false, 135); // Control car forward (speed 135)
      mov_mode = FORWARD;
    }
    boolean escape_buzzer_active = false;
    if (switc_ctrl == 3 || switc_ctrl == 4 || switc_ctrl == 8 || switc_ctrl == 9 || switc_ctrl == 10 || switc_ctrl == 11) {
      Serial.print("{\"event\":\"buzzer_on\"}");
      escape_buzzer_active = true;
    }
    while (switc_ctrl) {
      switch (switc_ctrl) {
      case 1:
      case 5:
      case 6:
        forward(false, 135); // Control car forward (speed 135)
        mov_mode = FORWARD;
        switc_ctrl = 0;
        break;
      case 3:
        left(false, 160); // Control car left (turn speed 160)
        mov_mode = LEFT;
        switc_ctrl = 0;
        break;
      case 4:
        left(false, 160); // Control car left (turn speed 160)
        mov_mode = LEFT;
        switc_ctrl = 0;
        break;
      case 8:
      case 11:
        right(false, 140); // Control car right (turn speed 140)
        mov_mode = RIGHT;
        switc_ctrl = 0;
        break;
      case 9:
      case 10:
        back(
            false,
            140); // Control car backwards (high torque speed 180 for reversing)
        mov_mode = BACK;
        switc_ctrl = 11;
        break;
      }
      ServoControl(90);
      current_angle = 90; // Reset sweep state to center
      sweep_state = 1;
    }
    if (escape_buzzer_active) {
      stop();
      mov_mode = STOP;
      Serial.print("{\"event\":\"buzzer_off\"}");
    }
  } else {
    first_is = true;
  }
}

/*
  Hand Tracking Mode — Phase 2 (State Machine v3)
  TRACK_DISTANCE → LOST_WAIT → SEARCH_HAND → ALIGN_HEADING → TRACK_DISTANCE

  Fixes v3:
  - Servo settle 500ms cố định (không quá nhanh)
  - Lost counter: cần 10 lần đọc liên tục mất tay mới chuyển LOST_WAIT
  - LOST_WAIT có rate limiting tránh block serial
  - ALIGN đơn giản: tìm tay → servo về 90° → xe quay → tay xuất hiện ở 90° →
  done
*/
void hand_tracking_mode(void) {
  // ── Phase 1 variables ──
  static float ema_distance = 0;
  static int ht_current_speed = 0;
  static unsigned long ht_last_update = 0;
  static boolean ht_first_entry = true;
  static int ht_active_direction = 0;
  static uint8_t ht_change_counter = 0;
  static int ht_last_moving_dir = 0;

  // ── Phase 2 variables ──
  static HT_STATE ht_state = HT_TRACK_DISTANCE;
  static uint8_t ht_lost_count = 0;
  static unsigned long ht_lost_start = 0;
  // Search
  static uint8_t ht_last_hand_angle = 90;
  static uint8_t scan_angles[HT_SCAN_COUNT];
  static uint8_t scan_distances[HT_SCAN_COUNT];
  static uint8_t ht_scan_index = 0;
  static uint8_t ht_scan_phase = 0;
  static unsigned long ht_scan_timer = 0;
  // Align
  static unsigned long ht_align_start = 0;
  static uint8_t lost_debounce_counter = 0;

  // ── Exit mode ──
  if (func_mode != HandTracking) {
    if (!ht_first_entry) {
      stop();
      servo.detach();
    }
    ht_first_entry = true;
    ht_state = HT_TRACK_DISTANCE;
    return;
  }

  // ── First entry ──
  if (ht_first_entry) {
    servoWriteNonBlocking(90);
    delays(500);
    ema_distance = (float)getDistance();
    CMD_Distance = (int)ema_distance;
    ht_current_speed = 0;
    ht_active_direction = 0;
    ht_change_counter = 0;
    ht_last_moving_dir = 0;
    ht_state = HT_TRACK_DISTANCE;
    ht_last_hand_angle = 90;
    ht_lost_count = 0;
    lost_debounce_counter = 0;
    stop();
    ht_first_entry = false;
  }

  // ══════════════════════════════════════
  //           STATE MACHINE
  // ══════════════════════════════════════
  switch (ht_state) {

  // ── STATE: TRACK_DISTANCE ──
  case HT_TRACK_DISTANCE: {
    // Traffic light check: red light = force stop
    if (!traffic_light_green) {
      stop();
      mov_mode = STOP;
      ht_current_speed = 0;
      return;
    }
    if (millis() - ht_last_update < HT_UPDATE_INTERVAL)
      return;
    ht_last_update = millis();

    int raw_dist = getDistance();
    CMD_Distance = raw_dist;
    int desired_direction = 0;
    int target_speed = 0;

    // Mất tay: đếm liên tục, cần HT_LOST_COUNT lần mới chuyển state
    if (raw_dist == 0 || raw_dist > HT_MAX_RANGE) {
      lost_debounce_counter++;
      if (lost_debounce_counter >=
          3) { // Chỉ coi là mất tay thật sự nếu đọc sai 3 lần liên tiếp (150ms)
        ht_lost_count++;
        ema_distance = HT_TARGET_DIST; // Reset EMA tránh bị kéo
        desired_direction = 0;
        target_speed = 0;

        if (ht_lost_count >= HT_LOST_COUNT) {
          stop();
          ht_current_speed = 0;
          ht_active_direction = 0;
          mov_mode = STOP;
          ht_lost_count = 0;
          ht_lost_start = millis();
          ht_state = HT_LOST_WAIT;
          return;
        }
      } else {
        // Giữ nguyên khoảng cách cũ để lọc nhiễu tức thời từ động cơ khi đang
        // chạy
        raw_dist = (int)ema_distance;
      }
    } else {
      lost_debounce_counter = 0;
      ht_lost_count = 0; // Reset counter khi tay còn
      ema_distance =
          HT_EMA_ALPHA * (float)raw_dist + (1.0 - HT_EMA_ALPHA) * ema_distance;
    }
    int dist = (int)ema_distance;

    // Bù trễ động & Hysteresis
    int lower_bound = HT_TARGET_DIST - HT_DEADZONE;
    int upper_bound = HT_TARGET_DIST + HT_DEADZONE;
    if (ht_active_direction == 2)
      lower_bound += 2;
    else if (ht_active_direction == 1)
      upper_bound -= 2;
    if (ht_active_direction == 0 && ht_last_moving_dir == 2)
      lower_bound -= HT_HYSTERESIS;
    else if (ht_active_direction == 0 && ht_last_moving_dir == 1)
      upper_bound += HT_HYSTERESIS;

    // Tính hướng
    if (raw_dist == 0 || raw_dist > HT_MAX_RANGE) {
      // Đang trong giai đoạn đếm lost — dừng xe
      desired_direction = 0;
      target_speed = 0;
    } else if (dist >= lower_bound && dist <= upper_bound) {
      desired_direction = 0;
      target_speed = 0;
    } else if (dist < HT_SAFE_MIN) {
      desired_direction = 2;
      target_speed = 140;
    } else if (dist < lower_bound) {
      desired_direction = 2;
      int error = lower_bound - dist;
      int range = lower_bound - HT_SAFE_MIN;
      target_speed =
          (range > 0) ? map(error, 0, range, HT_SPEED_MIN, 140) : 140;
    } else {
      desired_direction = 1;
      int error = dist - upper_bound;
      int range = HT_MAX_RANGE - upper_bound;
      target_speed = (range > 0)
                         ? map(error, 0, range, HT_SPEED_MIN, HT_SPEED_MAX)
                         : HT_SPEED_MIN;
    }
    target_speed = constrain(target_speed, 0, HT_SPEED_MAX);

    // Direction logic
    if (desired_direction == ht_active_direction) {
      ht_change_counter = 0;
    } else if (desired_direction == 0) {
      if (ht_active_direction != 0)
        ht_last_moving_dir = ht_active_direction;
      ht_active_direction = 0;
      ht_change_counter = 0;
    } else if (ht_active_direction == 0) {
      ht_active_direction = desired_direction;
      ht_change_counter = 0;
      ht_last_moving_dir = 0;
    } else {
      ht_change_counter++;
      if (ht_change_counter >= HT_STABLE_COUNT) {
        ht_active_direction = desired_direction;
        ht_change_counter = 0;
        ht_last_moving_dir = 0;
      } else {
        target_speed = 0;
      }
    }

    // Ramping
    if (target_speed > ht_current_speed)
      ht_current_speed = min(ht_current_speed + HT_RAMP_STEP, target_speed);
    else if (target_speed == 0)
      ht_current_speed = 0;
    else
      ht_current_speed =
          max(ht_current_speed - (HT_RAMP_STEP * 2), target_speed);

    // Execute
    if (ht_active_direction == 0) {
      stop();
      mov_mode = STOP;
      ht_current_speed = 0;
    } else if (ht_active_direction == 1) {
      forward(false, ht_current_speed);
      mov_mode = FORWARD;
    } else {
      back(false, ht_current_speed);
      mov_mode = BACK;
    }
    break;
  }

  // ── STATE: LOST_WAIT (chờ thêm trước khi scan, có rate limit) ──
  case HT_LOST_WAIT: {
    stop();
    mov_mode = STOP;
    // Rate limit — dùng HT_UPDATE_INTERVAL để không spam getDistance()
    if (millis() - ht_last_update < HT_UPDATE_INTERVAL)
      return;
    ht_last_update = millis();

    int check = getDistance();
    CMD_Distance = check;
    if (check > 0 && check <= HT_MAX_RANGE) {
      // Tay quay lại → về TRACK
      ema_distance = (float)check;
      ht_lost_count = 0;
      ht_state = HT_TRACK_DISTANCE;
      return;
    }
    // Chờ thêm 300ms sau lost counter rồi mới SEARCH
    if (millis() - ht_lost_start >= 300) {
      static const int8_t offsets[HT_SCAN_COUNT] = {0,  -30, 30, -45,
                                                    45, -60, 60};
      for (uint8_t i = 0; i < HT_SCAN_COUNT; i++) {
        int a = (int)ht_last_hand_angle + offsets[i];
        scan_angles[i] = constrain(a, 5, 175);
      }
      ht_scan_index = 0;
      ht_scan_phase = 0;
      ht_state = HT_SEARCH_HAND;
    }
    break;
  }

  // ── STATE: SEARCH_HAND (quét servo 500ms/góc, scan hết chọn best) ──
  case HT_SEARCH_HAND: {
    stop();
    mov_mode = STOP;

    switch (ht_scan_phase) {
    case 0: // MOVE servo đến góc scan
      servoWriteNonBlocking(scan_angles[ht_scan_index]);
      ht_scan_timer = millis();
      ht_scan_phase = 1;
      break;

    case 1: // SETTLE — chờ 500ms
      if (millis() - ht_scan_timer >= HT_SERVO_SETTLE)
        ht_scan_phase = 2;
      break;

    case 2: { // READ
      int d = getDistance();
      CMD_Distance = d;
      scan_distances[ht_scan_index] = (d > 250) ? 250 : d;
      ht_scan_index++;
      if (ht_scan_index >= HT_SCAN_COUNT)
        ht_scan_phase = 3; // EVALUATE
      else
        ht_scan_phase = 0;
      break;
    }

    case 3: { // EVALUATE — chọn góc có vật cản gần nhất
      uint8_t best_idx = 255;
      uint8_t min_dist = 255;
      for (uint8_t i = 0; i < HT_SCAN_COUNT; i++) {
        uint8_t d = scan_distances[i];
        if (d > 0 && d < HT_MAX_RANGE) {
          if (d < min_dist) {
            min_dist = d;
            best_idx = i;
          }
        }
      }
      if (best_idx != 255) {
        uint8_t found_angle = scan_angles[best_idx];
        ht_last_hand_angle = found_angle;

        // Tay ngay trước mặt (85°-95°) → TRACK luôn
        if (found_angle >= 85 && found_angle <= 95) {
          servoWriteNonBlocking(90);
          safe_delay(500);
          int d = getDistance();
          CMD_Distance = d;
          ema_distance = (float)d;
          ht_current_speed = 0;
          ht_active_direction = 0;
          ht_change_counter = 0;
          ht_last_moving_dir = 0;
          ht_lost_count = 0;
          ht_state = HT_TRACK_DISTANCE;
        } else {
          // Tay lệch → Xoay servo về đúng hướng tay trước, rồi quay thân xe
          int angle_error = abs((int)found_angle - 90);

          // ★ FIX: Di chuyển servo về found_angle TRƯỚC khi quay thân xe
          servoWriteNonBlocking(found_angle);
          safe_delay(HT_SERVO_SETTLE); // Chờ servo ổn định ở hướng tay

          // Hệ số ms/độ riêng cho mỗi hướng (motor trái/phải khác nhau)
          unsigned long turn_duration;
          if (found_angle < 90) {
            // Tay bên PHẢI → servo chỉ sang PHẢI, xe quay PHẢI
            turn_duration =
                angle_error * 8; // Quay phải cần nhiều thời gian hơn
            right(false, HT_ALIGN_SPEED);
            mov_mode = RIGHT;
          } else {
            // Tay bên TRÁI → servo chỉ sang TRÁI, xe quay TRÁI
            turn_duration = angle_error * 12;
            left(false, HT_ALIGN_SPEED);
            mov_mode = LEFT;
          }

          // Xoay thân xe trong turn_duration
          safe_delay(turn_duration);
          stop();
          mov_mode = STOP;

          // Sau khi xe quay xong → servo về 90° (thẳng trước mặt)
          servoWriteNonBlocking(90);
          safe_delay(500); // Chờ servo về chính giữa xong

          ht_last_hand_angle =
              90; // Reset góc về 90 độ vì thân xe đã quay theo tay

          // Chuyển sang ALIGN_HEADING để kiểm tra lần cuối
          ht_align_start = millis();
          ht_state = HT_ALIGN_HEADING;
        }
      } else {
        // Không thấy → servo về 90°, chờ rồi scan lại
        servoWriteNonBlocking(90);
        ht_scan_timer = millis();
        ht_scan_phase = 4;
      }
      break;
    }

    case 4: // WAIT trước khi scan lại
      if (millis() - ht_scan_timer >= 500) {
        ht_scan_index = 0;
        ht_scan_phase = 0;
      }
      break;
    }
    break;
  }

  // ── STATE: ALIGN_HEADING ──
  // Đọc khoảng cách ở servo=90° xem tay đã ở trước mặt chưa
  case HT_ALIGN_HEADING: {
    static uint8_t align_retry_count = 0;
    int d = getDistance();
    CMD_Distance = d;

    if (d > 0 && d <= HT_MAX_RANGE) {
      align_retry_count = 0;
      // Tay đã ở chính giữa → căn xong!
      ema_distance = (float)d;
      ht_current_speed = 0;
      ht_active_direction = 0;
      ht_change_counter = 0;
      ht_last_moving_dir = 0;
      ht_lost_count = 0;
      ht_last_hand_angle = 90;
      ht_state = HT_TRACK_DISTANCE;
    } else {
      align_retry_count++;
      if (align_retry_count >= 3) {
        align_retry_count = 0;
        // Không thấy sau 3 lần → quay lại SEARCH_HAND
        static const int8_t offsets[HT_SCAN_COUNT] = {0,  -30, 30, -45,
                                                      45, -60, 60};
        for (uint8_t i = 0; i < HT_SCAN_COUNT; i++) {
          int a = (int)ht_last_hand_angle + offsets[i];
          scan_angles[i] = constrain(a, 5, 175);
        }
        ht_scan_index = 0;
        ht_scan_phase = 0;
        ht_state = HT_SEARCH_HAND;
      } else {
        delays(50); // Đợi 50ms trước khi thử lại
      }
    }
    break;
  }

  } // end switch
}

/*****************************************************Begin@CMD**************************************************************************************/

/*
  N21:command
  CMD mode：Ultrasonic module：App controls module status, module sends data to
  app
*/
void CMD_UltrasoundModuleStatus_Plus(
    uint8_t is_get) // Ultrasonic module processing
{
  // uint16_t
  CMD_Distance = getDistance(); // Ultrasonic module measuring distance

  if (1 == is_get) // is_get Start  true：Obstacles / false:No obstacles
  {
    if (CMD_Distance <= 50) {
      Serial.print('{' + CommandSerialNumber + "_true}");
    } else {
      Serial.print('{' + CommandSerialNumber + "_false}");
    }
  } else if (2 == is_get) // Ultrasonic is_get data
  {
    char toString[10];
    sprintf(toString, "%d", CMD_Distance);
    // Serial.print(toString);
    Serial.print('{' + CommandSerialNumber + '_' + toString + '}');
  }
}
/*
  N22:command
   CMD mode：Teacking module：App controls module status, module sends data to
  app
*/
void CMD_TraceModuleStatus_Plus(uint8_t is_get) // Tracking module processing
{
  if (0 == is_get) /*Get traces on the left*/
  {
    if (LineTeacking_Read_Left) {
      // Serial.print("{true}");
      Serial.print('{' + CommandSerialNumber + "_true}");
    } else {
      // Serial.print("{false}");
      Serial.print('{' + CommandSerialNumber + "_false}");
    }
  } else if (1 == is_get) /*Get traces on the middle*/
  {
    if (LineTeacking_Read_Middle) {
      // Serial.print("{true}");
      Serial.print('{' + CommandSerialNumber + "_true}");
    } else {
      // Serial.print("{false}");
      Serial.print('{' + CommandSerialNumber + "_false}");
    }
  } else if (2 == is_get) { /*Get traces on the right*/

    if (LineTeacking_Read_Right) {
      // Serial.print("{true}");
      Serial.print('{' + CommandSerialNumber + "_true}");
    } else {
      // Serial.print("{false}");
      Serial.print('{' + CommandSerialNumber + "_false}");
    }
  }
}

/*
  N1:command
  CMD mode：Sport mode <motor control> Control motor by app
  Input：uint8_t is_MotorSelection,  Motor selection   1：left  2：right  0：all
        uint8_t is_MotorDirection,   Motor steering  1：Forward  2：Reverse
  0：stop uint8_t is_MotorSpeed,       Motor speed   0-250
*/
void CMD_MotorControl_Plus(uint8_t is_MotorSelection, uint8_t is_MotorDirection,
                           uint8_t is_MotorSpeed) {
  static boolean MotorControl = false;

  if (func_mode == CMD_MotorControl) // Motor control mode
  {
    MotorControl = true;
    if (is_MotorSelection == 1 || is_MotorSelection == 0) // Left motor
    {
      if (is_MotorDirection == 1) // Positive rotation
      {
        analogWrite(ENA, is_MotorSpeed);
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
      } else if (is_MotorDirection == 2) // Reverse
      {
        analogWrite(ENA, is_MotorSpeed);
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
      } else if (is_MotorDirection == 0) {
        digitalWrite(ENA, LOW); // Turn off the motor enable pin
      }
    }
    if (is_MotorSelection == 2 || is_MotorSelection == 0) // Right motor
    {
      if (is_MotorDirection == 1) // Positive rotation
      {
        analogWrite(ENB, is_MotorSpeed);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
      } else if (is_MotorDirection == 2) // Reverse
      {
        analogWrite(ENB, is_MotorSpeed);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
      } else if (is_MotorDirection == 0) {
        digitalWrite(ENB, LOW); // Turn off the motor enable pin
      }
    }
  } else {
    if (MotorControl == true) {
      MotorControl = false;
      digitalWrite(ENA, LOW); // Turn off the motor enable pin
      digitalWrite(ENB, LOW);
    }
  }
}

/*
  N4：command
  CMD mode：<Car control> APP control car
  Time limited
*/
void CMD_CarControl_Plus(uint8_t is_CarDirection, uint8_t is_CarSpeed,
                         uint8_t is_Timer) {
  static boolean CarControl = false;
  static boolean CarControl_TE = false; // Have time to spend
  static boolean CarControl_return = false;
  if (func_mode == CMD_CarControl) // Car Control Mode
  {
    CarControl = true;
    if (is_Timer != 0) // Setting time cannot be empty
    {
      if ((millis() - CMD_CarControl_Millis) >
          (is_Timer * 1000)) // check the time
      {
        CarControl_TE = true;
        digitalWrite(ENA, LOW); // Turn off the motor enable pin
        digitalWrite(ENB, LOW);
        if (CarControl_return == false) {
          Serial.print('{' + CommandSerialNumber + "_ok}");
          delay(1);
          Serial.print('{' + CommandSerialNumber + "_ok}");
          CarControl_return = true;
        }
      } else {
        CarControl_TE = false; // Have time to spend
        CarControl_return = false;
      }
    }
    if (CarControl_TE == false) {
      switch (is_CarDirection) {
      case 1: /*Left-Forward Motion Mode*/
        left(false, is_CarSpeed);
        break;
      case 2: /*Right-Forward Motion Mode*/
        right(false, is_CarSpeed);
        break;
      case 3: /*Sport mode forward*/
        forward(false, is_CarSpeed);
        break;
      case 4: /*Sport mode back*/
        back(false, is_CarSpeed);
        break;
      default:
        break;
      }
    }
  } else {
    if (CarControl == true) {
      CarControl_return = false;
      CarControl = false;
      digitalWrite(ENA, LOW); // Turn off the motor enable pin
      digitalWrite(ENB, LOW);
      CMD_CarControl_Millis = 0;
    }
  }
}

/*
  N40：command
  CMD mode：<Car control> APP control car
  No time limit
*/
void CMD_CarControl_Plusxxx(uint8_t is_CarDirection, uint8_t is_CarSpeed) {
  static boolean CarControl = false;
  if (func_mode == CMD_CarControlxxx) // Car Control Mode
  {
    CarControl = true;
    switch (is_CarDirection) {
    case 1: /*Left-Forward Motion Mode*/
      left(false, is_CarSpeed);
      break;
    case 2: /*Right-Forward Motion Mode*/
      right(false, is_CarSpeed);
      break;
    case 3: /*Sport mode forward*/
      forward(false, is_CarSpeed);
      break;
    case 4: /*Sport mode back*/
      back(false, is_CarSpeed);
      break;
    default:
      break;
    }
  } else {
    if (CarControl == true) {
      CarControl = false;
      digitalWrite(ENA, LOW); // Turn off the motor enable pin
      digitalWrite(ENB, LOW);
    }
  }
}

/*
  N5:command
  CMD mode：
*/
void CMD_ClearAllFunctionsXXX(void) {
  if (func_mode == CMD_ClearAllFunctions) {

    mov_mode = STOP;
    func_mode = IDLE;
    digitalWrite(ENA, LOW); // Turn off the motor enable pin
    digitalWrite(ENB, LOW);

    /*CMD_MotorControl:Motor Control： Motor Speed、Motor Direction、Motor
     * Time*/
    CMD_MotorSelection = NULL;
    CMD_MotorDirection = NULL;

    CMD_MotorSpeed = NULL;
    CMD_leftMotorControl_Millis = NULL;
    CMD_rightMotorControl_Millis = NULL;

    /*CMD_CarControl:Car Control：Car moving direction、Car Speed、Car moving
     * time*/
    CMD_CarDirection = NULL;
    CMD_CarSpeed = NULL;
    CMD_CarTimer = NULL;
    CMD_CarControl_Millis = NULL;
  }
}

void getDistance_xx(void) {
  CMD_Distance = getDistance(); // Ultrasonic measurement distance
}

/*
  N100:command
  CMD mode: Telemetry — Send full status JSON to app
  Response: {"d":distance,"s":speed,"dir":direction,"m":mode}
*/
void CMD_Telemetry_Plus(void) {
  uint16_t dist = CMD_Distance;
  // Get current direction as number
  uint8_t dir = 0; // 0=stop
  switch (mov_mode) {
  case LEFT:
    dir = 1;
    break;
  case RIGHT:
    dir = 2;
    break;
  case FORWARD:
    dir = 3;
    break;
  case BACK:
    dir = 4;
    break;
  case STOP:
    dir = 0;
    break;
  case LEFT_FORWARD:
    dir = 6;
    break;
  case LEFT_BACK:
    dir = 7;
    break;
  case RIGHT_FORWARD:
    dir = 8;
    break;
  case RIGHT_BACK:
    dir = 9;
    break;
  default:
    dir = 0;
    break;
  }
  // Get current mode as number
  uint8_t mode = 0; // 0=idle
  switch (func_mode) {
  case IDLE:
    mode = 0;
    break;
  case LineTeacking:
    mode = 1;
    break;
  case ObstaclesAvoidance:
    mode = 2;
    break;
  case Bluetooth:
    mode = 3;
    break;
  case IRremote:
    mode = 4;
    break;
  case HandTracking:
    mode = 6;
    break;
  default:
    mode = 5;
    break;
  }
  // Send telemetry as JSON
  char buf[80];
  sprintf(buf, "{\"d\":%d,\"s\":%d,\"dir\":%d,\"m\":%d,\"tl\":%d}", dist,
          carSpeed_rocker, dir, mode, traffic_light_green ? 1 : 0);
  Serial.print(buf);
}

/*****************************************************End@CMD**************************************************************************************/
/*
  Bluetooth serial port data acquisition and control command parsing
*/
// #include "hardwareSerial.h"
void getBTData_Plus(void) {
  static char SerialPortData[128];
  static uint8_t rx_index = 0;
  static bool in_frame = false; // true khi đã nhận được '{'

  while (Serial.available() > 0) {
    char c = Serial.read();

    // Chỉ bắt đầu buffering khi gặp '{', bỏ qua tất cả ký tự rác (\n, khoảng
    // trắng, v.v.)
    if (!in_frame) {
      if (c == '{') {
        in_frame = true;
        rx_index = 0;
        SerialPortData[0] = c;
        rx_index = 1;
        SerialPortData[1] = '\0';
      }
      // Bỏ qua mọi ký tự khác khi chưa có '{'
      continue;
    }

    // Đang trong frame JSON
    if (rx_index < 127) {
      SerialPortData[rx_index++] = c;
      SerialPortData[rx_index] = '\0';
    } else {
      // Buffer tràn, reset
      rx_index = 0;
      in_frame = false;
      continue;
    }

    if (c == '}') {
      // Frame hoàn chỉnh — parse JSON
      in_frame = false;
      StaticJsonDocument<150> doc;
      DeserializationError error =
          deserializeJson(doc, (const char *)SerialPortData);

      // Reset buffer
      rx_index = 0;
      SerialPortData[0] = '\0';

      if (!error) {
        int control_mode_N = doc["N"];
        char buf[8];
        uint8_t temp = doc["H"];
        sprintf(buf, "%d", temp);
        CommandSerialNumber = buf;

        switch (control_mode_N) {
        case 1: {
          Serial_mode = Serial_programming;
          func_mode = CMD_MotorControl;
          CMD_MotorSelection = doc["D1"];
          CMD_MotorDirection = doc["D2"];
          CMD_MotorSpeed = doc["D3"];
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } break;
        case 2: {
          Serial_mode = Serial_rocker;
          int SpeedRocker = doc["D2"];
          if (SpeedRocker != 0) {
            carSpeed_rocker = SpeedRocker;
          }
          int d1 = doc["D1"];
          if (d1 >= 1 && d1 <= 9) {
            func_mode = Bluetooth;
            switch (d1) {
            case 1:
              mov_mode = LEFT;
              break;
            case 2:
              mov_mode = RIGHT;
              break;
            case 3:
              mov_mode = FORWARD;
              break;
            case 4:
              mov_mode = BACK;
              break;
            case 5:
              mov_mode = STOP;
              break;
            case 6:
              mov_mode = LEFT_FORWARD;
              break;
            case 7:
              mov_mode = LEFT_BACK;
              break;
            case 8:
              mov_mode = RIGHT_FORWARD;
              break;
            case 9:
              mov_mode = RIGHT_BACK;
              break;
            }
          }
        } break;
        case 3: {
          Serial_mode = Serial_rocker;
          int d1 = doc["D1"];
          if (d1 == 1) {
            func_mode = LineTeacking;
          } else if (d1 == 2) {
            func_mode = ObstaclesAvoidance;
          } else if (d1 == 3) {
            func_mode = HandTracking;
          }
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } break;
        case 4: {
          Serial_mode = Serial_programming;
          func_mode = CMD_CarControl;
          CMD_CarDirection = doc["D1"];
          CMD_CarSpeed = doc["D2"];
          CMD_CarTimer = doc["T"];
          CMD_CarControl_Millis = millis();
        } break;
        case 5: {
          func_mode = CMD_ClearAllFunctions;
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } break;
        case 6: {
          uint8_t angleSetting = doc["D1"];
          ServoControl(angleSetting);
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } break;
        case 21: {
          Serial_mode = Serial_programming;
          CMD_UltrasoundModuleStatus_Plus(doc["D1"]);
        } break;
        case 22: {
          Serial_mode = Serial_programming;
          CMD_TraceModuleStatus_Plus(doc["D1"]);
        } break;
        case 40: {
          Serial_mode = Serial_programming;
          func_mode = CMD_CarControlxxx;
          CMD_CarDirectionxxx = doc["D1"];
          CMD_CarSpeedxxx = doc["D2"];
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } break;
        case 50: { // Traffic Light control from Raspberry Pi
          int d1 = doc["D1"];
          traffic_light_green = (d1 == 1); // D1=1: green (go), D1=0: red (stop)
          if (!traffic_light_green) {
            stop();
            mov_mode = STOP;
          }
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } break;
        case 100: {
          CMD_Telemetry_Plus();
        } break;
        default:
          break;
        }
      }
    }
  }
}
void setup(void) {
  Serial.begin(115200); // USB Serial to Raspberry Pi
  ServoControl(90);
  irrecv.enableIRIn(); // Enable infrared communication NEC

  pinMode(ECHO_PIN, INPUT); // Ultrasonic module initialization
  pinMode(TRIG_PIN, OUTPUT);

  pinMode(IN1, OUTPUT); // Motor-driven port configuration
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(LineTeacking_Pin_Right,
          INPUT); // Infrared tracking module port configuration
  pinMode(LineTeacking_Pin_Middle, INPUT);
  pinMode(LineTeacking_Pin_Left, INPUT);
}

void loop(void) {
  getBTData_Plus();           // Serial/Bluetooth data acquisition
  getIRData();                // Infrared data acquisition
  bluetooth_mode();           // Bluetooth remote mode
  irremote_mode();            // Infrared NEC remote control mode
  line_teacking_mode();       // Line Teacking Mode
  obstacles_avoidance_mode(); // Obstacles Avoidance Mode
  hand_tracking_mode();       // Hand Tracking Mode (Phase 1)

  // Throttle distance reads to avoid blocking serial (pulseIn is slow)
  if (func_mode != HandTracking) {
    if (millis() - lastDistanceMillis >= DISTANCE_INTERVAL) {
      CMD_Distance = getDistance();
      lastDistanceMillis = millis();
    }
  }
  /*CMD_MotorControl: Motor Control： Motor Speed、Motor Direction、Motor Time*/
  CMD_MotorControl_Plus(CMD_MotorSelection, CMD_MotorDirection,
                        CMD_MotorSpeed); // Control motor steering
  /*  CMD mode：<Car control> APP control car*/
  CMD_CarControl_Plus(
      CMD_CarDirection, CMD_CarSpeed,
      CMD_CarTimer); // Control the direction of the car<Time limited>
  CMD_CarControl_Plusxxx(
      CMD_CarDirectionxxx,
      CMD_CarSpeedxxx); // Control the direction of the car<No Time limited>
  CMD_ClearAllFunctionsXXX();
}
