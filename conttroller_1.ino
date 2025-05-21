#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>

// Pin Definitions
#define MOTOR1_1_PIN 2
#define MOTOR1_2_PIN 3
#define MOTOR2_1_PIN 4
#define MOTOR2_2_PIN 5

// EEPROM Address
#define EEPROM_ADDR 0

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variables
int motor1Time = 0;
int motor2Time = 0;
int gapTime = 0;
int cycleCount = 0;
int currentCycle = 1;

bool configComplete = false;
bool running = false;
bool resumedFromEEPROM = false;

int step = 0;
int runStep = 0;
unsigned long runStartTime = 0;
unsigned long lastMillis = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(MOTOR1_1_PIN, OUTPUT);
  pinMode(MOTOR1_2_PIN, OUTPUT);
  pinMode(MOTOR2_1_PIN, OUTPUT);
  pinMode(MOTOR2_2_PIN, OUTPUT);
  
  if (EEPROM.read(EEPROM_ADDR) == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Resume Last Task?");
    lcd.setCursor(0, 1);
    lcd.print("A: Yes   B: No");
    while (true) {
      char k = keypad.getKey();
      if (k == 'A') {
        readFromEEPROM();
        running = true;
        resumedFromEEPROM = true;
        runStartTime = millis();
        break;
      } else if (k == 'B') {
        EEPROM.write(EEPROM_ADDR, 0);
        break;
      }
    }
  }

  if (!running) {
    displayMenu();
  }
}

void loop() {
  if (!running) {
    char key = keypad.getKey();
    if (key) handleInput(key);
  } else {
    unsigned long currentMillis = millis();
    int remaining = 0;

    switch (runStep) {
      case 0:
        if (currentMillis - runStartTime < motor1Time * 60000UL) {
          digitalWrite(MOTOR1_1_PIN, HIGH);
          digitalWrite(MOTOR1_2_PIN, LOW);
          remaining = (motor1Time * 60) - (currentMillis - runStartTime) / 1000;
          updateRunLCD("Motor 1 Running", remaining);
        } else {
          digitalWrite(MOTOR1_1_PIN, LOW);
          digitalWrite(MOTOR1_2_PIN, LOW);
          runStep++;
          runStartTime = currentMillis;
        }
        break;

      case 1:
        if (currentMillis - runStartTime < gapTime * 60000UL) {
          updateRunLCD("Gap Interval", ((gapTime * 60) - (currentMillis - runStartTime) / 1000));
        } else {
          runStep++;
          runStartTime = currentMillis;
        }
        break;

      case 2:
        if (currentMillis - runStartTime < motor2Time * 60000UL) {
          digitalWrite(MOTOR2_1_PIN, HIGH);
          digitalWrite(MOTOR2_2_PIN, LOW);
          remaining = (motor2Time * 60) - (currentMillis - runStartTime) / 1000;
          updateRunLCD("Motor 2 Running", remaining);
        } else {
          digitalWrite(MOTOR2_1_PIN, LOW);
          digitalWrite(MOTOR2_2_PIN, LOW);
          runStep++;
          runStartTime = currentMillis;
        }
        break;

      case 3:
        if (currentCycle < cycleCount) {
          currentCycle++;
          runStep = 0;
          runStartTime = currentMillis;
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Cycle Complete");
          running = false;
          EEPROM.write(EEPROM_ADDR, 0); // clear resume flag
        }
        break;
    }
  }
}

