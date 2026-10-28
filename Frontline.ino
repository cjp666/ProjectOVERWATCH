// --- FRONTLINE MODE CODE ---

unsigned long frontlineStartTime = 0;
const unsigned long FRONTLINE_DURATION = 90; // 90 seconds for testing
bool frontlineFinished = false;
int controllingTeam = 0; // 0 = Neutral, 1 = Red, 2 = Blue

// --- CAPTURE HOLD VARIABLES ---
unsigned long pressStartTime = 0;
int activePressTeam = 0;      // 0 = None, 1 = Red pressing, 2 = Blue pressing
bool holdingActive = false;
int lastDisplayedSecond = -1; // Used to update the LCD timer only when seconds change

void startFrontline() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("FRONTLINE ACTIVE"); // 16 chars
  lcd.setCursor(0, 1);
  lcd.print("STATUS: NEUTRAL "); // 16 chars
 
  frontlineStartTime = millis();
  frontlineFinished = false;
  controllingTeam = 0;
  activePressTeam = 0;
  holdingActive = false;
  digitalWrite(RELAY_PIN, LOW);
}

void runFrontline() {
  unsigned long currentMillis = millis();
  unsigned long elapsed = (currentMillis - frontlineStartTime) / 1000;
 
  if (!frontlineFinished) {
    if (elapsed < FRONTLINE_DURATION) {
      long remaining = FRONTLINE_DURATION - elapsed;
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      displayNumber((minutes * 100) + seconds); // Displays MMSS format
    } else {
      // Match timer hits zero
      displayNumber(0000);
      frontlineFinished = true;
     
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MATCH OVER      "); // 16 chars
      lcd.setCursor(0, 1);
      if (controllingTeam == 1) {
        lcd.print("WINNER: RED     "); // 16 chars
      } else if (controllingTeam == 2) {
        lcd.print("WINNER: BLUE    "); // 16 chars
      } else {
        lcd.print("RESULT: DRAW    "); // 16 chars
      }
     
      // End game horn/buzzer
      tone(RELAY_PIN, 1000, 2000); // 2-second end-game alarm
    }
  }

  // --- TEAM CAPTURE LOGIC (HOLD-TO-CAPTURE) ---
  if (!frontlineFinished) {
    bool redPressed = (digitalRead(RED_BTN) == LOW);
    bool bluePressed = (digitalRead(BLUE_BTN) == LOW);

    // Determine which button is currently being held
    int currentPressedTeam = 0;
    if (redPressed && !bluePressed) currentPressedTeam = 1;
    if (bluePressed && !redPressed) currentPressedTeam = 2;

    // If a button is held down and it's not already controlled by this team
    if (currentPressedTeam > 0 && controllingTeam != currentPressedTeam) {
      if (!holdingActive || activePressTeam != currentPressedTeam) {
        // First moment of press (or switched teams)
        holdingActive = true;
        activePressTeam = currentPressedTeam;
        pressStartTime = currentMillis;
        lastDisplayedSecond = -1;
      }

      // Calculate how many seconds have passed since they started holding
      unsigned long holdElapsed = (currentMillis - pressStartTime) / 1000;
      int holdRemaining = 5 - (int)holdElapsed;

      if (holdRemaining > 0) {
        // Show progress countdown on line 2 only when the second changes to prevent screen flicker
        if (holdRemaining != lastDisplayedSecond) {
          lastDisplayedSecond = holdRemaining;
          lcd.setCursor(0, 1);
          if (activePressTeam == 1) {
            lcd.print("HOLD RED: ");     // 10 chars
          } else {
            lcd.print("HOLD BLUE:");     // 10 chars
          }
          lcd.print(holdRemaining);
          lcd.print("s    ");            // Padding to fill 16 chars
        }
      } else {
        // 5 seconds reached! Capture complete.
        controllingTeam = activePressTeam;
        holdingActive = false;
        activePressTeam = 0;
       
        lcd.setCursor(0, 1);
        if (controllingTeam == 1) {
          lcd.print("STATUS: RED     "); // 16 chars
        } else {
          lcd.print("STATUS: BLUE    "); // 16 chars
        }
        tone(RELAY_PIN, 800, 300); // Longer confirmation beep for successful capture
      }
    }
    else {
      // If the button was released early, reset the hold progress
      if (holdingActive) {
        holdingActive = false;
        activePressTeam = 0;
       
        // Restore LCD back to current controller status
        lcd.setCursor(0, 1);
        if (controllingTeam == 1) {
          lcd.print("STATUS: RED     ");
        } else if (controllingTeam == 2) {
          lcd.print("STATUS: BLUE    ");
        } else {
          lcd.print("STATUS: NEUTRAL ");
        }
      }
    }
  }

  // Press '#' to abort and return to menu
  char key = keypad.getKey();
  if (key == '#') {
    noTone(RELAY_PIN);
    digitalWrite(RELAY_PIN, LOW);
    currentState = MENU;
    showMenu();
  }
}
