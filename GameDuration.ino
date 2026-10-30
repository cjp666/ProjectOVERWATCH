// --- GLOBAL TIME INPUT HELPER ---
int getGameDuration(const char* modeTitle) {
  char inputBuffer[5] = "0000";
  int digitCount = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(modeTitle);
  lcd.setCursor(0, 1);
  lcd.print("TIME: 0000 [#OK]");

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        if (digitCount < 4) {
          inputBuffer[0] = inputBuffer[1];
          inputBuffer[1] = inputBuffer[2];
          inputBuffer[2] = inputBuffer[3];
          inputBuffer[3] = key;
          digitCount++;

          lcd.setCursor(6, 1);
          lcd.print(inputBuffer);
        }
      }
      else if (key == '*') {
        inputBuffer[0] = '0'; inputBuffer[1] = '0'; inputBuffer[2] = '0'; inputBuffer[3] = '0';
        digitCount = 0;
        lcd.setCursor(6, 1);
        lcd.print(inputBuffer);
      }
      else if (key == '#') {
        int totalSeconds = (inputBuffer[0] - '0') * 600 +
                           (inputBuffer[1] - '0') * 60 +
                           (inputBuffer[2] - '0') * 10 +
                           (inputBuffer[3] - '0');
        if (totalSeconds > 0) {
          // FLUSH BUFFER: Clear out any extra lingering keypresses (like a second #)
          while (keypad.getKey() != NO_KEY);

          return totalSeconds;
        }
      }
    }
  }
}