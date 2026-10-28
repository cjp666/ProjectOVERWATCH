// --- LAUNCH COUNTDOWN HELPER ---
void runLaunchCountdown() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAME ON         ");

  // 10-second pre-game countdown
  for (int i = 10; i > 0; i--) {
    // Update LCD line 2 with remaining seconds
    lcd.setCursor(0, 1);
    lcd.print("IN ");
    lcd.print(i < 10 ? "0" : ""); // Add leading zero for single digits if desired
    lcd.print(i);
    lcd.print("s           ");

    // Also mirror the countdown seconds on the 7-segment display (e.g., 0010, 0009...)
    displayNumber(i);

    // Tone logic: Last 2 seconds (i == 2 or i == 1) get a continuous 2-second beep?
    // Wait, if it loops every second, let's look at the timing:
    if (i <= 2) {
      // For the last 2 seconds, play a longer tone or distinct beep
      tone(RELAY_PIN, 1200, 900); // Higher pitch, nearly full second beep
    } else {
      // Standard 1-second beep for seconds 10 down to 3
      tone(RELAY_PIN, 800, 200);  // Short chirp per second
    }

    delay(1000); // Wait 1 second per count
  }

  // Final zero marker
  displayNumber(0);
  noTone(RELAY_PIN);
}
