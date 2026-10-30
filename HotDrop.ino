// --- HOT DROP MODE CODE (EXTRACTION / TIME TRIAL DEFENSE) ---

enum HotDropPhase { 
  HOT_DROP_SELECT_START, 
  HOT_DROP_SHOW_CODE, 
  HOT_DROP_LIVE_RACE, 
  HOT_DROP_HOLDING_BUTTON, 
  HOT_DROP_ENTER_CODE, 
  HOT_DROP_DEFENDING, 
  HOT_DROP_ROUND_COMPLETE, 
  HOT_DROP_PAUSE_BETWEEN_ROUNDS, 
  HOT_DROP_GAME_OVER 
};

static HotDropPhase hotDropPhase = HOT_DROP_SELECT_START;

unsigned long hotDropRoundStartTime = 0;
unsigned long HOT_DROP_DURATION = 300; // Default match duration (e.g., 5 mins)
unsigned long activeDefenseDuration = 120; // Default 2 minutes (120s), scales dynamically if running out of match time

int hotDropStartingTeam = 1; // 1 = Red goes first, 2 = Blue goes first
int currentHotDropTeam = 0;
int nextHotDropTeam = 0;

int hotDropTargetCode = 0;
char hdEnteredCode[5] = "";
int hdDigitIndex = 0;

// Hold button variables (5-second hold)
unsigned long hdButtonHoldStart = 0;
bool hdIsHolding = false;
int hdLastHoldSec = -1;

// Defense timer variables
unsigned long hdDefenseStartTime = 0;
bool defenseWasCutShort = false; // Tracks if defense scaled down due to low match time

unsigned long redHotDropTime = 0;   // Time taken by Red (in seconds)
unsigned long blueHotDropTime = 0;  // Time taken by Blue (in seconds)

void startHotDrop() {
  // 1. Prompt user for custom game duration with a 3-minute (180s) minimum check
  while (true) {
    HOT_DROP_DURATION = getGameDuration("HOT DROP TIME");
    
    if (HOT_DROP_DURATION >= 180) {
      break; // Valid game duration
    }
    
    // Error feedback if set under 3 minutes
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MIN TIME: 03:00 ");
    lcd.setCursor(0, 1);
    lcd.print("DEFEND REQS 2M+ ");
    tone(RELAY_PIN, 400, 1000);
    delay(2000);
  }

  // 2. Ask starting team: 1=Red, 2=Blue
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HOT DROP: START ");
  lcd.setCursor(0, 1);
  lcd.print("TEAM: 1=R 2=B   ");

  while (true) {
    char key = keypad.getKey();
    if (key == '1') {
      hotDropStartingTeam = 1;
      currentHotDropTeam = 1; // Red
      nextHotDropTeam = 2;    // Blue
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '2') {
      hotDropStartingTeam = 2;
      currentHotDropTeam = 2; // Blue
      nextHotDropTeam = 1;    // Red
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '*') {
      if (confirmExit()) {
        exitHotDropToMenu();
        return;
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HOT DROP: START ");
      lcd.setCursor(0, 1);
      lcd.print("TEAM: 1=R 2=B   ");
    }
  }

  // Initialize Round 1 Code
  hotDropTargetCode = random(1000, 10000);
  hotDropPhase = HOT_DROP_SHOW_CODE;
  showHotDropCodePrompt(currentHotDropTeam, hotDropTargetCode);
}

void runHotDrop() {
  char key = keypad.getKey();

  // --- 1. UNIVERSAL EXIT CHECK (* KEY) ---
  if (key == '*') {
    unsigned long menuStart = millis();
    if (confirmExit()) {
      exitHotDropToMenu();
      return;
    } else {
      unsigned long menuDuration = millis() - menuStart;
      if (hotDropPhase == HOT_DROP_LIVE_RACE || hotDropPhase == HOT_DROP_HOLDING_BUTTON || hotDropPhase == HOT_DROP_ENTER_CODE || hotDropPhase == HOT_DROP_DEFENDING) {
        hotDropRoundStartTime += menuDuration;
        if (hotDropPhase == HOT_DROP_DEFENDING) {
          hdDefenseStartTime += menuDuration;
        }
      }
      redrawHotDropScreen();
      return;
    }
  }

  // --- 2. STATE MACHINE ROUTER ---
  switch (hotDropPhase) {
    case HOT_DROP_SHOW_CODE:
      if (key == '#') {
        runLaunchCountdown();
        initHotDropRound(currentHotDropTeam);
      }
      break;

    case HOT_DROP_LIVE_RACE:
    case HOT_DROP_HOLDING_BUTTON:
      runHotDropLiveRound(currentHotDropTeam);
      break;

    case HOT_DROP_ENTER_CODE:
      runHotDropCodeEntry(currentHotDropTeam, key);
      break;

    case HOT_DROP_DEFENDING:
      runHotDropDefense(currentHotDropTeam);
      break;

    case HOT_DROP_PAUSE_BETWEEN_ROUNDS:
      if (key == '#') {
        hotDropPhase = HOT_DROP_SHOW_CODE;
        hotDropTargetCode = random(1000, 10000);
        showHotDropCodePrompt(nextHotDropTeam, hotDropTargetCode);
      }
      break;

    case HOT_DROP_GAME_OVER:
      if (key == '#') {
        exitHotDropToMenu();
      }
      break;

    default:
      break;
  }
}

// ==========================================
// HELPER FUNCTIONS
// ==========================================

void showHotDropCodePrompt(int teamNum, int codeVal) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(teamNum == 1 ? "RED CODE HIDE   " : "BLUE CODE HIDE  ");
  
  lcd.setCursor(0, 1);
  lcd.print("CODE: ");
  if (codeVal < 1000) lcd.print("0");
  if (codeVal < 100) lcd.print("0");
  if (codeVal < 10) lcd.print("0");
  lcd.print(codeVal);
  lcd.print(" [#]=OK");
}

