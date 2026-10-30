// --- CRITICAL MASS MODE CODE (REFACTORED) ---

enum CMPhase { CM_SELECT_DEFENDER, CM_ROUND1_LIVE, CM_PAUSE_BETWEEN_ROUNDS, CM_ROUND2_LIVE, CM_GAME_OVER };
static CMPhase cmPhase = CM_SELECT_DEFENDER;

unsigned long cmRoundStartTime = 0;
unsigned long CM_DURATION = 90; 

int round1Defender = 0; // 1 = Red, 2 = Blue
int round2Defender = 0;

unsigned long redSurvivalTime = 0;  
unsigned long blueSurvivalTime = 0; 

// --- 5-SECOND BREACH HOLD VARIABLES ---
unsigned long breachPressStartTime = 0;
bool breachHoldingActive = false;
int lastBreachDisplayedSecond = -1;

void startCriticalMass() {
  CM_DURATION = getGameDuration("CRIT MASS TIME");
  promptForDefenderSelection();
  
  runLaunchCountdown();
  initRound(1, round1Defender);
}

void runCriticalMass() {
  // 1. Handle universal exit check first
  if (handleCriticalMassExit()) return;

  // 2. Dispatch state machine
  switch (cmPhase) {
    case CM_ROUND1_LIVE:
      runLiveRound(1, round1Defender);
      break;
      
    case CM_PAUSE_BETWEEN_ROUNDS:
      runPauseState();
      break;
      
    case CM_ROUND2_LIVE:
      runLiveRound(2, round2Defender);
      break;
      
    case CM_GAME_OVER:
      runGameOverState();
      break;
      
    default:
      break;
  }
}

// ==========================================
// HELPER FUNCTIONS & SUB-STATES
// ==========================================

void promptForDefenderSelection() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CRITICAL MASS   ");
  lcd.setCursor(0, 1);
  lcd.print("DEFEND: 1=R 2=B ");

  while (true) {
    char key = keypad.getKey();
    if (key == '1') {
      round1Defender = 1; 
      round2Defender = 2; 
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '2') {
      round1Defender = 2; 
      round2Defender = 1; 
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '*') {
      if (confirmExit()) {
        exitToMenu();
        return;
      }
      // Redraw if they cancelled exit
      promptForDefenderSelection();
      return;
    }
  }
}

void initRound(int roundNum, int defendingTeam) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CRITICAL MASS   ");
  lcd.setCursor(0, 1);
  lcd.print(defendingTeam == 1 ? "RED DEFENDING   " : "BLUE DEFENDING  ");

  cmRoundStartTime = millis();
  cmPhase = (roundNum == 1) ? CM_ROUND1_LIVE : CM_ROUND2_LIVE;
  breachHoldingActive = false;
  digitalWrite(RELAY_PIN, LOW);
}

void runLiveRound(int roundNum, int defendingTeam) {
  unsigned long currentMillis = millis();
  unsigned long elapsed = (currentMillis - cmRoundStartTime) / 1000;

  if (elapsed < CM_DURATION) {
    // Update live clock
    long remaining = CM_DURATION - elapsed;
    displayNumber(((remaining / 60) * 100) + (remaining % 60));

    // Handle breach mechanics (returns true if core was breached)
    int attackingTeam = (defendingTeam == 1) ? 2 : 1;
    if (checkAndProcessBreach(attackingTeam, defendingTeam, elapsed, roundNum)) {
      return; // Round ended via breach
    }

  } else {
    // Time expired: Defenders survived full duration
    recordSurvival(defendingTeam, CM_DURATION);
    completeRound(roundNum);
  }
}

bool checkAndProcessBreach(int attackingTeam, int defendingTeam, unsigned long elapsed, int roundNum) {
  bool attackPressed = (attackingTeam == 1) ? (digitalRead(RED_BTN) == LOW) : (digitalRead(BLUE_BTN) == LOW);
  unsigned long currentMillis = millis();

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
        lcd.print(attackingTeam == 1 ? "BREACH RED:     " : "BREACH BLUE:    ");
        lcd.setCursor(12, 1);
        lcd.print(holdRemaining);
        lcd.print("s ");
      }
    } else {
      // 5 seconds reached! Core breached.
      breachHoldingActive = false;
      recordSurvival(defendingTeam, elapsed);
      
      if (roundNum == 1) completeRound(1);
      else finishGame();
      
      return true;
    }
  } else {
    // Released button early - cancel breach hold and restore status text
    if (breachHoldingActive) {
      breachHoldingActive = false;
      lcd.setCursor(0, 1);
      lcd.print(defendingTeam == 1 ? "RED DEFENDING   " : "BLUE DEFENDING  ");
    }
  }
  return false;
}

void recordSurvival(int defendingTeam, unsigned long timeVal) {
  if (defendingTeam == 1) redSurvivalTime = timeVal;
  else blueSurvivalTime = timeVal;
}

void completeRound(int roundNum) {
  displayNumber(0000);
  tone(RELAY_PIN, 1000, 1500);

  if (roundNum == 1) {
    cmPhase = CM_PAUSE_BETWEEN_ROUNDS;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ROUND 1 COMPLETE");
    lcd.setCursor(0, 1);
    lcd.print("PRESS [#] FOR R2");
  }
}

void runPauseState() {
  char key = keypad.getKey();
  if (key == '#') {
    runLaunchCountdown();
    initRound(2, round2Defender);
  }
}

void finishGame() {
  displayNumber(0000);
  tone(RELAY_PIN, 1000, 2000);
  cmPhase = CM_GAME_OVER;

  int rMin = redSurvivalTime / 60;
  int rSec = redSurvivalTime % 60;
  int bMin = blueSurvivalTime / 60;
  int bSec = blueSurvivalTime % 60;

  lcd.clear();
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

  lcd.setCursor(0, 1);
  if (redSurvivalTime > blueSurvivalTime) {
    lcd.print("WINNER: RED     "); 
  } else if (blueSurvivalTime > redSurvivalTime) {
    lcd.print("WINNER: BLUE    "); 
  } else {
    lcd.print("RESULT: DRAW    ");
  }
}

void runGameOverState() {
  char key = keypad.getKey();
  if (key == '#') {
    exitToMenu();
  }
}

bool handleCriticalMassExit() {
  char key = keypad.getKey();
  if (key == '*') {
    unsigned long menuStart = millis();
    if (confirmExit()) {
      exitToMenu();
      return true;
    } else {
      // Offset start time so match clock doesn't skip
      cmRoundStartTime += (millis() - menuStart);

      // Redraw appropriate screen based on phase
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CRITICAL MASS   ");
      lcd.setCursor(0, 1);
      if (cmPhase == CM_ROUND1_LIVE) {
        lcd.print(round1Defender == 1 ? "RED DEFENDING   " : "BLUE DEFENDING  ");
      } else if (cmPhase == CM_ROUND2_LIVE) {
        lcd.print(round2Defender == 1 ? "RED DEFENDING   " : "BLUE DEFENDING  ");
      } else if (cmPhase == CM_PAUSE_BETWEEN_ROUNDS) {
        lcd.print("PRESS [#] FOR R2");
      }
    }
  }
  return false;
}

void exitToMenu() {
  noTone(RELAY_PIN);
  digitalWrite(RELAY_PIN, LOW);
  currentState = MENU;
  showMenu();
}
