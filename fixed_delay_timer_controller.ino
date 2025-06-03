#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define MOTOR1_1_PIN 5
#define MOTOR1_2_PIN 6
#define MOTOR2_1_PIN 3
#define MOTOR2_2_PIN 4
#define FIXED_GAP_SEC 15
#define EEPROM_ADDR 0
int runStep = 0;
bool motor1PulseDone = false;
bool motor2PulseDone = false;
bool m1Started = false, m1Ended = false, m1PulseStartDone = false, m1PulseEndDone = false;
bool m2Started = false, m2Ended = false, m2PulseStartDone = false, m2PulseEndDone = false;

unsigned long cycleStartTime = 0;

unsigned long m1StartTime = 0;
unsigned long m2StartTime = 0;

LiquidCrystal_I2C lcd(0x27, 20, 4);
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {14, 15, 16, 17};
byte colPins[COLS] = {7, 8, 9, 10};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int motor1Time = 0, motor2Time = 0, gapTime = 0, cycleCount = 0;
int step = 0, currentCycle = 0;
bool configComplete = false, running = false, resumedFromEEPROM = false;
unsigned long runStartTime = 0;

void setup() {
  Serial.begin(9600);
  lcd.init(); lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Version_16");
  delay(1500);
  pinMode(MOTOR1_1_PIN, OUTPUT);
  pinMode(MOTOR1_2_PIN, OUTPUT);
  pinMode(MOTOR2_1_PIN, OUTPUT);
  pinMode(MOTOR2_2_PIN, OUTPUT);
  digitalWrite(MOTOR1_1_PIN, LOW);
  digitalWrite(MOTOR1_2_PIN, LOW);
  digitalWrite(MOTOR2_1_PIN, LOW);
  digitalWrite(MOTOR2_2_PIN, LOW);
  displayMenu();
}