void initHotDropRound(int teamNum) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(teamNum == 1 ? "RED HOT DROP    " : "BLUE HOT DROP   ");
  lcd.setCursor(0, 1);
  lcd.print("RACE TO DROPZONE");

  hdIsHolding = false;
  hdLastHoldSec = -1;
  defenseWasCutShort = false;
  activeDefenseDuration = 120; // Reset default to 2 minutes
  hotDropRoundStartTime = millis();
  hotDropPhase = HOT_DROP_LIVE_RACE;
  digitalWrite(RELAY_PIN, LOW);
}

void runHotDropLiveRound(int teamNum) {
  unsigned long currentMillis = millis();
  unsigned long elapsed = (currentMillis - hotDropRoundStartTime) / 1000;

  if (elapsed < HOT_DROP_DURATION) {
    long remaining = HOT_DROP_DURATION - elapsed;
    int minutes = remaining / 60;
    int seconds = remaining % 60;
    displayNumber((minutes * 100) + seconds);

    // Read team button (Red = RED_BTN, Blue = BLUE_BTN)
    bool buttonPressed = (teamNum == 1) ? (digitalRead(RED_BTN) == LOW) : (digitalRead(BLUE_BTN) == LOW);

    if (buttonPressed) {
      if (!hdIsHolding) {
        hdIsHolding = true;
        hdButtonHoldStart = currentMillis;
        hdLastHoldSec = -1;
      } else {
        unsigned long holdElapsed = (currentMillis - hdButtonHoldStart) / 1000;
        int holdSecsLeft = 5 - holdElapsed;

        if (holdSecsLeft > 0) {
          if (holdSecsLeft != hdLastHoldSec) {
            hdLastHoldSec = holdSecsLeft;
            lcd.setCursor(0, 1);
            lcd.print(teamNum == 1 ? "HOLD RED: " : "HOLD BLUE: ");
            lcd.print(holdSecsLeft);
            lcd.print("s   ");
            tone(RELAY_PIN, 1500, 30);
          }
        } else {
          // 5-SECOND HOLD COMPLETE! Transition to Code Entry
          tone(RELAY_PIN, 1200, 200);
          hotDropPhase = HOT_DROP_ENTER_CODE;
          hdDigitIndex = 0;
          memset(hdEnteredCode, 0, sizeof(hdEnteredCode));

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("ENTER CODE:     ");
          lcd.setCursor(0, 1);
          lcd.print("____            ");
          return;
        }
      }
    } else {
      // Released early -> Reset hold progress
      if (hdIsHolding) {
        hdIsHolding = false;
        lcd.setCursor(0, 1);
        lcd.print("RACE TO DROPZONE");
      }
    }

  } else {
    // TIME EXPIRED (Out of time)
    tone(RELAY_PIN, 500, 2000);
    recordHotDropTime(teamNum, HOT_DROP_DURATION);
    completeHotDropRound(teamNum);
  }
}

