/*
 * IR Remote Code Scanner v3 - Majority Voting
 * ---------------------------------------------
 * Thu thap nhieu mau tu moi nut bam, phan tich va
 * chi hien thi ma xuat hien nhieu nhat (chinh xac nhat).
 * 
 * Cach su dung:
 *   1. Upload sketch nay len Arduino UNO
 *   2. Mo Serial Monitor (baud rate: 9600)
 *   3. Nhan va GIU 1 nut trong khoang 2-3 giay
 *   4. Tha nut, cho 2 giay -> ket qua se hien thi
 *   5. Lam tuong tu cho cac nut khac
 * 
 * IR Receiver pin: 12
 */

#include <IRremote.h>

#define RECV_PIN 12
#define MAX_SAMPLES 20       // So mau toi da thu thap
#define SILENCE_TIMEOUT 1500 // 1.5 giay im lang = ket thuc 1 nut

IRrecv irrecv(RECV_PIN);
decode_results results;

unsigned long samples[MAX_SAMPLES]; // Mang luu cac ma thu duoc
uint8_t sampleCount = 0;           // So mau da thu
unsigned long lastReceiveTime = 0;  // Thoi diem nhan tin hieu cuoi
bool collecting = false;            // Dang thu thap?
uint8_t buttonNumber = 0;           // So thu tu nut

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();

  Serial.println(F("===================================="));
  Serial.println(F("  IR Remote Code Scanner v3"));
  Serial.println(F("  (Majority Voting - Chinh xac)"));
  Serial.println(F("===================================="));
  Serial.println(F("Huong dan:"));
  Serial.println(F("  1. Nhan va GIU 1 nut 2-3 giay"));
  Serial.println(F("  2. Tha nut, cho 2 giay"));
  Serial.println(F("  3. Ket qua chinh xac se hien thi"));
  Serial.println(F("  4. Lam tuong tu cho nut tiep theo"));
  Serial.println(F("------------------------------------"));
  Serial.println();
}

// Tim ma xuat hien nhieu nhat trong mang samples
unsigned long findMostFrequent(uint8_t *outCount) {
  unsigned long bestCode = 0;
  uint8_t bestCount = 0;

  for (uint8_t i = 0; i < sampleCount; i++) {
    uint8_t count = 0;
    for (uint8_t j = 0; j < sampleCount; j++) {
      if (samples[j] == samples[i]) {
        count++;
      }
    }
    if (count > bestCount) {
      bestCount = count;
      bestCode = samples[i];
    }
  }

  *outCount = bestCount;
  return bestCode;
}

// Phan tich va in ket qua
void analyzeAndReport() {
  if (sampleCount == 0) return;

  buttonNumber++;

  Serial.print(F("\n=== NUT #"));
  Serial.print(buttonNumber);
  Serial.println(F(" ==="));
  Serial.print(F("Tong so mau thu duoc: "));
  Serial.println(sampleCount);

  // Tim ma chinh xac nhat
  uint8_t bestCount = 0;
  unsigned long bestCode = findMostFrequent(&bestCount);

  // Tinh % do tin cay
  uint8_t confidence = (uint8_t)((unsigned long)bestCount * 100 / sampleCount);

  Serial.println(F("------------------------------------"));
  Serial.print(F(">>> MA CHINH XAC:  "));
  Serial.println(bestCode);
  Serial.print(F(">>> MA HEX:        0x"));
  Serial.println(bestCode, HEX);
  Serial.print(F(">>> DO TIN CAY:    "));
  Serial.print(confidence);
  Serial.print(F("% ("));
  Serial.print(bestCount);
  Serial.print(F("/"));
  Serial.print(sampleCount);
  Serial.println(F(" lan)"));

  if (confidence >= 60) {
    Serial.println(F(">>> TRANG THAI:    *** DANG TIN CAY ***"));
  } else if (confidence >= 40) {
    Serial.println(F(">>> TRANG THAI:    ** TAM CHAP NHAN **"));
  } else {
    Serial.println(F(">>> TRANG THAI:    ! KHONG ON DINH - thu lai !"));
  }

  // Hien thi tat ca ma khac biet da nhan
  Serial.println(F("------------------------------------"));
  Serial.println(F("Chi tiet cac ma da nhan:"));
  
  // Dem va hien thi tung ma duy nhat
  for (uint8_t i = 0; i < sampleCount; i++) {
    // Kiem tra da hien thi ma nay chua
    bool alreadyShown = false;
    for (uint8_t j = 0; j < i; j++) {
      if (samples[j] == samples[i]) {
        alreadyShown = true;
        break;
      }
    }
    if (alreadyShown) continue;

    // Dem so lan xuat hien
    uint8_t count = 0;
    for (uint8_t j = 0; j < sampleCount; j++) {
      if (samples[j] == samples[i]) count++;
    }

    Serial.print(F("  "));
    Serial.print(samples[i]);
    Serial.print(F(" (0x"));
    Serial.print(samples[i], HEX);
    Serial.print(F(") x"));
    Serial.print(count);
    if (samples[i] == bestCode) {
      Serial.print(F("  <<< MA TOT NHAT"));
    }
    Serial.println();
  }

  Serial.println(F("====================================\n"));
  Serial.println(F(">> Nhan nut tiep theo..."));
  Serial.println();
}

void loop() {
  if (irrecv.decode(&results)) {
    unsigned long code = results.value;
    irrecv.resume();

    // Bo qua ma REPEAT va ma = 0
    if (code == 0xFFFFFFFF || code == 0) {
      lastReceiveTime = millis(); // Van cap nhat thoi gian
      return;
    }

    // Bat dau thu thap neu chua
    if (!collecting) {
      collecting = true;
      sampleCount = 0;
      Serial.println(F(">> Dang thu thap... (giu nut)"));
    }

    // Luu mau
    if (sampleCount < MAX_SAMPLES) {
      samples[sampleCount] = code;
      sampleCount++;
      Serial.print(F("   Mau #"));
      Serial.print(sampleCount);
      Serial.print(F(": "));
      Serial.println(code);
    }

    lastReceiveTime = millis();
    delay(100); // Ngan delay chong nhieu
  }

  // Kiem tra timeout - da tha nut
  if (collecting && (millis() - lastReceiveTime > SILENCE_TIMEOUT)) {
    collecting = false;
    analyzeAndReport();
    sampleCount = 0;
  }
}
