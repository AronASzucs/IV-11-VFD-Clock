/*
IV-11 VFD Tube Clock code by Aron Szucs





*/

#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>

#define LATCH_PIN  10
#define PWM_PIN    9
#define BRIGHTNESS 0    // 0 = full brightness, 255 = off
#define DOT_BIT    0b00000010 // defines dot bit

#define MODE 

RTC_DS3231 rtc; // defines clock

const uint8_t digits[] = {  // defines 0 through 9
  0b11101101, 0b00000101, 0b11011100, 0b10011101, 0b00110101,
  0b10111001, 0b11111001, 0b00001101, 0b11111101, 0b00111101
};

unsigned long lastUpdate = 0;
bool colonOn = true;

void sendFrame(uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t t5, uint8_t t6) {
  digitalWrite(LATCH_PIN, LOW);
  SPI.transfer(t6); SPI.transfer(t5); SPI.transfer(t4);
  SPI.transfer(t3); SPI.transfer(t2); SPI.transfer(t1);
  digitalWrite(LATCH_PIN, HIGH);
}

void displayTime(uint8_t h, uint8_t m, uint8_t s) {
  if (h == 0) h = 12;
  else if (h > 12) h -= 12;

  uint8_t dot = colonOn ? DOT_BIT : 0;

  sendFrame(
    h / 10 ? digits[h / 10] : 0,   // blank leading zero
    digits[h % 10] | dot,
    digits[m / 10],
    digits[m % 10] | dot,
    digits[s / 10],
    digits[s % 10]
  );
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
      sendFrame(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF); delay(200);
      sendFrame(0x00, 0x00, 0x00, 0x00, 0x00, 0x00); delay(200);
    }
  }
}

void loop() {
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    colonOn = !colonOn;
    DateTime t = rtc.now();
    displayTime(t.hour(), t.minute(), t.second());
  }
}