void runHotDropCodeEntry(int teamNum, char key) {
  unsigned long currentMillis = millis();
  unsigned long elapsed = (currentMillis - hotDropRoundStartTime) / 1000;

  if (elapsed < HOT_DROP_DURATION) {
    long remaining = HOT_DROP_DURATION - elapsed;
    int minutes = remaining / 60;
    int seconds = remaining % 60;
    displayNumber((minutes * 100) + seconds);

    if (key >= '0' && key <= '9') {
      if (hdDigitIndex < 4) {
        hdEnteredCode[hdDigitIndex] = key;
        hdDigitIndex++;
        hdEnteredCode[hdDigitIndex] = '\0';
        tone(RELAY_PIN, 1200, 50);

        lcd.setCursor(0, 1);
        lcd.print(hdEnteredCode);
        for (int i = hdDigitIndex; i < 4; i++) {
          lcd.print("_");
        }
      }
    } 
    else if (key == '#') {
      if (hdDigitIndex == 4) {
        int enteredVal = atoi(hdEnteredCode);
        if (enteredVal == hotDropTargetCode) {
          // CORRECT CODE! ARM THE PROP!
          tone(RELAY_PIN, 1500, 500);
          
          // Calculate remaining match time
          long remainingMatchTime = HOT_DROP_DURATION - elapsed;

          if (remainingMatchTime < 120) {
            // Less than 2 minutes left in game -> cap defense to remaining game time
            activeDefenseDuration = remainingMatchTime;
            defenseWasCutShort = true;
          } else {
            // Normal full 2-minute defense (120s)
            activeDefenseDuration = 120;
            defenseWasCutShort = false;
          }

          hotDropPhase = HOT_DROP_DEFENDING;
          hdDefenseStartTime = millis();

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("ARMED! DEFEND   ");
          lcd.setCursor(0, 1);
          lcd.print("DEFEND: ");
          int dMin = activeDefenseDuration / 60;
          int dSec = activeDefenseDuration % 60;
          if (dMin < 10) lcd.print("0");
          lcd.print(dMin);
          lcd.print(":");
          if (dSec < 10) lcd.print("0");
          lcd.print(dSec);
          lcd.print("   ");
        } else {
          // INCORRECT CODE ERROR
          tone(RELAY_PIN, 400, 500);
          hdDigitIndex = 0;
          memset(hdEnteredCode, 0, sizeof(hdEnteredCode));
          lcd.setCursor(0, 1);
          lcd.print("ERR  ____       ");
          delay(800);
          lcd.setCursor(0, 1);
          lcd.print("____            ");
        }
      }
    }
  } else {
    recordHotDropTime(teamNum, HOT_DROP_DURATION);
    completeHotDropRound(teamNum);
  }
}

