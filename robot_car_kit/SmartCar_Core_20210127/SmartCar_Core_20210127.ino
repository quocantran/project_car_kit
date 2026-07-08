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

/*══════════════════════════════════════════════════════════════
 * HAND TRACKING — Configurable Constants (dễ thay đổi)
 * Thay đổi các giá trị bên dưới để điều chỉnh hành vi theo dõi tay
 *══════════════════════════════════════════════════════════════*/
#define HT_TARGET_DIST      20    // Khoảng cách mục tiêu (cm) — xe giữ khoảng cách này với tay
#define HT_DEADZONE         4     // Vùng chết ±cm — trong vùng này xe đứng yên
#define HT_HYSTERESIS       3     // cm thêm khi xe đã dừng — tránh dao động (deadzone mở rộng = ±7cm)
#define HT_SAFE_MIN         10    // Khoảng cách tối thiểu an toàn (cm) — gần hơn sẽ lùi tốc độ max
#define HT_MAX_RANGE        50    // Phạm vi phát hiện tối đa (cm) — xa hơn sẽ dừng
#define HT_SPEED_MIN        80    // Tốc độ PWM tối thiểu (0-255) — thấp hơn = ít quá đà
#define HT_SPEED_MAX        180   // Tốc độ PWM tối đa (0-255)
#define HT_EMA_ALPHA        0.45  // Hệ số lọc EMA (0.0-1.0) — tăng lên để phản hồi nhanh hơn, giảm trễ
#define HT_RAMP_STEP        25    // Bước tăng/giảm tốc tối đa mỗi chu kỳ — tăng lên để tăng tốc nhanh hơn
#define HT_UPDATE_INTERVAL  50    // ms giữa mỗi lần đọc sensor — giảm xuống để cập nhật nhanh hơn
#define HT_STABLE_COUNT     3     // Số lần đọc liên tục trước khi đổi hướng — chống dao động

// Throttle getDistance() to avoid blocking serial reads
unsigned long lastDistanceMillis = 0;
#define DISTANCE_INTERVAL 500  // Only read distance every 500ms
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

