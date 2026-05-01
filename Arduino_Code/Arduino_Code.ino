/*
IV-11 VFD Tube Clock
code by Aron Szucs

Display Modes (SELECT in clock):
0 - HH:MM:SS
1 - MM:SS

Setting Modes (MODE cycles down chain):
0 - Normal clock
1 - Set hour
2 - Set minute
3 - Set second
4 - Set brightness (1-9, all tubes)
5 - Auto brightness (Auto t / Auto F)
6 - 12/24hr (shows 12 or 24)

SELECT on any setting → save + return to clock
SELECT on clock → toggle display mode
*/

#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

#define LATCH_PIN       10
#define PWM_PIN         9
#define MODE_BTN_PIN    2
#define UP_BTN_PIN      3
#define DOWN_BTN_PIN    4
#define SELECT_BTN_PIN  5
#define DOT_BIT         0b01000000
#define DEBOUNCE        150

#define ADDR_BRIGHTNESS  0
#define ADDR_12HR        1
#define ADDR_AUTO_BRIGHT 2

RTC_DS3231 rtc;

const int digits[] = {
  0b10110111, // 0
  0b10100000, // 1
  0b00111011, // 2
  0b10111001, // 3
  0b10101100, // 4
  0b10011101, // 5
  0b10011111, // 6
  0b10110000, // 7
  0b10111111, // 8
  0b10111101  // 9
};

const int BLANK = 0b00000000;
const int LET_A = 0b01111101;
const int LET_u = 0b11000001;
const int LET_t = 0b01110000;
const int LET_o = 0b11010001;
const int LET_F = 0b01111000;

const int brightnessLevels[] = {0, 28, 56, 84, 112, 140, 168, 196, 220, 245};

const int autoSchedule[24] = {
  230,230,230,230,230,230,
  180,100,50,20,0,0,
  0,0,0,0,0,20,
  80,150,200,220,225,230
};

int  clockMode  = 0;
int  dispMode   = 0;
int  brightness = 5;
bool use12hr    = true;
bool autoBright = false;
bool colonOn    = true;
bool editBlink  = true;
bool timeEdited = false; // tracks if user has touched time in edit mode

// live display values during edit — follow RTC until user edits
int dispHour = 0;
int dispMin  = 0;
int dispSec  = 0;

int lastMode   = HIGH;
int lastUp     = HIGH;
int lastDown   = HIGH;
int lastSelect = HIGH;

unsigned long lastUpdate     = 0;
unsigned long lastBlink      = 0;
unsigned long lastModePress  = 0;
unsigned long lastUpPress    = 0;
unsigned long lastDownPress  = 0;
unsigned long lastSelectPress= 0;

// ── EEPROM ────────────────────────────────────────────────

void loadSettings() {
  brightness = EEPROM.read(ADDR_BRIGHTNESS);
  use12hr    = EEPROM.read(ADDR_12HR);
  autoBright = EEPROM.read(ADDR_AUTO_BRIGHT);
  if (brightness < 1 || brightness > 9) brightness = 5;
  if (use12hr > 1)    use12hr    = true;
  if (autoBright > 1) autoBright = false;
  analogWrite(PWM_PIN, brightnessLevels[brightness]);
  Serial.println("=== Settings loaded ===");
  Serial.print("Brightness: ");  Serial.println(brightness);
  Serial.print("12hr: ");        Serial.println(use12hr ? "ON" : "OFF");
  Serial.print("Auto bright: "); Serial.println(autoBright ? "ON" : "OFF");
}

void saveSettings() {
  EEPROM.write(ADDR_BRIGHTNESS,  brightness);
  EEPROM.write(ADDR_12HR,        use12hr);
  EEPROM.write(ADDR_AUTO_BRIGHT, autoBright);
  Serial.println("=== Settings saved ===");
}

// ── SPI ───────────────────────────────────────────────────

void sendFrame(int t1, int t2, int t3, int t4, int t5, int t6) {
  digitalWrite(LATCH_PIN, LOW);
  SPI.transfer(t1);
  SPI.transfer(t2);
  SPI.transfer(t3);
  SPI.transfer(t4);
  SPI.transfer(t5);
  SPI.transfer(t6);
  digitalWrite(LATCH_PIN, HIGH);
}

// ── DISPLAY ───────────────────────────────────────────────

int toDisplay(int h) {
  if (use12hr) {
    if (h == 0) h = 12;
    else if (h > 12) h -= 12;
  }
  return h;
}

