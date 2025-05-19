#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
bool motor1PulseDone = false;
bool motor2PulseDone = false;
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {14, 15, 16, 17};
byte colPins[COLS] = {7, 8, 9, 10};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#define MOTOR1_1_PIN 3
#define MOTOR1_2_PIN 4
#define MOTOR2_1_PIN 5
#define MOTOR2_2_PIN 6
#define EEPROM_ADDR 0

int motor1Time = 0, motor2Time = 0, gapTime = 0, cycleCount = 1; // NEW
int currentCycle = 1; // NEW
bool configComplete = false;

int step = 0; // 0: M1, 1: M2, 2: Gap, 3: Repeat Count
int runStep = 0;
unsigned long runStartTime;
bool running = false, resumedFromEEPROM = false;

void setup() {
  Serial.begin(9600);
  pinMode(MOTOR1_1_PIN, OUTPUT);
  pinMode(MOTOR1_2_PIN, OUTPUT);
  pinMode(MOTOR2_1_PIN, OUTPUT);
  pinMode(MOTOR2_2_PIN, OUTPUT);
  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, LOW);
  digitalWrite(MOTOR2_1_PIN, LOW);
  digitalWrite(MOTOR2_2_PIN, LOW);

  lcd.init();
  lcd.backlight();

  checkEEPROMState();
  if (!resumedFromEEPROM) displayMenu();
}

void checkEEPROMState() {
  byte flag = EEPROM.read(EEPROM_ADDR);
  if (flag == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Resume Last Task?");
    lcd.setCursor(0, 1);
    lcd.print("A: Yes   B: No");

    while (true) {
      char key = keypad.getKey();
      if (key == 'A') {
        motor1Time = EEPROM.read(1);
        motor2Time = EEPROM.read(2);
        gapTime = EEPROM.read(3);
        runStep = EEPROM.read(4);
        cycleCount = EEPROM.read(5); // NEW
        currentCycle = EEPROM.read(6); // NEW
        running = true;
        resumedFromEEPROM = true;
        runStartTime = millis();
        break;
      } else if (key == 'B') {
        EEPROM.write(EEPROM_ADDR, 0);
        break;
      }
    }
  }
}

void loop() {
  char key = keypad.getKey();
  if (key) handleInput(key);
  if (running) runMotors();
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

if (step == 0) {
  if (val >= 1 && val <= 60) {
    motor1Time = val;
    valid = true;
  }
} else if (step == 1) {
  if (val >= 1 && val <= 60) {
    motor2Time = val;
    valid = true;
  }
} else if (step == 2) {
  if (val >= 1 && val <= 5) {
    gapTime = val;
    valid = true;
  }
} else if (step == 3) { // Cycle repeat step
  if (val >= 1 && val <= 5) {
    cycleCount = val;
    valid = true;
  }
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
  updateMenu();  // Stay on same step
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
    lcd.setCursor(0,0);
    lcd.print("Starting Cycle...");
    runStartTime = millis();
    runStep = 0;
    running = true;
    currentCycle = 1; // NEW
    saveToEEPROM();
  }
  else if (key == '*') {
    emergencyStop();
  }
}

void emergencyStop() {
  running = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCY STOP");
  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, LOW);
  delay(1000);
  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, HIGH);
  delay(1000);
  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, LOW);
  delay(500);
  digitalWrite(MOTOR2_1_PIN, LOW);
  digitalWrite(MOTOR2_2_PIN, LOW);
  delay(1000);
  digitalWrite(MOTOR2_1_PIN, LOW);
  digitalWrite(MOTOR2_2_PIN, HIGH);
  delay(1000);
  digitalWrite(MOTOR2_1_PIN, LOW);
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

void updateLCD(String title, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  lcd.setCursor(0, 2);
  lcd.print("A:Clear  #:Set");
}

void updateMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  if (step == 0) lcd.print("Set Motor1 (1-60m)");
  else if (step == 1) lcd.print("Set Motor2 (1-60m)");
  else if (step == 2) lcd.print("Set Gap (1-5m)");
  else if (step == 3) lcd.print("Cycle Repeat (1-20)");

  lcd.setCursor(0,1);
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

