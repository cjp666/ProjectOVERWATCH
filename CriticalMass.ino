// --- CRITICAL MASS MODE CODE (2-ROUND TIME TRIAL WITH HOLD-TO-BREACH) ---

enum CMPhase { CM_SELECT_DEFENDER, CM_ROUND1_LIVE, CM_PAUSE_BETWEEN_ROUNDS, CM_ROUND2_LIVE, CM_GAME_OVER };
static CMPhase cmPhase = CM_SELECT_DEFENDER;

unsigned long cmRoundStartTime = 0;
unsigned long CM_DURATION = 90; // Dynamic duration per round

int round1Defender = 0; // 1 = Red defended first, 2 = Blue defended first
int round2Defender = 0;

unsigned long redSurvivalTime = 0; // Total seconds Red survived as defenders
unsigned long blueSurvivalTime = 0; // Total seconds Blue survived as defenders

// --- 5-SECOND BREACH HOLD VARIABLES ---
unsigned long breachPressStartTime = 0;
bool breachHoldingActive = false;
int lastBreachDisplayedSecond = -1;

void startCriticalMass() {
  // 1. Prompt user for custom game time per round
  CM_DURATION = getGameDuration("CRIT MASS TIME");

  // 2. Prompt for Round 1 Defending Team
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CRITICAL MASS ");
  lcd.setCursor(0, 1);
  lcd.print("DEFEND: 1=R 2=B ");

  while (true) {
    char key = keypad.getKey();
    if (key == '1') {
      round1Defender = 1; // Red defends first
      round2Defender = 2; // Blue defends second
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '2') {
      round1Defender = 2; // Blue defends first
      round2Defender = 1; // Red defends second
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '#') {
      currentState = MENU;
      showMenu();
      return;
    }
  }

  // 3. Launch Round 1 Countdown
  runLaunchCountdown();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CRITICAL MASS ");
  lcd.setCursor(0, 1);
  if (round1Defender == 1) {
    lcd.print("RED DEFENDING ");
  } else {
    lcd.print("BLUE DEFENDING ");
  }

  cmRoundStartTime = millis();
  cmPhase = CM_ROUND1_LIVE;
  breachHoldingActive = false;
  digitalWrite(RELAY_PIN, LOW);
}

