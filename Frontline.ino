// --- FRONTLINE MODE CODE (ACCUMULATED TIME / DOMINATION) ---

unsigned long frontlineStartTime = 0;
unsigned long FRONTLINE_DURATION = 90; // Set dynamically from user input
bool frontlineFinished = false;
int controllingTeam = 0; // 0 = Neutral, 1 = Red, 2 = Blue

// --- ACCUMULATED TIME TRACKING ---
unsigned long redTotalSeconds = 0;
unsigned long blueTotalSeconds = 0;
unsigned long lastSecondTick = 0;

// --- CAPTURE HOLD VARIABLES ---
unsigned long pressStartTime = 0;
int activePressTeam = 0;     
bool holdingActive = false;
int lastDisplayedSecond = -1;

void startFrontline() {
  // 1. Prompt user for custom game time
  FRONTLINE_DURATION = getGameDuration("FRONTLINE TIME");

  // 2. Run the 10-second pre-game launch sequence
  runLaunchCountdown();

  // 3. Official match start
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("FRONTLINE ACTIVE");
  lcd.setCursor(0, 1);
  lcd.print("STATUS: NEUTRAL ");
 
  frontlineStartTime = millis();
  lastSecondTick = frontlineStartTime;
  frontlineFinished = false;
  controllingTeam = 0;
  redTotalSeconds = 0;
  blueTotalSeconds = 0;
  activePressTeam = 0;
  holdingActive = false;
  digitalWrite(RELAY_PIN, LOW);
}

void runFrontline() {
  unsigned long currentMillis = millis();

  // --- UNIVERSAL EXIT CHECK (* KEY) ---
  char key = keypad.getKey();
  if (key == '*') {
    unsigned long menuStart = millis();
    if (confirmExit()) {
      noTone(RELAY_PIN);
      digitalWrite(RELAY_PIN, LOW);
      currentState = MENU;
      showMenu();
      return;
    } else {
      // Offset match start time and tick tracker so clock doesn't skip forward
      unsigned long menuDuration = millis() - menuStart;
      frontlineStartTime += menuDuration;
      lastSecondTick += menuDuration;

      // Redraw appropriate screen based on match state
      lcd.clear();
      lcd.setCursor(0, 0);
      if (frontlineFinished) {
        lcd.print("GAME OVER       ");
        lcd.setCursor(0, 1);
        if (redTotalSeconds > blueTotalSeconds) {
          lcd.print("WINNER: RED     ");
        } else if (blueTotalSeconds > redTotalSeconds) {
          lcd.print("WINNER: BLUE    ");
        } else {
          lcd.print("RESULT: DRAW    ");
        }
      } else {
        lcd.print("FRONTLINE ACTIVE");
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

  unsigned long elapsed = (currentMillis - frontlineStartTime) / 1000;
 
  if (!frontlineFinished) {
    if (elapsed < FRONTLINE_DURATION) {
      long remaining = FRONTLINE_DURATION - elapsed;
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      displayNumber((minutes * 100) + seconds); // Displays MMSS format

      // --- ACCUMULATE SCORE EVERY SECOND ---
      if (currentMillis - lastSecondTick >= 1000) {
        lastSecondTick += 1000;
        if (controllingTeam == 1) {
          redTotalSeconds++;
        } else if (controllingTeam == 2) {
          blueTotalSeconds++;
        }
      }

    } else {
      // Match timer hits zero
      displayNumber(0000);
      frontlineFinished = true;
     
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GAME OVER       ");
      lcd.setCursor(0, 1);
     
      // Compare accumulated times to determine the winner
      if (redTotalSeconds > blueTotalSeconds) {
        lcd.print("WINNER: RED     ");
      } else if (blueTotalSeconds > redTotalSeconds) {
        lcd.print("WINNER: BLUE    ");
      } else {
        lcd.print("RESULT: DRAW    ");
      }
     
      // End game horn/buzzer
      tone(RELAY_PIN, 1000, 2000);
    }
  } else {
    // If game is finished, allow pressing '#' or something to go back to menu, or let '*' handle it.
    // If you want '#' to also return to menu post-game, you can check it here:
    if (key == '#') {
      noTone(RELAY_PIN);
      digitalWrite(RELAY_PIN, LOW);
      currentState = MENU;
      showMenu();
      return;
    }
  }

  // --- TEAM CAPTURE LOGIC (HOLD-TO-CAPTURE) ---
  if (!frontlineFinished) {
    bool redPressed = (digitalRead(RED_BTN) == LOW);
    bool bluePressed = (digitalRead(BLUE_BTN) == LOW);

    int currentPressedTeam = 0;
    if (redPressed && !bluePressed) currentPressedTeam = 1;
    if (bluePressed && !redPressed) currentPressedTeam = 2;

    if (currentPressedTeam > 0 && controllingTeam != currentPressedTeam) {
      if (!holdingActive || activePressTeam != currentPressedTeam) {
        holdingActive = true;
        activePressTeam = currentPressedTeam;
        pressStartTime = currentMillis;
        lastDisplayedSecond = -1;
      }

      unsigned long holdElapsed = (currentMillis - pressStartTime) / 1000;
      int holdRemaining = 5 - (int)holdElapsed; // FIXED: Added missing minus sign

      if (holdRemaining > 0) {
        // Only update LCD if we aren't currently showing hold progress on line 2
        if (holdRemaining != lastDisplayedSecond) {
          lastDisplayedSecond = holdRemaining;
          lcd.setCursor(0, 1);
          if (activePressTeam == 1) {
            lcd.print("HOLD RED:       ");    
          } else {
            lcd.print("HOLD BLUE:      ");    
          }
          lcd.setCursor(12, 1);
          lcd.print(holdRemaining);
          lcd.print("s ");         
        }
      } else {
        // 5 seconds reached! Capture complete.
        controllingTeam = activePressTeam;
        holdingActive = false;
        activePressTeam = 0;
       
        lcd.setCursor(0, 1);
        if (controllingTeam == 1) {
          lcd.print("STATUS: RED     ");
        } else {
          lcd.print("STATUS: BLUE    ");
        }
        tone(RELAY_PIN, 800, 300);
      }
    }
    else {
      // If the button was released early, reset the hold progress (unless actively holding)
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
}