void runMotors() {
  unsigned long elapsedMin = (millis() - runStartTime) / 60000;
  int remaining = 0;

if (runStep == 0 && elapsedMin < motor1Time) {
  remaining = motor1Time - elapsedMin;
  lcd.setCursor(0,0);
  lcd.print("Motor1 Running     ");
  lcd.setCursor(0,1);
  lcd.print("Time Left: ");
  lcd.print(remaining);
  lcd.print(" min    ");
  lcd.setCursor(0, 2);
  lcd.print("Cycle ");
  lcd.print(currentCycle);
  lcd.print("/");
  lcd.print(cycleCount);
  if (!motor1PulseDone) {
  // Run the 3-phase ON/OFF pulse sequence just once
  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, LOW);
  delay(1000);
  digitalWrite(MOTOR1_1_PIN, HIGH);
  digitalWrite(MOTOR1_2_PIN, LOW);
  delay(1000);
  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, LOW);
  delay(500);
  motor1PulseDone = true;  // prevent re-execution
  }
}
else if (runStep == 0) {
  if (motor1PulseDone) {
    // Second pulse sequence, again only once
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, HIGH);
    delay(1000);
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(500);
    motor1PulseDone = false;  // reset for next cycle
  }

  runStartTime = millis();
  runStep = 1;
  saveToEEPROM();
}

  else if (runStep == 1 && elapsedMin < gapTime) {
    remaining = gapTime - elapsedMin;
    lcd.setCursor(0,0);
    lcd.print("Waiting (gap)     ");
    lcd.setCursor(0,1);
    lcd.print("Time Left: ");
    lcd.print(remaining);
    lcd.print(" min    ");
    lcd.setCursor(0, 2);
    lcd.print("Cycle ");
    lcd.print(currentCycle);
    lcd.print("/");
    lcd.print(cycleCount);
  }
  else if (runStep == 1) {
    runStartTime = millis();
    runStep = 2;
    saveToEEPROM();
  }
  else if (runStep == 2 && elapsedMin < motor2Time) {
    remaining = motor2Time - elapsedMin;
    lcd.setCursor(0,0);
    lcd.print("Motor2 Running     ");
    lcd.setCursor(0,1);
    lcd.print("Time Left: ");
    lcd.print(remaining);
    lcd.print(" min    ");
    lcd.setCursor(0, 2);
    lcd.print("Cycle ");
    lcd.print(currentCycle);
    lcd.print("/");
    lcd.print(cycleCount);
    if (!motor2PulseDone) {
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, HIGH);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(500);
    motor2PulseDone = true;
    }
  }
  else if (runStep == 2) {
    if (motor2PulseDone) {
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, HIGH);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(500);
    motor2PulseDone = false;
    }

    if (currentCycle < cycleCount) {
      currentCycle++;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cycle ");
      lcd.print(currentCycle);
      lcd.print(" Completed");
      lcd.setCursor(0, 1);
      lcd.print("Starting Next...");
      delay(100);
      runStartTime = millis();
      runStep = 0;
      saveToEEPROM();
    } else {
      running = false;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Cycle Complete");

      EEPROM.write(EEPROM_ADDR, 0);
      delay(2000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Rerun same config?");
      lcd.setCursor(0,1);
      lcd.print("A: Yes   B: No");

      while (true) {
        char k = keypad.getKey();
        if (k == 'A') {
          runStartTime = millis();
          runStep = 0;
          currentCycle = 1;
          running = true;
          saveToEEPROM();
          break;
        } else if (k == 'B') {
          displayMenu();
          break;
        }
      }
    }
  }
}

void saveToEEPROM() {
  EEPROM.write(EEPROM_ADDR, 1);
  EEPROM.write(1, motor1Time);
  EEPROM.write(2, motor2Time);
  EEPROM.write(3, gapTime);
  EEPROM.write(4, runStep);
  EEPROM.write(5, cycleCount); // NEW
  EEPROM.write(6, currentCycle); // NEW
}