void runHotDropDefense(int teamNum) {
  unsigned long currentMillis = millis();
  unsigned long defenseElapsed = (currentMillis - hdDefenseStartTime) / 1000;

  if (defenseElapsed < activeDefenseDuration) {
    long defenseRem = activeDefenseDuration - defenseElapsed;
    int dMin = defenseRem / 60;
    int dSec = defenseRem % 60;

    // Display remaining defense countdown on TM1637 and LCD Line 2
    displayNumber((dMin * 100) + dSec);

    lcd.setCursor(8, 1);
    if (dMin < 10) lcd.print("0");
    lcd.print(dMin);
    lcd.print(":");
    if (dSec < 10) lcd.print("0");
    lcd.print(dSec);
    lcd.print("   ");

  } else {
    // DEFENSE TIME EXPIRED!
    if (defenseWasCutShort) {
      // Defense was cut short by total match clock expiring -> max penalty score
      tone(RELAY_PIN, 500, 2000);
      recordHotDropTime(teamNum, HOT_DROP_DURATION);
    } else {
      // Successfully held for full 2 minutes -> total score = rush time + 2 minutes
      tone(RELAY_PIN, 1000, 2000); // 2-second success tone
      unsigned long totalTimeTaken = (currentMillis - hotDropRoundStartTime) / 1000;
      recordHotDropTime(teamNum, totalTimeTaken);
    }
    completeHotDropRound(teamNum);
  }
}

void recordHotDropTime(int teamNum, unsigned long timeVal) {
  if (teamNum == 1) redHotDropTime = timeVal;
  else blueHotDropTime = timeVal;
}

void completeHotDropRound(int teamNum) {
  displayNumber(0000);

  if (teamNum == hotDropStartingTeam) {
    hotDropPhase = HOT_DROP_PAUSE_BETWEEN_ROUNDS;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ROUND 1 COMPLETE");
    lcd.setCursor(0, 1);
    lcd.print("PRESS [#] FOR R2");
  } else {
    finishHotDropGame();
  }
}

void finishHotDropGame() {
  hotDropPhase = HOT_DROP_GAME_OVER;

  int rMin = redHotDropTime / 60;
  int rSec = redHotDropTime % 60;
  int bMin = blueHotDropTime / 60;
  int bSec = blueHotDropTime % 60;

  lcd.clear();
  // Line 1: Show respective times
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

  // Line 2: Winner announcement (Fastest time wins)
  lcd.setCursor(0, 1);
  if (redHotDropTime < blueHotDropTime) {
    lcd.print("WINNER: RED     ");
  } else if (blueHotDropTime < redHotDropTime) {
    lcd.print("WINNER: BLUE    ");
  } else {
    lcd.print("RESULT: DRAW    ");
  }
}

void redrawHotDropScreen() {
  switch (hotDropPhase) {
    case HOT_DROP_SELECT_START:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HOT DROP: START ");
      lcd.setCursor(0, 1);
      lcd.print("TEAM: 1=R 2=B   ");
      break;
    case HOT_DROP_SHOW_CODE:
      showHotDropCodePrompt(currentHotDropTeam, hotDropTargetCode);
      break;
    case HOT_DROP_LIVE_RACE:
    case HOT_DROP_HOLDING_BUTTON:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(currentHotDropTeam == 1 ? "RED HOT DROP    " : "BLUE HOT DROP   ");
      lcd.setCursor(0, 1);
      lcd.print("RACE TO DROPZONE");
      break;
    case HOT_DROP_ENTER_CODE:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER CODE:     ");
      lcd.setCursor(0, 1);
      lcd.print(hdDigitIndex > 0 ? hdEnteredCode : "____");
      break;
    case HOT_DROP_DEFENDING:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ARMED! DEFEND   ");
      lcd.setCursor(0, 1);
      lcd.print("DEFEND: ");
      {
        int dMin = activeDefenseDuration / 60;
        int dSec = activeDefenseDuration % 60;
        if (dMin < 10) lcd.print("0");
        lcd.print(dMin);
        lcd.print(":");
        if (dSec < 10) lcd.print("0");
        lcd.print(dSec);
        lcd.print("   ");
      }
      break;
    case HOT_DROP_PAUSE_BETWEEN_ROUNDS:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ROUND 1 COMPLETE");
      lcd.setCursor(0, 1);
      lcd.print("PRESS [#] FOR R2");
      break;
    case HOT_DROP_GAME_OVER:
      finishHotDropGame();
      break;
    default:
      break;
  }
}

void exitHotDropToMenu() {
  noTone(RELAY_PIN);
  digitalWrite(RELAY_PIN, LOW);
  currentState = MENU;
  showMenu();
}
