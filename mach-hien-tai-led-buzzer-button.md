# Mô tả mạch hiện tại LED và buzzer để vẽ sơ đồ

Tài liệu này mô tả mạch phần cứng hiện tại của hệ thống xe robot dùng Raspberry Pi GPIO Extension Board, breadboard, LED đỏ, LED xanh và buzzer 5V điều khiển qua transistor S8050.

Mục tiêu của file này là cung cấp thông tin đủ chi tiết để đưa cho GPT/công cụ AI khác vẽ lại sơ đồ mạch.

---

## 1. Bộ điều khiển chính

- Bộ điều khiển: Raspberry Pi 4.
- Raspberry Pi nối ra breadboard thông qua Raspberry Pi GPIO Extension Board / breakout board.
- Code backend dùng kiểu đánh số BCM GPIO.
- Cần phân biệt rõ:
  - BCM GPIO22 không phải chân vật lý số 22.
  - BCM GPIO22 tương ứng chân vật lý pin 15 trên header Raspberry Pi 40 pin.

---

## 2. Mapping chân GPIO theo code hiện tại

| Chức năng | Tên trong code | BCM GPIO | Chân vật lý Raspberry Pi | Ghi chú |
|---|---|---:|---:|---|
| LED đỏ | GPIO_LED_RED_PIN | GPIO17 | Pin 11 | Báo trạng thái đèn đỏ / STOP |
| LED xanh | GPIO_LED_GREEN_PIN | GPIO27 | Pin 13 | Báo trạng thái đèn xanh / GO |
| Buzzer | GPIO_BUZZER_PIN | GPIO22 | Pin 15 | Điều khiển buzzer qua transistor S8050 |
| Nguồn buzzer | 5V | 5V | Pin 2 hoặc pin 4 | Cấp dương cho buzzer 5V |
| Mass chung | GND | GND | Pin 6 / 9 / 14 / 20 / 25 / 30 / 34 / 39 | GND chung cho Pi, LED, transistor và buzzer |

> GPIO23 / physical pin 16 vẫn được khai báo cho button trong code, nhưng **không có dây nối và không thuộc mạch hiện tại**.

---

## 3. Sơ đồ tổng quan dạng text

```text
Raspberry Pi / GPIO Extension Board

GPIO17  ---- điện trở ---- anode LED đỏ
GND     ------------------ cathode LED đỏ

GPIO27  ---- điện trở ---- anode LED xanh
GND     ------------------ cathode LED xanh

GPIO22  ---- điện trở 1kΩ ---- Base S8050
GND     ----------------------- Emitter S8050
Collector S8050 --------------- cực âm buzzer
5V ---------------------------- cực dương buzzer

```

---

## 4. Mạch LED đỏ

### Linh kiện

- 1 LED đỏ.
- 1 điện trở hạn dòng, khuyến nghị 220Ω đến 1kΩ.
- Dây nối từ GPIO Extension Board.

### Kết nối

```text
GPIO17 / physical pin 11 ---- điện trở 220Ω-1kΩ ---- chân dài LED đỏ (+)
Chân ngắn LED đỏ (-) ------------------------------- GND Raspberry Pi
```

### Logic hoạt động

```text
GPIO17 HIGH -> LED đỏ sáng
GPIO17 LOW  -> LED đỏ tắt
```

---

## 5. Mạch LED xanh

### Linh kiện

- 1 LED xanh.
- 1 điện trở hạn dòng, khuyến nghị 220Ω đến 1kΩ.
- Dây nối từ GPIO Extension Board.

### Kết nối

```text
GPIO27 / physical pin 13 ---- điện trở 220Ω-1kΩ ---- chân dài LED xanh (+)
Chân ngắn LED xanh (-) ----------------------------- GND Raspberry Pi
```

### Logic hoạt động

```text
GPIO27 HIGH -> LED xanh sáng
GPIO27 LOW  -> LED xanh tắt
```

---

## 6. Mạch buzzer hiện tại qua transistor S8050

### Linh kiện

- 1 buzzer 2 chân, đã test kêu khi cấp 5V trực tiếp.
- 1 transistor S8050 loại NPN.
- 1 điện trở Base khoảng 1kΩ.
- Nguồn 5V từ Raspberry Pi hoặc nguồn 5V ngoài.
- GND chung với Raspberry Pi.

### Mục đích dùng transistor

Buzzer dùng 5V và có thể cần dòng lớn hơn khả năng cấp trực tiếp của GPIO Raspberry Pi. Vì vậy GPIO22 không cấp dòng trực tiếp cho buzzer mà chỉ điều khiển Base của transistor S8050. Transistor đóng vai trò công tắc phía mass / low-side switch.

---

## 7. Pinout S8050 cần dùng

Với S8050 vỏ TO-92 phổ biến:

```text
Nhìn vào mặt phẳng có chữ "S8050", ba chân hướng xuống dưới:

Trái   = Emitter (E)
Giữa   = Base    (B)
Phải   = Collector (C)
```

Biểu diễn:

```text
        Mặt phẳng có chữ S8050
       ┌─────────────────────┐
       │        S8050         │
       └─────────────────────┘
          |       |       |
          E       B       C
        trái    giữa    phải
```

Lưu ý: một số lô transistor S8050 có thể khác pinout. Nếu cắm theo E-B-C nhưng còi không kêu hoặc kêu yếu/chập chờn, cần kiểm tra datasheet đúng theo mã in trên transistor.

