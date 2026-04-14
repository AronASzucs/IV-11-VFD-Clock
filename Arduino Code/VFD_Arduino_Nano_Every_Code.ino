#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>

#define LATCH_PIN 10
#define PWM_PIN 9
#define BRIGHTNESS 250  // 0-255, 0 being brightest and 255 being off. 

RTC_DS3231 rtc;

const uint8_t digits[] = {
  0b11101101, // 0
  0b00000101, // 1
  0b11011100, // 2
  0b10011101, // 3
  0b00110101, // 4
  0b10111001, // 5
  0b11111001, // 6
  0b00001101, // 7
  0b11111101, // 8
  0b00111101, // 9
};

#define DOT_BIT 0b00000010

unsigned long lastUpdate = 0;
bool colonOn = true;

void sendFrame(uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t t5, uint8_t t6) {
  digitalWrite(LATCH_PIN, LOW);
  SPI.transfer(t6);
  SPI.transfer(t5);
  SPI.transfer(t4);
  SPI.transfer(t3);
  SPI.transfer(t2);
  SPI.transfer(t1);
  digitalWrite(LATCH_PIN, HIGH);
}

void displayTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  if (hours == 0) hours = 12;
  else if (hours > 12) hours -= 12;

  uint8_t h1 = hours / 10;
  uint8_t h2 = hours % 10;
  uint8_t m1 = minutes / 10;
  uint8_t m2 = minutes % 10;
  uint8_t s1 = seconds / 10;
  uint8_t s2 = seconds % 10;

  uint8_t h1seg = (h1 == 0) ? 0b00000000 : digits[h1];
  uint8_t h2seg = colonOn ? digits[h2] | DOT_BIT : digits[h2];
  uint8_t m2seg = colonOn ? digits[m2] | DOT_BIT : digits[m2];

  sendFrame(h1seg, h2seg, digits[m1], m2seg, digits[s1], digits[s2]);
}

void setup() {
  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, HIGH);
  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, BRIGHTNESS);
  SPI.begin();
  SPI.beginTransaction(SPISettings(500000, LSBFIRST, SPI_MODE0));
  Wire.begin();

  if (!rtc.begin()) {
    while (1) {
      sendFrame(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
      delay(200);
      sendFrame(0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      delay(200);
    }
  }
}

void loop() {
  unsigned long now = millis();

  if (now - lastUpdate >= 1000) {
    lastUpdate = now;
    colonOn = !colonOn;
    DateTime t = rtc.now();
    displayTime(t.hour(), t.minute(), t.second());
  }
}