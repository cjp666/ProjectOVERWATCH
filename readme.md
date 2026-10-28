# Project: OVERWATCH
# Multi-Mode Modular Airsoft Prop 🎮💥

A professional-grade, feature-rich electronic game prop built on an **Arduino Nano** for airsoft skirmishes and milsim events. Designed with a modular software architecture, robust state-machine navigation, and tactical game mechanics.

---

## 🛠️ Hardware Stack
* **Microcontroller:** Arduino Nano
* **Display 1:** 16x2 I2C LCD (Main UI & status readouts)
* **Display 2:** TM1637 4-Digit 7-Segment Display (Live match clock / MM:SS countdown)
* **Input:** 4x4 Matrix Keypad (Dynamic time entry & navigation)
* **Controls:** 2x Arcade Push Buttons (Red Team / Blue Team inputs via `INPUT_PULLUP`)
* **Feedback:** Relay / Buzzer module (Pin `A2`) for game start/end horns and escalating countdown chirps
* **Power & Wiring:** Organized via Wago lever connectors for clean power/ground distribution

---

## 📂 Project Architecture
The codebase uses a modular multi-file structure to ensure scalability, clean separation of concerns, and easy maintenance:

```text
AirsoftProp/
│
├── sketch.ino # Main entry point, state machine router, and global utilities
├── Frontline.ino # Domination / Tug-of-War game mode logic
├── CriticalMass.ino # Asymmetrical 2-round defense/time-trial game mode logic
└── SystemTest.ino # Hardware diagnostic and testing routines
```

## 🕹️ Game Modes

### 1. Frontline (Domination / Tug-of-War) — ✅ Completed
* **Concept:** Neutral zone control. Teams must capture and hold the objective to accumulate total control time.
* **Mechanics:**
* Requires a continuous **5-second hold** on a team's arcade button to capture the point (prevents drive-by touches).
* Real-time progress feedback (`HOLD RED: 4s...`) with abort protection if buttons are released early.
* Tracks accumulated control seconds in the background every second.
* **Win Condition:** Highest total accumulated control time when the match timer expires.

### 2. Critical Mass (Asymmetrical 2-Round Defense Time-Trial) — ✅ Completed
* **Concept:** A high-stakes, role-swapping defense mode where teams compete to see who can hold out the longest against attackers.
* **Mechanics:**
* **Setup:** Enter match duration and select which team defends Round 1 (`1=Red`, `2=Blue`).
* **Hold-to-Breach:** Attackers must sustain a **5-second hold** on their button to breach the core.
* **Role Swap:** Automatically pauses between rounds, resets, and swaps roles for Round 2 via a marshal prompt (`#`).
* **Marshal Reporting:** At game over, Line 1 displays exact split times (`R 01:30 B 01:15`) for quick reporting, and Line 2 declares the winner based on **longest survival time**.

### 3. Enigma (Planned)
* **Concept:** Code-breaking / objective hacking mode.
* **Mechanics:** *(Details to be added as development begins)*

### 4. Hot Drop (Planned)
* **Concept:** Fast-paced extraction / timer rush mode.
* **Mechanics:** *(Details to be added as development begins)*