void delays(unsigned long t) {
  for (unsigned long i = 0; i < t; i++) {
    getBTData_Plus(); // Bluetooth Communication Data Acquisition
    getIRData();      // Infrared Communication Data Acquisition
    delay(1);
  }
}
/*ULTRASONIC*/
unsigned int getDistance(void) { // Getting distance
  static unsigned int tempda = 0;
  unsigned int tempda_x = 0;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  // Use 30ms timeout instead of default 1s to avoid blocking serial reads
  tempda_x = ((unsigned int)pulseIn(ECHO_PIN, HIGH, 30000) / 58);
  if (tempda_x > 150) {
    tempda_x = 150;
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
  analogWrite(ENA, in_carSpeed);
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
  if (func_mode == ObstaclesAvoidance) {
    if (first_is == true) // Enter the mode for the first time, and modulate the
                          // steering gear to 90 degrees
    {
      ServoControl(90);
      first_is = false;
    }
    uint8_t get_Distance = getDistance();
    if (function_xxx(get_Distance, 0, 20)) {
      stop();
      /*
      ------------------------------------------------------------------------------------------------------
      ServoControl(30 * 1): 0 1 0 1 0 1 0 1
      ServoControl(30 * 3): 0 0 1 1 0 0 1 1
      ServoControl(30 * 5): 0 0 0 0 1 1 1 1
      1 2 4 >>>             0 1 2 3 4 5 6 7
      1 3 5 >>>             0 1 3 4 5 6 5 9
      ------------------------------------------------------------------------------------------------------
      Truth table of obstacle avoidance state
      */
      for (int i = 1; i < 6;
           i +=
           2) // 1、3、5 Omnidirectional detection of obstacle avoidance status
      {
        ServoControl(30 * i);
        get_Distance = getDistance();
        delays(200);
        if (function_xxx(get_Distance, 0, 5)) {
          switc_ctrl = 10;
          break;
        } else if (function_xxx(get_Distance, 0,
                                20)) // How many cm in the front have obstacles?
        {
          switc_ctrl += i;
        }
      }
      ServoControl(90);
    } else // if (function_xxx(get_Distance, 20, 50))
    {
      forward(false, 150); // Control car forwar
    }
    while (switc_ctrl) {
      switch (switc_ctrl) {
      case 1:
      case 5:
      case 6:
        forward(false, 150); // Control car forwar
        switc_ctrl = 0;
        break;
      case 3:
        left(false, 250); // Control car left
        switc_ctrl = 0;
        break;
      case 4:
        left(false, 250); // Control car left
        switc_ctrl = 0;
        break;
      case 8:
      case 11:
        right(false, 250); // Control car right
        switc_ctrl = 0;
        break;
      case 9:
      case 10:
        back(false, 150); // Control car Car backwards
        switc_ctrl = 11;
        break;
      }
      ServoControl(90);
    }
  } else {
    first_is = true;
  }
}

/*
  Hand Tracking Mode — Phase 1 (v3: fix deadlock)
  Xe tự động tiến/lùi để duy trì khoảng cách mục tiêu với bàn tay
  
  Cơ chế chống dao động:
  1. Hysteresis hướng: khi xe vừa dừng, mở rộng deadzone CHỈ về phía hướng cũ
  2. Stable count: CHỈ áp dụng khi ĐẢO HƯỚNG (forward↔back), KHÔNG block khởi động
  3. EMA mượt (alpha thấp) + ramp nhỏ: giảm quá đà do quán tính
*/
void hand_tracking_mode(void) {
  static float ema_distance = 0;
  static int ht_current_speed = 0;
  static unsigned long ht_last_update = 0;
  static boolean ht_first_entry = true;
  static int ht_active_direction = 0;   // Hướng đang thực thi: 0=stop, 1=forward, 2=back
  static uint8_t ht_change_counter = 0; // Đếm số lần đọc liên tục khi muốn ĐẢO hướng
  static int ht_last_moving_dir = 0;    // Hướng chạy cuối cùng trước khi dừng

  if (func_mode != HandTracking) {
    ht_first_entry = true;
    return;
  }

  // First entry: initialize servo to front-facing and prime EMA filter
  if (ht_first_entry) {
    ServoControl(90);
    ema_distance = (float)getDistance();
    ht_current_speed = 0;
    ht_active_direction = 0;
    ht_change_counter = 0;
    ht_last_moving_dir = 0;
    stop();
    ht_first_entry = false;
  }

  // Rate limiting
  if (millis() - ht_last_update < HT_UPDATE_INTERVAL) return;
  ht_last_update = millis();

  // Read raw distance and apply EMA filter
  int raw_dist = getDistance();
  
  int desired_direction = 0;
  int target_speed = 0;
  
  // ── Kiểm tra mất vật cản đột ngột (sensor trả về 0 hoặc vượt quá phạm vi) ──
  // Cảm biến trả về 0 khi không nhận được echo (ví dụ: rút tay ra quá nhanh hoặc xa)
  if (raw_dist == 0 || raw_dist > HT_MAX_RANGE) {
    desired_direction = 0;
    target_speed = 0;
    ema_distance = HT_TARGET_DIST; // Reset bộ lọc về 20 để dừng xe ngay, tránh bị kéo lùi
  } else {
    ema_distance = HT_EMA_ALPHA * (float)raw_dist + (1.0 - HT_EMA_ALPHA) * ema_distance;
  }
  int dist = (int)ema_distance;

  // ── Bù trễ động (Dynamic Lag Compensation) & Hysteresis ──
  int lower_bound = HT_TARGET_DIST - HT_DEADZONE;  // 16cm
  int upper_bound = HT_TARGET_DIST + HT_DEADZONE;  // 24cm

  // Bù trễ khi xe đang di chuyển: phanh sớm hơn để trừ hao trôi bánh và trễ cảm biến
  if (ht_active_direction == 2) {
    // Đang lùi: dừng sớm hơn (nâng ngưỡng dưới lên 18cm)
    lower_bound += 2;
  } else if (ht_active_direction == 1) {
    // Đang tiến: dừng sớm hơn (hạ ngưỡng trên xuống 22cm)
    upper_bound -= 2;
  }

  // Hysteresis khi xe đang dừng: chỉ mở rộng deadzone về phía hướng xe vừa chạy
  if (ht_active_direction == 0 && ht_last_moving_dir == 2) {
    lower_bound -= HT_HYSTERESIS;  // 13cm
  } else if (ht_active_direction == 0 && ht_last_moving_dir == 1) {
    upper_bound += HT_HYSTERESIS;  // 27cm
  }

  // ── Tính hướng mong muốn ──
  desired_direction = 0;
  target_speed = 0;

  if (raw_dist == 0 || raw_dist > HT_MAX_RANGE) {
    // Đã xử lý ở trên, giữ dừng
    desired_direction = 0;
    target_speed = 0;
  } else if (dist >= lower_bound && dist <= upper_bound) {
    // Trong deadzone — giữ nguyên
    desired_direction = 0;
    target_speed = 0;
  } else if (dist < HT_SAFE_MIN) {
    // Quá gần — lùi tốc độ max (nhưng lùi nhẹ hơn tiến)
    desired_direction = 2;
    target_speed = 140; // Giới hạn tốc độ lùi tối đa an toàn để giảm trượt đà
  } else if (dist < lower_bound) {
    // Tay tiến lại gần — lùi tỷ lệ
    desired_direction = 2;
    int error = lower_bound - dist;
    int range = lower_bound - HT_SAFE_MIN;
    if (range > 0) {
      target_speed = map(error, 0, range, HT_SPEED_MIN, 140);
    } else {
      target_speed = 140;
    }
  } else {
    // Tay rút xa — tiến theo tỷ lệ
    desired_direction = 1;
    int error = dist - upper_bound;
    int range = HT_MAX_RANGE - upper_bound;
    if (range > 0) {
      target_speed = map(error, 0, range, HT_SPEED_MIN, HT_SPEED_MAX);
    } else {
      target_speed = HT_SPEED_MIN;
    }
  }

  target_speed = constrain(target_speed, 0, HT_SPEED_MAX);

  // ── Direction logic ──
  if (desired_direction == ht_active_direction) {
    // Cùng hướng — chạy bình thường
    ht_change_counter = 0;
  } else if (desired_direction == 0) {
    // Muốn dừng — cho dừng ngay
    if (ht_active_direction != 0) {
      ht_last_moving_dir = ht_active_direction;  // Nhớ hướng cuối cho hysteresis
    }
    ht_active_direction = 0;
    ht_change_counter = 0;
  } else if (ht_active_direction == 0) {
    // KHỞI ĐỘNG từ dừng → cho phép ngay, KHÔNG cần chờ stable count
    ht_active_direction = desired_direction;
    ht_change_counter = 0;
    ht_last_moving_dir = 0;  // Xóa hysteresis sau khi khởi động
  } else {
    // ĐẢO HƯỚNG (forward↔back) — phải chờ stable count
    ht_change_counter++;
    if (ht_change_counter >= HT_STABLE_COUNT) {
      ht_active_direction = desired_direction;
      ht_change_counter = 0;
      ht_last_moving_dir = 0;
    } else {
      // Chưa đủ — buộc dừng trong khi chờ
      target_speed = 0;
    }
  }

  // ── Acceleration/Deceleration ramping ──
  if (target_speed > ht_current_speed) {
    // Tăng tốc mượt từ từ
    ht_current_speed = min(ht_current_speed + HT_RAMP_STEP, target_speed);
  } else {
    // Giảm tốc nhanh hơn (hoặc dừng ngay lập tức nếu target_speed = 0) để tránh trôi xe quá đà
    if (target_speed == 0) {
      ht_current_speed = 0;
    } else {
      ht_current_speed = max(ht_current_speed - (HT_RAMP_STEP * 2), target_speed);
    }
  }

  // ── Execute movement ──
  if (ht_active_direction == 0) {
    stop();
    mov_mode = STOP;
    ht_current_speed = 0;  // Chỉ reset khi THỰC SỰ muốn dừng
  } else if (ht_active_direction == 1) {
    forward(false, ht_current_speed);
    mov_mode = FORWARD;
  } else if (ht_active_direction == 2) {
    back(false, ht_current_speed);
    mov_mode = BACK;
  }
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
  uint16_t dist = getDistance();
  // Get current direction as number
  uint8_t dir = 0; // 0=stop
  switch (mov_mode) {
    case LEFT:          dir = 1; break;
    case RIGHT:         dir = 2; break;
    case FORWARD:       dir = 3; break;
    case BACK:          dir = 4; break;
    case STOP:          dir = 0; break;
    case LEFT_FORWARD:  dir = 6; break;
    case LEFT_BACK:     dir = 7; break;
    case RIGHT_FORWARD: dir = 8; break;
    case RIGHT_BACK:    dir = 9; break;
    default:            dir = 0; break;
  }
  // Get current mode as number
  uint8_t mode = 0; // 0=idle
  switch (func_mode) {
    case IDLE:               mode = 0; break;
    case LineTeacking:       mode = 1; break;
    case ObstaclesAvoidance: mode = 2; break;
    case Bluetooth:          mode = 3; break;
    case IRremote:           mode = 4; break;
    case HandTracking:       mode = 6; break;
    default:                 mode = 5; break;
  }
  // Send telemetry as JSON
  char buf[80];
  sprintf(buf, "{\"d\":%d,\"s\":%d,\"dir\":%d,\"m\":%d}",
          dist, carSpeed_rocker, dir, mode);
  Serial.print(buf);
}

/*****************************************************End@CMD**************************************************************************************/
/*
  Bluetooth serial port data acquisition and control command parsing
*/
// #include "hardwareSerial.h"
void getBTData_Plus(void) {
  static String SerialPortData = "";
  uint8_t c = 0;
  if (Serial.available() > 0) {
    while ((c != '}') &&
           Serial.available() >
               0) // Forcibly wait for a frame of data to finish receiving
    {
      // while (Serial.available() == 0)
      //   ;
      c = Serial.read();
      SerialPortData += (char)c;
    }
  }
  if (c == '}') {
    // Serial.println(SerialPortData);
    StaticJsonDocument<200> doc; // Create a JsonDocument object
    DeserializationError error =
        deserializeJson(doc, SerialPortData); // Deserialize JSON data
    SerialPortData = "";
    if (!error) // Check if deserialization is successful
    {
      int control_mode_N = doc["N"];
      char buf[8];  // Fixed: was buf[3], overflows when H >= 100 (e.g. "176\0" = 4 bytes)
      uint8_t temp = doc["H"];
      sprintf(buf, "%d", temp);
      CommandSerialNumber = buf; // Get the serial number of the new command
      // Debug echo: confirm Arduino received and parsed the command
      Serial.print("{\"dbg\":\"N=");
      Serial.print(control_mode_N);
      Serial.print(",H=");
      Serial.print(temp);
      Serial.println("\"}");
      switch (control_mode_N) {
      case 1: /*Motion module  processing <command：N 1>*/
      {
        Serial_mode = Serial_programming;
        func_mode = CMD_MotorControl;
        CMD_MotorSelection = doc["D1"];
        CMD_MotorDirection = doc["D2"];
        CMD_MotorSpeed = doc["D3"];
        Serial.print('{' + CommandSerialNumber + "_ok}");
      } break;
      case 2: /*Remote switching mode  processing <command：N 2>*/
      {
        Serial_mode = Serial_rocker;
        int SpeedRocker = doc["D2"];
        if (SpeedRocker != 0) {
          carSpeed_rocker = SpeedRocker;
        }
        if (1 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = LEFT;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (2 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = RIGHT;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (3 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = FORWARD;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (4 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = BACK;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (5 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = STOP;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (6 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = LEFT_FORWARD;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (7 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = LEFT_BACK;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (8 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = RIGHT_FORWARD;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (9 == doc["D1"]) {
          func_mode = Bluetooth;
          mov_mode = RIGHT_BACK;
          // Serial.print('{' + CommandSerialNumber + "_ok}");
        }
      } break;
      case 3: /*Remote switching mode  processing <command：N 3>*/
      {
        Serial_mode = Serial_rocker;
        if (1 == doc["D1"]) // Line Teacking Mode
        {
          func_mode = LineTeacking;
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (2 == doc["D1"]) // Obstacles Avoidance Mode
        {
          func_mode = ObstaclesAvoidance;
          Serial.print('{' + CommandSerialNumber + "_ok}");
        } else if (3 == doc["D1"]) // Hand Tracking Mode
        {
          func_mode = HandTracking;
          Serial.print('{' + CommandSerialNumber + "_ok}");
        }
      } break;
      case 4: /*Motion module  processing <command：N 4>*/
      {
        Serial_mode = Serial_programming;
        func_mode = CMD_CarControl;
        CMD_CarDirection = doc["D1"];
        CMD_CarSpeed = doc["D2"];
        CMD_CarTimer = doc["T"];
        CMD_CarControl_Millis = millis(); // Get the timestamp
        // Serial.print("{ok}");
      } break;
      case 5: /*Clear mode  processing <command：N 5>*/
      {
        func_mode = CMD_ClearAllFunctions;
        Serial.print('{' + CommandSerialNumber + "_ok}");
      }

      break;
      case 6: /*CMD mode：angle Setting*/
      {
        uint8_t angleSetting = doc["D1"];
        ServoControl(angleSetting);
        Serial.print('{' + CommandSerialNumber + "_ok}");
      }

      break;
      case 21: /*Ultrasonic module  processing <command：N 21>*/
      {
        Serial_mode = Serial_programming;
        CMD_UltrasoundModuleStatus_Plus(doc["D1"]);
      }

      break;
      case 22: /*Trace module data processing <command：N 22>*/
      {
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
      case 100: /*Telemetry query  processing <command: N 100>*/
      {
        CMD_Telemetry_Plus();
      } break;
      default:
        break;
      }
    }
  } else if (SerialPortData != "") {
    if (true == SerialPortData.equals("f")) {
      func_mode = CMD_CarControlxxx;
      CMD_CarDirectionxxx = 3;
      CMD_CarSpeedxxx = 180;
      SerialPortData = "";
    } else if (true == SerialPortData.equals("b")) {
      func_mode = CMD_CarControlxxx;
      CMD_CarDirectionxxx = 4;
      CMD_CarSpeedxxx = 180;
      SerialPortData = "";
    } else if (true == SerialPortData.equals("l")) {
      func_mode = CMD_CarControlxxx;
      CMD_CarDirectionxxx = 1;
      CMD_CarSpeedxxx = 180;
      SerialPortData = "";
    } else if (true == SerialPortData.equals("r")) {
      func_mode = CMD_CarControlxxx;
      CMD_CarDirectionxxx = 2;
      CMD_CarSpeedxxx = 180;
      SerialPortData = "";
    } else if (true == SerialPortData.equals("s")) {
      func_mode = Bluetooth;
      mov_mode = STOP;
      SerialPortData = "";
    } else if (true == SerialPortData.equals("1")) {
      func_mode = LineTeacking;
      SerialPortData = "";
    } else if (true == SerialPortData.equals("2")) {
      func_mode = ObstaclesAvoidance;
      SerialPortData = "";
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
  if (millis() - lastDistanceMillis >= DISTANCE_INTERVAL) {
    CMD_Distance = getDistance();
    lastDistanceMillis = millis();
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
