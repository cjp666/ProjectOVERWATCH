#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// --- LCD SETUP ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- TM1637 NATIVE DIRECT DRIVE ---
#define CLK 3
#define DIO 4

const uint8_t SEG_DATA[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// --- KEYPAD SETUP ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 2, 12, 11};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- BUTTONS & OUTPUTS ---
const int RED_BTN = A0;
const int BLUE_BTN = A1;
const int RELAY_PIN = A2;

// --- GAME & TEST STATES ---
enum GameState { MENU, FRONTLINE, CRITICAL_MASS, ENIGMA, HOT_DROP, SYSTEM_TEST };
GameState currentState = MENU;

void setup() {
  lcd.init();
  lcd.backlight();
 
  pinMode(CLK, OUTPUT);
  pinMode(DIO, OUTPUT);
 
  pinMode(RED_BTN, INPUT_PULLUP);
  pinMode(BLUE_BTN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  showMenu();
}

void loop() {
  char key = keypad.getKey();

  if (currentState == MENU) {
    if (key == '1') {
      currentState = FRONTLINE;
      startFrontline();
    } else if (key == '2') {
      currentState = CRITICAL_MASS;
      startCriticalMass();
    } else if (key == '3') {
      currentState = ENIGMA;
      startEnigma();
    } else if (key == '4') {
      currentState = HOT_DROP;
      startHotDrop();
    } else if (key == '9') {
      currentState = SYSTEM_TEST;
      startSystemTest();
    }
  }
  else if (currentState == FRONTLINE) {
    runFrontline();
  }
  else if (currentState == SYSTEM_TEST) {
    runSystemTest(); // This calls the function living in SystemTest.ino!
  }
}

void showMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM READY");
  lcd.setCursor(0, 1);
  lcd.print("1-4:MODE 9:TEST");
  displayNumber(8888);
}

void startCriticalMass() {
  lcd.clear();
  lcd.print("MODE: CRITICAL");
}

void startEnigma() {
  lcd.clear();
  lcd.print("MODE: ENIGMA");
}

void startHotDrop() {
  lcd.clear();
  lcd.print("MODE: HOT DROP");
}

// --- TM1637 HELPER UTILITIES ---
void tmStart() {
  digitalWrite(DIO, HIGH);
  digitalWrite(CLK, HIGH);
  digitalWrite(DIO, LOW);
}

void tmStop() {
  digitalWrite(CLK, LOW);
  digitalWrite(DIO, LOW);
  digitalWrite(CLK, HIGH);
  digitalWrite(DIO, HIGH);
}

bool tmWriteByte(uint8_t b) {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(CLK, LOW);
    digitalWrite(DIO, (b & 1) ? HIGH : LOW);
    b >>= 1;
    digitalWrite(CLK, HIGH);
  }
  digitalWrite(CLK, LOW);
  pinMode(DIO, INPUT);
  digitalWrite(CLK, HIGH);
  bool ack = digitalRead(DIO) == LOW;
  pinMode(DIO, OUTPUT);
  digitalWrite(CLK, LOW);
  return ack;
}

void displayNumber(int num) {
  tmStart();
  tmWriteByte(0x40);
  tmStop();

  tmStart();
  tmWriteByte(0xC0);
  tmWriteByte(SEG_DATA[(num / 1000) % 10]);
  tmWriteByte(SEG_DATA[(num / 100) % 10]);
  tmWriteByte(SEG_DATA[(num / 10) % 10]);
  tmWriteByte(SEG_DATA[num % 10]);
  tmStop();

  tmStart();
  tmWriteByte(0x88 | 0x07);
  tmStop();
}