void loop() {
  char key = keypad.getKey();
  if (key == '*') {
    emergencyStop();
    return;
  }

  unsigned long now = millis();

  if (running) {
    unsigned long elapsedSec = (now - cycleStartTime) / 1000;
    //unsigned long m2DelayMillis = gapTime * 60000UL;

    // --- Motor 1 Control ---
    if (!m1Started) {
      // Start pulse
      if (!m1PulseStartDone) {
        Serial.println("Motor 1 Start Pulse");
        digitalWrite(MOTOR1_1_PIN, HIGH);
        digitalWrite(MOTOR1_2_PIN, LOW);
        delay(1000);
        digitalWrite(MOTOR1_1_PIN, LOW);
        digitalWrite(MOTOR1_2_PIN, LOW);
        delay(500);
        Serial.println("motor1 on");
        m1PulseStartDone = true;
        m1Started = true;
        m1StartTime = now;
      }
    } else if (!m1Ended && (now - m1StartTime >= motor1Time * 60000UL)) {
      // End pulse
      if (!m1PulseEndDone) {
        Serial.println("Motor 1 End Pulse");
        digitalWrite(MOTOR1_1_PIN, LOW);
        digitalWrite(MOTOR1_2_PIN, HIGH);
        delay(1000);
        digitalWrite(MOTOR1_1_PIN, LOW);
        digitalWrite(MOTOR1_2_PIN, LOW);
        delay(500);
        Serial.println("motor1 off");
        m1PulseEndDone = true;
        m1Ended = true;
      }
    }

    // --- Motor 2 Control ---
    // --- Motor 2 Control ---
// --- Motor 2 Control ---
if (m1Started && !m2Started) {
  if ((now - m1StartTime) >= FIXED_GAP_SEC * 1000UL) {
    // Start pulse
    if (!m2PulseStartDone) {
      Serial.println("Motor 2 Start Pulse");
      digitalWrite(MOTOR2_1_PIN, HIGH);
      digitalWrite(MOTOR2_2_PIN, LOW);
      delay(1000);
      digitalWrite(MOTOR2_1_PIN, LOW);
      digitalWrite(MOTOR2_2_PIN, LOW);
      delay(500);
      Serial.println("motor2 on");
      m2PulseStartDone = true;
      m2Started = true;
      m2StartTime = now;
    }
  }
} else if (m2Started && !m2Ended && (now - m2StartTime >= motor2Time * 60000UL)) {
  // End pulse
  if (!m2PulseEndDone) {
    Serial.println("Motor 2 End Pulse");
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, HIGH);
    delay(1000);
    digitalWrite(MOTOR2_1_PIN, LOW);
    digitalWrite(MOTOR2_2_PIN, LOW);
    delay(500);
    Serial.println("motor2 off");
    m2PulseEndDone = true;
    m2Ended = true;
  }
}


    // --- Update LCD Countdown ---
    /*int remaining1 = 0, remaining2 = 0;
    if (!m1Ended) remaining1 = motor1Time * 60 - ((now - m1StartTime) / 1000);
    if (!m2Ended && m2Started) remaining2 = motor2Time * 60 - ((now - m2StartTime) / 1000);
    int maxRem = max(remaining1, remaining2);
    updateRunLCD("Running...", maxRem);*/
    int rem1 = (!m1Ended) ? motor1Time * 60 - (now - m1StartTime) / 1000 : 0;
    int rem2 = (m2Started && !m2Ended) ? motor2Time * 60 - (now - m2StartTime) / 1000 : 0;
    updateRunLCD("Running...", max(rem1, rem2));
    //printProgressBar(0, 1, totalTime - maxRem, totalTime);

    // --- Cycle Completion ---
    if (m1Ended && m2Ended) {
      currentCycle++;
      if (currentCycle > cycleCount) {
        running = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("All Cycles Done!");
        delay(3000);
        // Add these lines to reset flags
        m1Started = m1Ended = m1PulseStartDone = m1PulseEndDone = false;
        m2Started = m2Ended = m2PulseStartDone = m2PulseEndDone = false;
        displayMenu();
      } else {
        resetMotorCycle();
      }
    }
  } else {
    if (key) handleInput(key);
  }
}
void prepareNewCycle() {
  m1Started = m1Ended = m1PulseStartDone = m1PulseEndDone = false;
  m2Started = m2Ended = m2PulseStartDone = m2PulseEndDone = false;
  currentCycle = 1;
  cycleStartTime = millis();
  running = true;
}
void resetMotorCycle() {
  m1Started = false;
  m1Ended = false;
  m1PulseStartDone = false;
  m1PulseEndDone = false;
  m2Started = false;
  m2Ended = false;
  m2PulseStartDone = false;
  m2PulseEndDone = false;
  cycleStartTime = millis();
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

    if (step == 0 && val >= 3 && val <= 30) { motor1Time = val; valid = true; }
    else if (step == 1 && val >= 1 && val <= 29) { 
      if (val < motor1Time) {
      motor2Time = val;
      valid = true;
      } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Motor 2 must be < Motor 1");
      delay(2000);
      updateMenu();
        }
     }
    else if (step == 2 && val >= 1 && val <= 5) { cycleCount = val; valid = true; }

    if (valid) {
      input = "";
      if (step < 2) step++;
      updateMenu();
    } else {
      input = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Out of Range!");
      lcd.setCursor(0, 1);
      if (step == 0) lcd.print("Valid: 3-30 mins");
      else if (step == 1) lcd.print("Valid: 1-29 mins");
      else if (step == 2) lcd.print("Valid: 1-5 times");
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
    if (step < 2) step++;
    updateMenu();
  }
  else if (key == 'D' && step == 2) {
    configComplete = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Starting Cycle...");
    runStartTime = millis();
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
  if (step == 0) lcd.print("Set Motor1 (3-30m)");
  else if (step == 1) lcd.print("Set Motor2 (1-29m)");
  else if (step == 2) lcd.print("Cycle Repeat (1-5)");
  lcd.setCursor(0, 1);
  lcd.print("B:Prev C:Next");
  if (step == 2) {
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

void updateRunLCD(const char* status, int maxRem) {
  lcd.setCursor(0, 0);
  lcd.print("Cy: ");
  lcd.print(currentCycle);
  lcd.print("/");
  lcd.print(cycleCount);
  
  // Show formatted time (mm:ss)
  int mins = maxRem / 60;
  int secs = maxRem % 60;
  lcd.setCursor(12, 0);
  if (mins < 10) lcd.print("0");
  lcd.print(mins);
  lcd.print(":");
  if (secs < 10) lcd.print("0");
  lcd.print(secs);

  // --- Motor 1 Progress ---
  lcd.setCursor(0, 1);
  lcd.print("M1: ");
  lcd.print(m1Started && !m1Ended ? "ON " : "OFF");

  int total1 = motor1Time * 60;
  int progress1 = total1 == 0 ? 0 : (total1 - (millis() - m1StartTime) / 1000);
  printProgressBar(6, 1, progress1, total1);

  // --- Motor 2 Progress ---
  lcd.setCursor(0, 2);
  lcd.print("M2: ");
  lcd.print(m2Started && !m2Ended ? "ON " : "OFF");

  int total2 = motor2Time * 60;
  int progress2 = total2 == 0 ? 0 : (total2 - (millis() - m2StartTime) / 1000);
  printProgressBar(6, 2, progress2, total2);

  // --- Status ---
  lcd.setCursor(0, 3);
  lcd.print("Status: ");
  lcd.print(status);
}

void printProgressBar(int col, int row, int remaining, int total) {
  lcd.setCursor(col, row);
  int bars = (total > 0) ? map(total - remaining, 0, total, 0, 10) : 0;
  for (int i = 0; i < 10; i++) {
    lcd.print(i < bars ? '#' : '-');
  }
}


void saveToEEPROM() {
  EEPROM.write(EEPROM_ADDR, 1); // Save simple flag
}

void emergencyStop() {
  running = false;
  motor1PulseDone = false;
  motor2PulseDone = false;
  resetMotorCycle();
  digitalWrite(MOTOR1_1_PIN, LOW); digitalWrite(MOTOR1_2_PIN, HIGH);
  delay(500);
  digitalWrite(MOTOR1_1_PIN, LOW); digitalWrite(MOTOR1_2_PIN, LOW);
  delay(200);
  digitalWrite(MOTOR2_1_PIN, LOW); digitalWrite(MOTOR2_2_PIN, HIGH);
  delay(500);
  digitalWrite(MOTOR2_1_PIN, LOW); digitalWrite(MOTOR2_2_PIN, LOW);
  delay(100);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCY STOP");
  delay(1000);

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
