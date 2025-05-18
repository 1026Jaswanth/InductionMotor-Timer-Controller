# InductionMotor-Timer-Controller

This project provides an Arduino Nano-based controller for managing two 3-phase induction motors using a 4x4 keypad and a 20x4 I2C LCD. It allows users to set operation times for both motors with a configurable interval gap and supports EEPROM-based task resumption after power loss.

## 🔧 Features

- ⏱️ Configure run time (1–60 minutes) for **Motor 1** and **Motor 2**
- ⏳ Set a gap time (1–5 minutes) between the motor runs
- 💾 EEPROM recovery — resume operation after unexpected power loss
- ⌨️ Simple input interface using a 4x4 keypad
- 📟 Real-time display with a 20x4 I2C LCD
- 🛑 Emergency stop using the `*` key

## 📦 Hardware Requirements

- Arduino Nano
- 4x4 Matrix Keypad
- 20x4 I2C LCD Display
- 2 Relay Modules / Motor Contactors for 3-phase motors
- External 3-phase motor drivers (if required)
- EEPROM (built-in on Nano)
- Pull-down resistors (if needed for motor pins)

## 🧩 Wiring Overview

| Component        | Arduino Nano Pin   |
|------------------|--------------------|
| LCD (I2C)        | A4 (SDA), A5 (SCL) |
| Keypad Rows      | 14, 15, 16, 17     |
| Keypad Columns   | 7, 8, 9, 10        |
| Motor 1 Control  | 3, 4               |
| Motor 2 Control  | 5, 6               |

> 💡 Ensure proper isolation and safety precautions while handling 3-phase motors.

## 🚀 How It Works

1. Upon boot, the user is prompted to **resume the last task** (if it was interrupted).
2. Navigate the setup using:
   - `B`: Previous step
   - `C`: Next step
   - `#`: Confirm entry
   - `*`: Emergency stop
   - `D`: Start the configured sequence
3. Motors are activated in sequence with the configured interval.
4. EEPROM stores the config and progress during runtime.

## 💾 EEPROM Layout

| Address | Purpose        |
|---------|----------------|
| 0       | Resume Flag (1/0) |
| 1       | Motor1 Time    |
| 2       | Motor2 Time    |
| 3       | Gap Time       |
| 4       | Current Step   |

## 🖼️ Screenshots


