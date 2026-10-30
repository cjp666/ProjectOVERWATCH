// --- ENIGMA MODE CODE (4-DIGIT CODE BREAKING TIME TRIAL) ---

enum EnigmaPhase { 
  ENIGMA_SELECT_START, 
  ENIGMA_SHOW_CODE_1, 
  ENIGMA_ROUND1_LIVE, 
  ENIGMA_PAUSE_BETWEEN_ROUNDS, 
  ENIGMA_SHOW_CODE_2, 
  ENIGMA_ROUND2_LIVE, 
  ENIGMA_GAME_OVER 
};

static EnigmaPhase enigmaPhase = ENIGMA_SELECT_START;

unsigned long enigmaRoundStartTime = 0;
unsigned long ENIGMA_DURATION = 90; // Default, set dynamically

int startingTeam = 1; // 1 = Red goes first, 2 = Blue goes first
int round1Team = 0;
int round2Team = 0;

int targetCodeRound1 = 0;
int targetCodeRound2 = 0;

char enteredCode[5] = "";
int digitIndex = 0;

unsigned long redCrackTime = 0;   // Time taken by Red (in seconds)
unsigned long blueCrackTime = 0;  // Time taken by Blue (in seconds)

// --- 10-SECOND LOCKOUT VARIABLES ---
bool isEnigmaLockedOut = false;
unsigned long enigmaLockoutEndTime = 0;
int lastLockoutSecDisplayed = -1;

void startEnigma() {
  // 1. Prompt user for custom game time
  ENIGMA_DURATION = getGameDuration("ENIGMA TIME");

  // 2. Ask starting team: 1=Red, 2=Blue
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ENIGMA: START   ");
  lcd.setCursor(0, 1);
  lcd.print("TEAM: 1=R 2=B   ");

  while (true) {
    char key = keypad.getKey();
    if (key == '1') {
      startingTeam = 1;
      round1Team = 1; // Red
      round2Team = 2; // Blue
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '2') {
      startingTeam = 2;
      round1Team = 2; // Blue
      round2Team = 1; // Red
      tone(RELAY_PIN, 800, 150);
      break;
    } else if (key == '*') {
      if (confirmExit()) {
        exitEnigmaToMenu();
        return;
      }
      // Redraw prompt if cancelled
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENIGMA: START   ");
      lcd.setCursor(0, 1);
      lcd.print("TEAM: 1=R 2=B   ");
    }
  }

  // Initialize round 1 code
  targetCodeRound1 = random(1000, 10000); // 4-digit code (1000-9999)
  enigmaPhase = ENIGMA_SHOW_CODE_1;
  showCodePrompt(round1Team, targetCodeRound1);
}

void runEnigma() {
  char key = keypad.getKey();

  // --- 1. UNIVERSAL EXIT CHECK (* KEY) ---
  if (key == '*') {
    unsigned long menuStart = millis();
    if (confirmExit()) {
      exitEnigmaToMenu();
      return;
    } else {
      unsigned long menuDuration = millis() - menuStart;
      if (enigmaPhase == ENIGMA_ROUND1_LIVE || enigmaPhase == ENIGMA_ROUND2_LIVE) {
        enigmaRoundStartTime += menuDuration;
        if (isEnigmaLockedOut) {
          enigmaLockoutEndTime += menuDuration; // Shift lockout timer too so it doesn't instantly expire
        }
      }
      redrawEnigmaScreen();
      return;
    }
  }

  // --- 2. STATE MACHINE ROUTER ---
  switch (enigmaPhase) {
    case ENIGMA_SHOW_CODE_1:
      if (key == '#') {
        runLaunchCountdown();
        initEnigmaRound(round1Team);
      }
      break;

    case ENIGMA_ROUND1_LIVE:
      runEnigmaLiveRound(round1Team, targetCodeRound1, 1, key);
      break;

    case ENIGMA_PAUSE_BETWEEN_ROUNDS:
      if (key == '#') {
        enigmaPhase = ENIGMA_SHOW_CODE_2;
        targetCodeRound2 = random(1000, 10000);
        showCodePrompt(round2Team, targetCodeRound2);
      }
      break;

    case ENIGMA_SHOW_CODE_2:
      if (key == '#') {
        runLaunchCountdown();
        initEnigmaRound(round2Team);
      }
      break;

    case ENIGMA_ROUND2_LIVE:
      runEnigmaLiveRound(round2Team, targetCodeRound2, 2, key);
      break;

    case ENIGMA_GAME_OVER:
      if (key == '#') {
        exitEnigmaToMenu();
      }
      break;

    default:
      break;
  }
}

// ==========================================
// HELPER FUNCTIONS
// ==========================================

void showCodePrompt(int teamNum, int codeVal) {
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

void initEnigmaRound(int teamNum) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(teamNum == 1 ? "RED CRACKING    " : "BLUE CRACKING   ");
  lcd.setCursor(0, 1);
  lcd.print("CODE: ____      ");

  digitIndex = 0;
  memset(enteredCode, 0, sizeof(enteredCode));
  isEnigmaLockedOut = false;
  enigmaLockoutEndTime = 0;
  lastLockoutSecDisplayed = -1;

  enigmaRoundStartTime = millis();
  enigmaPhase = (teamNum == round1Team) ? ENIGMA_ROUND1_LIVE : ENIGMA_ROUND2_LIVE;
  digitalWrite(RELAY_PIN, LOW);
}