void displayClock(int h, int m, int s) {
  h = toDisplay(h);
  int dot = colonOn ? DOT_BIT : 0;
  if (dispMode == 0) {
    sendFrame(h/10 ? digits[h/10] : BLANK, digits[h%10]|dot, digits[m/10], digits[m%10]|dot, digits[s/10], digits[s%10]);
  } else {
    sendFrame(BLANK, BLANK, digits[m/10], digits[m%10]|dot, digits[s/10], digits[s%10]);
  }
}

void displayTimeEdit(int h, int m, int s) {
  h = toDisplay(h);
  int dot = colonOn ? DOT_BIT : 0;
  int h1 = h/10 ? digits[h/10] : BLANK;
  int h2 = digits[h%10]|dot;
  int m1 = digits[m/10];
  int m2 = digits[m%10]|dot;
  int s1 = digits[s/10];
  int s2 = digits[s%10];
  if (!editBlink) {
    if (clockMode == 1) { h1 = BLANK; h2 = BLANK; }
    if (clockMode == 2) { m1 = BLANK; m2 = BLANK; }
    if (clockMode == 3) { s1 = BLANK; s2 = BLANK; }
  }
  sendFrame(h1, h2, m1, m2, s1, s2);
}

void displayBrightness() {
  int d = digits[brightness];
  sendFrame(d, d, d, d, d, d);
}

void displayAutoBright() {
  sendFrame(LET_A, LET_u, LET_t, LET_o, BLANK, autoBright ? LET_t : LET_F);
}

void display1224() {
  int tens = use12hr ? digits[1] : digits[2];
  int ones = use12hr ? digits[2] : digits[4];
  sendFrame(BLANK, BLANK, tens, ones, BLANK, BLANK);
}

void handleDisplay() {
  DateTime t = rtc.now();
  if (autoBright && clockMode == 0) analogWrite(PWM_PIN, autoSchedule[t.hour()]);

  switch (clockMode) {
    case 0:
      displayClock(t.hour(), t.minute(), t.second());
      break;
    case 1: case 2: case 3:
      // if not yet edited by user, follow live RTC time
      if (!timeEdited) {
        dispHour = t.hour();
        dispMin  = t.minute();
        dispSec  = t.second();
      }
      displayTimeEdit(dispHour, dispMin, dispSec);
      break;
    case 4: displayBrightness();  break;
    case 5: displayAutoBright();  break;
    case 6: display1224();        break;
  }
}

// ── BUTTONS ───────────────────────────────────────────────

