// Global Exit Confirmation Helper
bool confirmExit() {
  unsigned long promptStart = millis();

  // Save whatever was on the LCD if needed, or just overwrite with prompt
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EXIT, ARE YOU ");
  lcd.setCursor(0, 1);
  lcd.print("SURE? 1=Y 2=N ");

  while (true) {
    char key = keypad.getKey();
    if (key == '1') {
      // Yes, exit confirmed
      tone(RELAY_PIN, 600, 200);
      return true;
    }
    else if (key == '2' || key == '*') {
      // No, resume game
      tone(RELAY_PIN, 800, 100);

      // Calculate how long we spent in the menu so we can add it
      // to the game start time, preventing the match clock from jumping forward.
      unsigned long menuDuration = millis() - promptStart;

      // We will use this offset in the game loops!
      return false;
    }
  }
}
