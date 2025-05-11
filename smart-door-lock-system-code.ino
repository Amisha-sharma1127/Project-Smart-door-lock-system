#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// RFID setup
#define SS_PIN 53
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

// Servo setup
Servo lockServo;
#define SERVO_PIN 7
#define LOCKED_ANGLE 0
#define UNLOCKED_ANGLE 90

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x3F if 0x27 doesn't work

// Bluetooth
String incomingBT = "";

// Keypad setup
#define ROWS 4
#define COLS 4
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {22, 23, 24, 25};
byte colPins[COLS] = {26, 27, 28, 29};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Credentials
const char* authorizedUID = "5316ACF8";
const char* passcode = "1234A";
String inputCode = "";

// Buzzer
#define BUZZER_PIN 6

void setup() {
  Serial.begin(9600);
  Serial1.begin(38400);

  SPI.begin();
  rfid.PCD_Init();

  lockServo.attach(SERVO_PIN);
  lockServo.write(LOCKED_ANGLE);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Lock Init");
  delay(1500);
  lcd.clear();
  lcd.print("Status: Locked");

  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("System Ready");
}

void loop() {
  checkBluetooth();
  checkRFID();
  checkKeypad();
}

void checkBluetooth() {
  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n' || c == '\r') {
      incomingBT.trim();
      if (incomingBT == "UNLOCK") {
        unlock();
      } else if (incomingBT == "LOCK") {
        lock();
      }
      incomingBT = "";
    } else {
      incomingBT += c;
    }
  }
}

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID UID:");
  lcd.setCursor(0, 1);
  lcd.print(uid);
  delay(1000);

  if (uid == authorizedUID) {
    unlock();
  } else {
    lcd.clear();
    lcd.print("Access Denied");
    Serial.println("Invalid UID: " + uid);
    beepError();
    delay(2000);
  }

  rfid.PICC_HaltA();
}

void checkKeypad() {
  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      if (inputCode == passcode) {
        unlock();
      } else {
        lcd.clear();
        lcd.print("Wrong Passcode");
        Serial.println("Wrong Passcode");
        beepError();
        delay(2000);
      }
      inputCode = "";
    } else if (key == '*') {
      inputCode = "";
      lcd.clear();
      lcd.print("Cleared");
    } else {
      inputCode += key;
      lcd.setCursor(0, 1);
      lcd.print(inputCode);
    }
  }
}

void unlock() {
  lockServo.write(UNLOCKED_ANGLE);
  delay(1000);
  lcd.clear();
  lcd.print("Unlocked");
  Serial.println("Unlocked");
  beepSuccess();
  delay(5000);
  lock();
}

void lock() {
  lockServo.write(LOCKED_ANGLE);
  delay(1000);
  lcd.clear();
  lcd.print("Locked");
  Serial.println("Locked");
  beepError();  // Just reused sound; adjust if needed
}

void beepSuccess() {
  tone(BUZZER_PIN, 2000, 100);
  delay(150);
  noTone(BUZZER_PIN);
}

void beepError() {
  tone(BUZZER_PIN, 500, 600);
  delay(600);
  noTone(BUZZER_PIN);
}