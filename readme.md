# Project: OVERWATCH
# Multi-Mode Modular Airsoft Prop 🎮💥

A professional-grade, feature-rich electronic game prop built on an **Arduino Nano** for airsoft skirmishes and milsim events. Designed with a modular software architecture, robust state-machine navigation, and tactical game mechanics.

---

## 🛠️ Hardware Stack
- **Microcontroller:** Arduino Nano
- **Display 1:** 16x2 I2C LCD (Main UI & status readouts)
- **Display 2:** TM1637 4-Digit 7-Segment Display (Live match clock / MM:SS countdown)
- **Input:** 4x4 Matrix Keypad (Dynamic time entry, code cracking & setup)
- **Controls:** 2x Arcade Push Buttons (`RED_BTN` / `BLUE_BTN` inputs via `INPUT_PULLUP`)
- **Feedback:** Relay / Buzzer module (Pin `A2`) for game start/end horns, error buzzes, and escalating countdown chirps
- **Power & Wiring:** Organized via Wago lever connectors for clean power/ground distribution

---

## 📂 Project Architecture
The codebase uses a modular multi-file structure to ensure scalability, clean separation of concerns, and easy maintenance:

```text
AirsoftProp/
│
├── sketch.ino        # Main entry point, state machine router, random seeding, and global utilities
├── Frontline.ino     # Domination / Tug-of-War game mode logic
├── CriticalMass.ino  # Asymmetrical 2-round defense/time-trial game mode logic
├── Enigma.ino        # 4-digit code-breaking time trial game mode logic
├── HotDrop.ino       # Extraction / defense time-trial game mode logic
└── SystemTest.ino    # Hardware diagnostic and testing routines
```

## 🕹️ Game Modes

### 1. Frontline (Domination / Tug-of-War) — ✅ Completed
- **Concept:** Neutral zone control. Teams must capture and hold the objective to accumulate total control time.
- **Mechanics:**
  - Requires a continuous **5-second hold** on a team's arcade button to capture the point (prevents drive-by touches).
  - Real-time progress feedback (`HOLD RED: 4s...`) with abort protection if buttons are released early.
  - Tracks accumulated control seconds in the background every second.
- **Win Condition:** Highest total accumulated control time when the match timer expires.

### 2. Critical Mass (Asymmetrical 2-Round Defense Time-Trial) — ✅ Completed
- **Concept:** A high-stakes, role-swapping defense mode where teams compete to see who can hold out the longest against attackers.
- **Mechanics:**
  - **Setup:** Enter match duration and select which team defends Round 1 (`1=Red`, `2=Blue`).
  - **Hold-to-Breach:** Attackers must sustain a **5-second hold** on their button to breach the core.
  - **Role Swap:** Automatically pauses between rounds, resets, and swaps roles for Round 2 via a marshal prompt (`#`).
  - **Marshal Reporting:** At game over, Line 1 displays exact split times (`R 01:30 B 01:15`) for quick reporting, and Line 2 declares the winner based on **longest survival time**.

### 3. Enigma (4-Digit Code-Breaking Time Trial) — ✅ Completed
- **Concept:** A high-pressure code-cracking race where teams must find a hidden code on the field and punch it into the terminal as fast as possible.
- **Mechanics:**
  - **Setup:** Enter match duration and choose starting team (`1=Red`, `2=Blue`).
  - **Code Generation:** Prop generates a random 4-digit code for the marshal to write down and hide on the field. Pressing `#` starts the live countdown.
  - **Keypad Entry & Penalty:** Teams type the 4-digit code and hit `#`. If incorrect, it triggers an audible error tone and enforces a **10-second non-blocking lockout penalty** (displaying `LOCKED: 10s...`) while the main game clock continues ticking down.
  - **Role Swap:** Between rounds, the marshal hides a new random code for the second team.
- **Win Condition:** Team that successfully enters the code in the **fastest time** wins (lowest completion time). Out-of-time teams record the full match duration.

### 4. Hot Drop (Extraction / Defense Time Trial) — ✅ Completed
- **Concept:** A high-intensity rush to the drop zone, followed by an armed security defense phase to lock down the extraction point.
- **Mechanics:**
  - **Duration Safety Check:** Minimum match duration is restricted to **3 minutes (180s)** to ensure enough time for both the rush and the defense phase.
  - **Setup & Code:** Enter total match duration and choose starting team (`1=Red`, `2=Blue`). Marshal notes the generated random code.
  - **Drop Zone Capture:** Team races to the prop, holds their team's arcade button continuously for **5 seconds**, then enters the 4-digit code followed by `#`.
  - **Defense Phase & Dynamic Scaling:** Once armed, the team must defend the prop. Normal defense time is **2 minutes (120s)**. If fewer than 2 minutes remain on the match clock when armed, the defense duration automatically scales down to match the remaining match time.
  - **Role Swap:** Pauses between rounds for the marshal to reset the code for the next team.
- **Win Condition:** Team with the **fastest total completed time** (rush + successful defense) wins. If the clock runs out during defense, the team records the max match duration as a penalty.

---

## 🛡️ Safety & Reliability Features
- **Universal Exit (`*`):** Pressing `*` during any live game mode prompts a confirmation screen (`Exit, are you sure? 1=Y 2=N`) to prevent accidental mid-game aborts.
- **Clock Drift Protection:** If an exit prompt is cancelled by a marshal, internal timers and clock tickers automatically offset by the elapsed pause duration so the match clock never skips forward.
- **Non-Blocking Architecture:** All game modes rely on polling and timestamp checks (`millis()`) rather than blocking delays, ensuring clocks and keypads remain fully responsive.