void runEnigmaLiveRound(int teamNum, int targetCode, int roundNum, char key) {
  unsigned long currentMillis = millis();
  unsigned long elapsed = (currentMillis - enigmaRoundStartTime) / 1000;

  if (elapsed < ENIGMA_DURATION) {
    long remaining = ENIGMA_DURATION - elapsed;
    int minutes = remaining / 60;
    int seconds = remaining % 60;
    displayNumber((minutes * 100) + seconds);

    // --- HANDLE 10-SECOND LOCKOUT (NON-BLOCKING) ---
    if (isEnigmaLockedOut) {
      if (currentMillis < enigmaLockoutEndTime) {
        int lockoutRem = (enigmaLockoutEndTime - currentMillis) / 1000 + 1;
        if (lockoutRem != lastLockoutSecDisplayed) {
          lastLockoutSecDisplayed = lockoutRem;
          lcd.setCursor(0, 1);
          lcd.print("LOCKED: ");
          lcd.print(lockoutRem);
          lcd.print("s       ");
        }
        return; // Ignore keypad entry while locked out, but game clock keeps ticking!
      } else {
        // Lockout expired, restore normal entry prompt
        isEnigmaLockedOut = false;
        lcd.setCursor(0, 1);
        lcd.print("CODE: ____      ");
      }
    }

    // --- KEYPAD DIGIT ENTRY ---
    if (key >= '0' && key <= '9') {
      if (digitIndex < 4) {
        enteredCode[digitIndex] = key;
        digitIndex++;
        enteredCode[digitIndex] = '\0';
        tone(RELAY_PIN, 1200, 50); // Short keypress beep

        lcd.setCursor(6, 1);
        lcd.print(enteredCode);
        for (int i = digitIndex; i < 4; i++) {
          lcd.print("_");
        }
      }
    } 
    else if (key == '#') {
      if (digitIndex == 4) {
        int enteredVal = atoi(enteredCode);
        if (enteredVal == targetCode) {
          // CORRECT CODE!
          tone(RELAY_PIN, 1000, 1500);
          recordEnigmaTime(teamNum, elapsed);
          completeEnigmaRound(roundNum);
        } else {
          // INCORRECT CODE -> TRIGGER 10s LOCKOUT
          tone(RELAY_PIN, 400, 500);
          digitIndex = 0;
          memset(enteredCode, 0, sizeof(enteredCode));
          
          isEnigmaLockedOut = true;
          enigmaLockoutEndTime = currentMillis + 10000; // 10 seconds from now
          lastLockoutSecDisplayed = -1;

          lcd.setCursor(0, 1);
          lcd.print("LOCKED: 10s     ");
        }
      }
    }

  } else {
    // TIME EXPIRED (Out of time, time = game duration)
    tone(RELAY_PIN, 500, 2000);
    recordEnigmaTime(teamNum, ENIGMA_DURATION);
    completeEnigmaRound(roundNum);
  }
}

void recordEnigmaTime(int teamNum, unsigned long timeVal) {
  if (teamNum == 1) redCrackTime = timeVal;
  else blueCrackTime = timeVal;
}

void completeEnigmaRound(int roundNum) {
  displayNumber(0000);

  if (roundNum == 1) {
    enigmaPhase = ENIGMA_PAUSE_BETWEEN_ROUNDS;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ROUND 1 COMPLETE");
    lcd.setCursor(0, 1);
    lcd.print("PRESS [#] FOR R2");
  } else {
    finishEnigmaGame();
  }
}

void finishEnigmaGame() {
  enigmaPhase = ENIGMA_GAME_OVER;

  int rMin = redCrackTime / 60;
  int rSec = redCrackTime % 60;
  int bMin = blueCrackTime / 60;
  int bSec = blueCrackTime % 60;

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
  if (redCrackTime < blueCrackTime) {
    lcd.print("WINNER: RED     ");
  } else if (blueCrackTime < redCrackTime) {
    lcd.print("WINNER: BLUE    ");
  } else {
    lcd.print("RESULT: DRAW    ");
  }
}

void redrawEnigmaScreen() {
  switch (enigmaPhase) {
    case ENIGMA_SELECT_START:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENIGMA: START   ");
      lcd.setCursor(0, 1);
      lcd.print("TEAM: 1=R 2=B   ");
      break;
    case ENIGMA_SHOW_CODE_1:
      showCodePrompt(round1Team, targetCodeRound1);
      break;
    case ENIGMA_ROUND1_LIVE:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(round1Team == 1 ? "RED CRACKING    " : "BLUE CRACKING   ");
      lcd.setCursor(0, 1);
      if (isEnigmaLockedOut) {
        lcd.print("LOCKED          ");
      } else {
        lcd.print("CODE: ");
        lcd.print(digitIndex > 0 ? enteredCode : "____");
      }
      break;
    case ENIGMA_PAUSE_BETWEEN_ROUNDS:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ROUND 1 COMPLETE");
      lcd.setCursor(0, 1);
      lcd.print("PRESS [#] FOR R2");
      break;
    case ENIGMA_SHOW_CODE_2:
      showCodePrompt(round2Team, targetCodeRound2);
      break;
    case ENIGMA_ROUND2_LIVE:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(round2Team == 1 ? "RED CRACKING    " : "BLUE CRACKING   ");
      lcd.setCursor(0, 1);
      if (isEnigmaLockedOut) {
        lcd.print("LOCKED          ");
      } else {
        lcd.print("CODE: ");
        lcd.print(digitIndex > 0 ? enteredCode : "____");
      }
      break;
    case ENIGMA_GAME_OVER:
      finishEnigmaGame();
      break;
    default:
      break;
  }
}

void exitEnigmaToMenu() {
  noTone(RELAY_PIN);
  digitalWrite(RELAY_PIN, LOW);
  currentState = MENU;
  showMenu();
}