void handleButtons() {
  int modeBtn   = digitalRead(MODE_BTN_PIN);
  int upBtn     = digitalRead(UP_BTN_PIN);
  int downBtn   = digitalRead(DOWN_BTN_PIN);
  int selectBtn = digitalRead(SELECT_BTN_PIN);
  unsigned long now = millis();

  // MODE — advance chain
  if (modeBtn == LOW && lastMode == HIGH && now - lastModePress > DEBOUNCE) {
    lastModePress = now;
    if (clockMode == 0) {
      // snapshot RTC as starting point for edit
      DateTime t = rtc.now();
      dispHour   = t.hour();
      dispMin    = t.minute();
      dispSec    = t.second();
      timeEdited = false;
    }
    clockMode = (clockMode + 1) % 7;
    Serial.print("Mode → "); Serial.println(clockMode);
  }

  // SELECT — toggle display in clock, save + exit in settings
  if (selectBtn == LOW && lastSelect == HIGH && now - lastSelectPress > DEBOUNCE) {
    lastSelectPress = now;
    if (clockMode == 0) {
      dispMode = (dispMode + 1) % 2;
      Serial.print("Display mode → "); Serial.println(dispMode);
    } else {
      if (clockMode >= 1 && clockMode <= 3 && timeEdited) {
        DateTime t = rtc.now();
        rtc.adjust(DateTime(t.year(), t.month(), t.day(), dispHour, dispMin, dispSec));
        Serial.print("Time saved → ");
        Serial.print(dispHour); Serial.print(":"); Serial.print(dispMin); Serial.print(":"); Serial.println(dispSec);
      } else if (clockMode >= 1 && clockMode <= 3 && !timeEdited) {
        Serial.println("No edit made — time unchanged");
      }
      if (clockMode >= 4) saveSettings();
      timeEdited = false;
      clockMode  = 0;
      Serial.println("→ clock");
    }
  }

  // UP
  if (upBtn == LOW && lastUp == HIGH && now - lastUpPress > DEBOUNCE) {
    lastUpPress = now;
    if (clockMode >= 1 && clockMode <= 3) {
      // snapshot RTC on first edit so we start from current time
      if (!timeEdited) {
        DateTime t = rtc.now();
        dispHour   = t.hour();
        dispMin    = t.minute();
        dispSec    = t.second();
        timeEdited = true;
      }
    }
    if      (clockMode == 1) { dispHour   = (dispHour  + 1)  % 24; Serial.print("Hour: ");       Serial.println(dispHour); }
    else if (clockMode == 2) { dispMin    = (dispMin   + 1)  % 60; Serial.print("Min: ");        Serial.println(dispMin); }
    else if (clockMode == 3) { dispSec    = (dispSec   + 1)  % 60; Serial.print("Sec: ");        Serial.println(dispSec); }
    else if (clockMode == 4) { brightness = min(brightness + 1, 9); analogWrite(PWM_PIN, brightnessLevels[brightness]); Serial.print("Brightness: "); Serial.println(brightness); }
    else if (clockMode == 5) { autoBright = !autoBright; Serial.print("Auto: "); Serial.println(autoBright ? "ON" : "OFF"); }
    else if (clockMode == 6) { use12hr    = !use12hr;    Serial.print("12hr: "); Serial.println(use12hr ? "ON" : "OFF"); }
  }

  // DOWN
  if (downBtn == LOW && lastDown == HIGH && now - lastDownPress > DEBOUNCE) {
    lastDownPress = now;
    if (clockMode >= 1 && clockMode <= 3) {
      if (!timeEdited) {
        DateTime t = rtc.now();
        dispHour   = t.hour();
        dispMin    = t.minute();
        dispSec    = t.second();
        timeEdited = true;
      }
    }
    if      (clockMode == 1) { dispHour   = (dispHour  + 23) % 24; Serial.print("Hour: ");       Serial.println(dispHour); }
    else if (clockMode == 2) { dispMin    = (dispMin   + 59) % 60; Serial.print("Min: ");        Serial.println(dispMin); }
    else if (clockMode == 3) { dispSec    = (dispSec   + 59) % 60; Serial.print("Sec: ");        Serial.println(dispSec); }
    else if (clockMode == 4) { brightness = max(brightness - 1, 1); analogWrite(PWM_PIN, brightnessLevels[brightness]); Serial.print("Brightness: "); Serial.println(brightness); }
    else if (clockMode == 5) { autoBright = !autoBright; Serial.print("Auto: "); Serial.println(autoBright ? "ON" : "OFF"); }
    else if (clockMode == 6) { use12hr    = !use12hr;    Serial.print("12hr: "); Serial.println(use12hr ? "ON" : "OFF"); }
  }

  lastMode   = modeBtn;
  lastUp     = upBtn;
  lastDown   = downBtn;
  lastSelect = selectBtn;
}

// ── SETUP ─────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);
  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, HIGH);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(MODE_BTN_PIN,   INPUT_PULLUP);
  pinMode(UP_BTN_PIN,     INPUT_PULLUP);
  pinMode(DOWN_BTN_PIN,   INPUT_PULLUP);
  pinMode(SELECT_BTN_PIN, INPUT_PULLUP);

  SPI.begin();
  SPI.beginTransaction(SPISettings(500000, LSBFIRST, SPI_MODE0));
  Wire.begin();
  loadSettings();

  if (!rtc.begin()) {
    Serial.println("ERROR: RTC not found");
    while (1) {
      sendFrame(0xFF,0xFF,0xFF,0xFF,0xFF,0xFF); delay(200);
      sendFrame(0x00,0x00,0x00,0x00,0x00,0x00); delay(200);
    }
  }

  DateTime t = rtc.now();
  Serial.print("=== IV-11 Clock started: ");
  Serial.print(t.hour()); Serial.print(":"); Serial.print(t.minute()); Serial.print(":"); Serial.println(t.second());
}

// ── LOOP ──────────────────────────────────────────────────

void loop() {
  handleButtons();

  // blink — redraws edit modes immediately so blink is responsive
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    editBlink = !editBlink;
    if (clockMode >= 1 && clockMode <= 3) handleDisplay();
  }

  // 1 second tick
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    colonOn = !colonOn;
    handleDisplay();
  }
}