void runCriticalMass() {
  unsigned long currentMillis = millis();

  // --- ROUND 1 LOGIC ---
  if (cmPhase == CM_ROUND1_LIVE) {
    unsigned long elapsed = (currentMillis - cmRoundStartTime) / 1000;

    if (elapsed < CM_DURATION) {
      long remaining = CM_DURATION - elapsed;
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      displayNumber((minutes * 100) + seconds);

      // --- 5-SECOND HOLD-TO-BREACH LOGIC ---
      int attackingTeam = (round1Defender == 1) ? 2 : 1;
      bool attackPressed = (attackingTeam == 1) ? (digitalRead(RED_BTN) == LOW) : (digitalRead(BLUE_BTN) == LOW);

      if (attackPressed) {
        if (!breachHoldingActive) {
          breachHoldingActive = true;
          breachPressStartTime = currentMillis;
          lastBreachDisplayedSecond = -1;
        }

        unsigned long holdElapsed = (currentMillis - breachPressStartTime) / 1000;
        int holdRemaining = 5 - (int)holdElapsed;

        if (holdRemaining > 0) {
          if (holdRemaining != lastBreachDisplayedSecond) {
            lastBreachDisplayedSecond = holdRemaining;
            lcd.setCursor(0, 1);
            if (attackingTeam == 1) {
              lcd.print("BREACH RED: ");
            } else {
              lcd.print("BREACH BLUE:");
            }
            lcd.print(holdRemaining);
            lcd.print("s ");
          }
        } else {
          // 5 seconds reached! Core breached.
          breachHoldingActive = false;
          if (round1Defender == 1) redSurvivalTime = elapsed;
          else blueSurvivalTime = elapsed;

          endRound1();
          return;
        }
      } else {
        // Released button early - cancel breach hold
        if (breachHoldingActive) {
          breachHoldingActive = false;
          lcd.setCursor(0, 1);
          if (round1Defender == 1) {
            lcd.print("RED DEFENDING ");
          } else {
            lcd.print("BLUE DEFENDING ");
          }
        }
      }

    } else {
      // Time expired: Defenders survived full duration
      if (round1Defender == 1) redSurvivalTime = CM_DURATION;
      else blueSurvivalTime = CM_DURATION;

      endRound1();
    }
  }

  // --- PAUSE BETWEEN ROUNDS ---
  else if (cmPhase == CM_PAUSE_BETWEEN_ROUNDS) {
    char key = keypad.getKey();
    if (key == '#') {
      runLaunchCountdown();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CRITICAL MASS ");
      lcd.setCursor(0, 1);
      if (round2Defender == 1) {
        lcd.print("RED DEFENDING ");
      } else {
        lcd.print("BLUE DEFENDING ");
      }

      cmRoundStartTime = millis();
      cmPhase = CM_ROUND2_LIVE;
      breachHoldingActive = false;
      digitalWrite(RELAY_PIN, LOW);
    }
  }

  // --- ROUND 2 LOGIC ---
  else if (cmPhase == CM_ROUND2_LIVE) {
    unsigned long elapsed = (currentMillis - cmRoundStartTime) / 1000;

    if (elapsed < CM_DURATION) {
      long remaining = CM_DURATION - elapsed;
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      displayNumber((minutes * 100) + seconds);

      int attackingTeam = (round2Defender == 1) ? 2 : 1;
      bool attackPressed = (attackingTeam == 1) ? (digitalRead(RED_BTN) == LOW) : (digitalRead(BLUE_BTN) == LOW);

      if (attackPressed) {
        if (!breachHoldingActive) {
          breachHoldingActive = true;
          breachPressStartTime = currentMillis;
          lastBreachDisplayedSecond = -1;
        }

        unsigned long holdElapsed = (currentMillis - breachPressStartTime) / 1000;
        int holdRemaining = 5 - (int)holdElapsed;

        if (holdRemaining > 0) {
          if (holdRemaining != lastBreachDisplayedSecond) {
            lastBreachDisplayedSecond = holdRemaining;
            lcd.setCursor(0, 1);
            if (attackingTeam == 1) {
              lcd.print("BREACH RED: ");
            } else {
              lcd.print("BREACH BLUE:");
            }
            lcd.print(holdRemaining);
            lcd.print("s ");
          }
        } else {
          breachHoldingActive = false;
          if (round2Defender == 1) redSurvivalTime = elapsed;
          else blueSurvivalTime = elapsed;

          endGame();
          return;
        }
      } else {
        if (breachHoldingActive) {
          breachHoldingActive = false;
          lcd.setCursor(0, 1);
          if (round2Defender == 1) {
            lcd.print("RED DEFENDING ");
          } else {
            lcd.print("BLUE DEFENDING ");
          }
        }
      }

    } else {
      if (round2Defender == 1) redSurvivalTime = CM_DURATION;
      else blueSurvivalTime = CM_DURATION;

      endGame();
    }
  }

  // --- GAME OVER SUMMARY ---
  else if (cmPhase == CM_GAME_OVER) {
    char key = keypad.getKey();
    if (key == '#') {
      noTone(RELAY_PIN);
      digitalWrite(RELAY_PIN, LOW);
      currentState = MENU;
      showMenu();
    }
  }
}

void endRound1() {
  displayNumber(0000);
  tone(RELAY_PIN, 1000, 1500);

  cmPhase = CM_PAUSE_BETWEEN_ROUNDS;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ROUND 1 COMPLETE");
  lcd.setCursor(0, 1);
  lcd.print("PRESS [#] FOR R2");
}

void endGame() {
  displayNumber(0000);
  tone(RELAY_PIN, 1000, 2000);

  cmPhase = CM_GAME_OVER;

  int rMin = redSurvivalTime / 60;
  int rSec = redSurvivalTime % 60;

  int bMin = blueSurvivalTime / 60;
  int bSec = blueSurvivalTime % 60;

  lcd.clear();
  // Line 1: R MM:SS B MM:SS (e.g. R 01:30 B 01:15)
  lcd.setCursor(0, 0);
  lcd.print("R ");
  if (rMin < 10) lcd.print("0");
  lcd.print(rMin);
  lcd.print(":");
  if (rSec < 10) lcd.print("0");
  lcd.print(rSec);

  lcd.print(" B ");
  if (bMin < 10) lcd.print("0");
  lcd.print(bMin);
  lcd.print(":");
  if (bSec < 10) lcd.print("0");
  lcd.print(bSec);

  // Line 2: Winner announcement (Longest survival time wins)
  lcd.setCursor(0, 1);
  if (redSurvivalTime > blueSurvivalTime) {
    lcd.print("WINNER: RED ");
  } else if (blueSurvivalTime > redSurvivalTime) {
    lcd.print("WINNER: BLUE ");
  } else {
    lcd.print("RESULT: DRAW ");
  }
}
