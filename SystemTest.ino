// --- SYSTEM TEST MODE CODE ---

unsigned long testStartTime = 0;
const unsigned long TEST_DURATION = 60; // 60 seconds (01:00 to zero)
bool testFinished = false;

void startSystemTest() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Test");
  lcd.setCursor(0, 1);
  lcd.print("Ready...");
 
  testStartTime = millis();
  testFinished = false;
  digitalWrite(RELAY_PIN, LOW);
}

void runSystemTest() {
  unsigned long elapsed = (millis() - testStartTime) / 1000;
 
  if (!testFinished) {
    if (elapsed < TEST_DURATION) {
      long remaining = TEST_DURATION - elapsed;
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      displayNumber((minutes * 100) + seconds);
    } else {
      displayNumber(0000);
      testFinished = true;
      lcd.setCursor(0, 1);
      lcd.print("TEST COMPLETE   ");
     
      // Sound buzzer / relay (using tone or digital write)
      tone(RELAY_PIN, 1000, 2000); // 1000Hz tone for 2 seconds
      delay(2000);
      digitalWrite(RELAY_PIN, LOW);
    }
  }

  // Check Red Button
  if (digitalRead(RED_BTN) == LOW) {
    lcd.setCursor(0, 1);
    lcd.print("Red             ");
  }
 
  // Check Blue Button
  if (digitalRead(BLUE_BTN) == LOW) {
    lcd.setCursor(0, 1);
    lcd.print("Blue            ");
  }

  // Press '#' to return to menu
  char key = keypad.getKey();
  if (key == '#') {
    noTone(RELAY_PIN); // Ensure sound stops if aborted early
    digitalWrite(RELAY_PIN, LOW);
    currentState = MENU;
    showMenu();
  }
}