void handleInput(char key) {
  static String input = "";

  if (key >= '0' && key <= '9') {
    input += key;
    updateLCD("Enter time:", input + " min");
  }
  else if (key == '#') {
    int val = input.toInt();
    bool valid = false;

    if (step == 0 && val >= 1 && val <= 60) {
      motor1Time = val; valid = true;
    } else if (step == 1 && val >= 1 && val <= 60) {
      motor2Time = val; valid = true;
    } else if (step == 2 && val >= 1 && val <= 5) {
      gapTime = val; valid = true;
    } else if (step == 3 && val >= 1 && val <= 5) {
      cycleCount = val; valid = true;
    }

    if (valid) {
      input = "";
      if (step < 3) step++;
      updateMenu();
    } else {
      input = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Out of Range!");
      lcd.setCursor(0, 1);
      if (step == 0 || step == 1) lcd.print("Valid: 1-60 mins");
      else if (step == 2) lcd.print("Valid: 1-5 mins");
      else if (step == 3) lcd.print("Valid: 1-5 times");
      delay(2000);
      updateMenu();
    }
  }
  else if (key == 'A') {
    input = "";
    updateLCD("Cleared", "");
  }
  else if (key == 'B') {
    if (step > 0) step--;
    updateMenu();
  }
  else if (key == 'C') {
    if (step < 3) step++;
    updateMenu();
  }
  else if (key == 'D' && step == 3) {
    configComplete = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Starting Cycle...");
    runStartTime = millis();
    runStep = 0;
    running = true;
    currentCycle = 1;
    saveToEEPROM();
  }
  else if (key == '*') {
    emergencyStop();
  }
}

void updateMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (step == 0) lcd.print("Set Motor1 (1-60m)");
  else if (step == 1) lcd.print("Set Motor2 (1-60m)");
  else if (step == 2) lcd.print("Set Gap (1-5m)");
  else if (step == 3) lcd.print("Cycle Repeat (1-5)");

  lcd.setCursor(0, 1);
  lcd.print("B:Prev C:Next");
  if (step == 3) {
    lcd.setCursor(0, 2);
    lcd.print("D:Start *:Stop");
  }
}

void displayMenu() {
  step = 0;
  motor1Time = motor2Time = gapTime = cycleCount = 0;
  configComplete = false;
  updateMenu();
}

void updateLCD(String title, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  lcd.setCursor(0, 2);
  lcd.print("A:Clear  #:Set");
}

void updateRunLCD(String status, int remainingSec) {
  lcd.setCursor(0, 0);
  lcd.print("Cycle ");
  lcd.print(currentCycle);
  lcd.print("/");
  lcd.print(cycleCount);
  lcd.setCursor(0, 1);
  lcd.print(status);
  lcd.setCursor(0, 2);
  lcd.print("Remain: ");
  lcd.print(remainingSec);
  lcd.print("s    ");
  lcd.setCursor(0, 3);
  lcd.print("*:STOP ");
}

void emergencyStop() {
  running = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCY STOP");

  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, LOW);
  delay(1000);
  digitalWrite(MOTOR1_2_PIN, HIGH);
  delay(1000);
  digitalWrite(MOTOR1_2_PIN, LOW);
  delay(500);

  digitalWrite(MOTOR2_1_PIN, LOW);
  digitalWrite(MOTOR2_2_PIN, LOW);
  delay(1000);
  digitalWrite(MOTOR2_2_PIN, HIGH);
  delay(1000);
  digitalWrite(MOTOR2_2_PIN, LOW);
  delay(500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Resume Last Task?");
  lcd.setCursor(0, 1);
  lcd.print("A: Yes   B: No");

  while (true) {
    char k = keypad.getKey();
    if (k == 'A') {
      running = true;
      resumedFromEEPROM = true;
      runStartTime = millis();
      break;
    } else if (k == 'B') {
      EEPROM.write(EEPROM_ADDR, 0);
      resumedFromEEPROM = false;
      displayMenu();
      break;
    }
  }
}

void saveToEEPROM() {
  EEPROM.write(EEPROM_ADDR, 1);
  EEPROM.write(EEPROM_ADDR + 1, motor1Time);
  EEPROM.write(EEPROM_ADDR + 2, motor2Time);
  EEPROM.write(EEPROM_ADDR + 3, gapTime);
  EEPROM.write(EEPROM_ADDR + 4, cycleCount);
}

void readFromEEPROM() {
  motor1Time = EEPROM.read(EEPROM_ADDR + 1);
  motor2Time = EEPROM.read(EEPROM_ADDR + 2);
  gapTime = EEPROM.read(EEPROM_ADDR + 3);
  cycleCount = EEPROM.read(EEPROM_ADDR + 4);
  currentCycle = 1;
  runStep = 0;
}
