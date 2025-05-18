// Arduino Nano Motor Control with 4x4 Keypad and 20x4 I2C LCD
// Features: Configure and run 2 motors with interval + gap + EEPROM recovery

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {14, 15, 16, 17};
byte colPins[COLS] = {7, 8, 20, 21};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#define MOTOR1_1_PIN 3
#define MOTOR1_2_PIN 4
#define MOTOR2_1_PIN 5
#define MOTOR2_2_PIN 6
#define EEPROM_ADDR 0

int motor1Time = 0;
int motor2Time = 0;
int gapTime = 0;
bool configComplete = false;

int step = 0; // 0: M1, 1: M2, 2: Gap, 3: Confirm Start
int runStep = 0;
unsigned long runStartTime;
bool running = false;
bool resumedFromEEPROM = false;

void setup() {
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
  if (running) {
    runMotors();
    return;
  }

  char key = keypad.getKey();
  if (key) handleInput(key);
}

void handleInput(char key) {
  static String input = "";

  if (key >= '0' && key <= '9') {
    input += key;
    updateLCD("Enter time:", input + " min");
  }
  else if (key == '#') {
    int val = input.toInt();
    if (val < 1) val = 1;
    if (step == 0 && val <= 60) motor1Time = val;
    else if (step == 1 && val <= 60) motor2Time = val;
    else if (step == 2 && val <= 5) gapTime = val;
    input = "";
    if (step < 2) step++;
    updateMenu();
  }
  else if (key == 'B') {
    if (step > 0) step--;
    updateMenu();
  }
  else if (key == 'C') {
    if (step < 2) step++;
    updateMenu();
  }
  else if (key == 'D' && step == 2) {
    configComplete = true;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Starting Cycle...");
    runStartTime = millis();
    runStep = 0;
    running = true;
    saveToEEPROM();
  }
  else if (key == '*') {
    running = false;
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("EMERGENCY STOP");
    delay(2000);
    displayMenu();
  }
}

void updateLCD(String title, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void updateMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  if (step == 0) lcd.print("Set Motor1 (1-60m)");
  else if (step == 1) lcd.print("Set Motor2 (1-60m)");
  else if (step == 2) lcd.print("Set Gap (1-5m)");
  lcd.setCursor(0,1);
  lcd.print("B:Prev  #:Set  C:Next");
}

void displayMenu() {
  step = 0;
  motor1Time = motor2Time = gapTime = 0;
  configComplete = false;
  updateMenu();
}

void runMotors() {
  unsigned long elapsedMin = (millis() - runStartTime) / 60000;

  if (runStep == 0 && elapsedMin < motor1Time) {
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(1000);
    lcd.setCursor(0,0);
    lcd.print("Motor1 Running");
    digitalWrite(MOTOR1_1_PIN, HIGH);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(1000);
  } else if (runStep == 0) {
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, HIGH);
    delay(1000);
    digitalWrite(MOTOR1_1_PIN, LOW);
    digitalWrite(MOTOR1_2_PIN, LOW);
    delay(1000);
    runStartTime = millis();
    runStep = 1;
    saveToEEPROM();
  }
  else if (runStep == 1 && elapsedMin < gapTime) {
    lcd.setCursor(0,0);
    lcd.print("Waiting (gap)");
  } else if (runStep == 1) {
    runStartTime = millis();
    runStep = 2;
    saveToEEPROM();
  }
  else if (runStep == 2 && elapsedMin < motor2Time) {
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, HIGH);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);

  } else if (runStep == 2) {
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, HIGH);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(1000);
    running = false;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Cycle Complete");
    EEPROM.write(EEPROM_ADDR, 0); // Clear resume flag
  }
}

void saveToEEPROM() {
  EEPROM.write(EEPROM_ADDR, 1); // Flag to resume
  EEPROM.write(1, motor1Time);
  EEPROM.write(2, motor2Time);
  EEPROM.write(3, gapTime);
  EEPROM.write(4, runStep);
}