---

## 8. Đấu buzzer 5V qua S8050 theo mạch hiện tại

### Kết nối chuẩn

```text
Raspberry Pi / GPIO Extension Board       S8050 / Buzzer

GPIO22 / physical pin 15 ----[1kΩ]------ Base S8050
GND Raspberry Pi ------------------------ Emitter S8050
Collector S8050 ------------------------- cực âm buzzer (-)
5V Raspberry Pi ------------------------- cực dương buzzer (+)
```

### Dạng sơ đồ ASCII

```text
                 +5V Raspberry Pi
                       |
                       |
                  (+) Buzzer
                  (-) Buzzer
                       |
                       |
                 Collector
                  S8050 NPN
                 Emitter
                       |
                       |
                 GND Raspberry Pi

GPIO22 / pin 15 ----[1kΩ]---- Base S8050
```

### GND chung

GND của transistor phải là GND Raspberry Pi. Nếu dùng nguồn 5V ngoài cho buzzer thì bắt buộc:

```text
GND nguồn 5V ngoài ---- GND Raspberry Pi
```

Nếu không có GND chung, GPIO22 không có mốc điện áp để kích Base transistor, mạch có thể không kêu hoặc kêu chập chờn.

---

## 9. Logic buzzer thực tế sau khi đấu lại

Sau khi kiểm tra thực tế bằng log backend:

```text
on=True  -> GPIO22=HIGH -> còi tắt
on=False -> GPIO22=LOW  -> còi kêu
```

Do đó mạch hiện tại hoạt động theo kiểu active-LOW.

Backend hiện đã được chỉnh để phù hợp:

```text
Web bật còi  -> on=True  -> GPIO22 LOW  -> còi kêu
Web tắt còi  -> on=False -> GPIO22 HIGH -> còi tắt
```

Nghĩa là ở sơ đồ vẽ, cần ghi chú:

```text
Buzzer circuit: active-LOW
GPIO22 LOW  = buzzer ON
GPIO22 HIGH = buzzer OFF
```

---

## 10. Bảng netlist để vẽ mạch

| Net / node | Các điểm nối chung |
|---|---|
| 5V | Raspberry Pi 5V, cực dương buzzer |
| GND | Raspberry Pi GND, Emitter S8050, cathode LED đỏ, cathode LED xanh |
| BUZZER_CTRL | GPIO22 / physical pin 15, một đầu điện trở Base 1kΩ |
| BASE_S8050 | đầu còn lại điện trở 1kΩ, Base S8050 |
| BUZZER_LOW_SIDE | Collector S8050, cực âm buzzer |
| LED_RED_CTRL | GPIO17 / physical pin 11, điện trở LED đỏ, anode LED đỏ |
| LED_GREEN_CTRL | GPIO27 / physical pin 13, điện trở LED xanh, anode LED xanh |
---

## 11. Prompt gợi ý để đưa cho GPT vẽ mạch

Có thể dùng prompt sau:

```text
Hãy vẽ sơ đồ mạch breadboard/Raspberry Pi cho hệ thống sau:

- Raspberry Pi 4 dùng GPIO Extension Board trên breadboard.
- Dùng BCM GPIO numbering.
- LED đỏ: GPIO17 / physical pin 11 -> điện trở 220Ω-1kΩ -> anode LED đỏ; cathode LED đỏ -> GND.
- LED xanh: GPIO27 / physical pin 13 -> điện trở 220Ω-1kΩ -> anode LED xanh; cathode LED xanh -> GND.
- Không vẽ button và không nối GPIO23, vì button hiện không được lắp trên mạch.
- Buzzer 5V điều khiển qua transistor S8050 NPN low-side switch.
- Pinout S8050 khi nhìn mặt phẳng có chữ S8050, chân hướng xuống: trái Emitter, giữa Base, phải Collector.
- GPIO22 / physical pin 15 -> điện trở 1kΩ -> Base S8050.
- Emitter S8050 -> GND Raspberry Pi.
- Collector S8050 -> cực âm buzzer.
- Cực dương buzzer -> 5V Raspberry Pi.
- GND của toàn bộ mạch phải nối chung.
- Ghi chú trên sơ đồ: buzzer hiện hoạt động active-LOW, GPIO22 LOW = buzzer ON, GPIO22 HIGH = buzzer OFF.
```

---

## 12. Lưu ý an toàn

- Không nối trực tiếp 5V vào GPIO Raspberry Pi.
- Không nối 5V trực tiếp vào Base transistor.
- Base transistor phải đi qua điện trở khoảng 1kΩ.
- Không nên nối buzzer 5V trực tiếp vào GPIO22.
- Khi tháo/cắm dây, nên tắt nguồn Raspberry Pi trước.
- Cần kiểm tra đúng chân vật lý nếu không dùng GPIO Extension Board.

---

## 13. Tóm tắt ngắn gọn

Mạch hiện tại gồm:

```text
GPIO17 -> LED đỏ -> GND
GPIO27 -> LED xanh -> GND
GPIO22 -> điện trở 1kΩ -> Base S8050
S8050 Emitter -> GND
S8050 Collector -> Buzzer -
Buzzer + -> 5V
```

Còi hiện hoạt động active-LOW:

```text
GPIO22 LOW  -> còi kêu
GPIO22 HIGH -> còi tắt
```
