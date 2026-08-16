# AcroClock — Project History

Milestone log for the AcroClock project.
Records key decisions, structural changes, and completed phases.

---

## 2026-06-28

### Project inception
- Deferred clock project revived. Working title **AcroClock** (AcroSched-based
  clock; name to be confirmed).
- Goal: a 24-hour desk clock on the **AcroSched 1.7** cooperative scheduler,
  with DS3231 timekeeping, large green seven-segment display, ambient-light
  brightness control, four alarms, and an audible buzzer.

### Documentation
- `requirements.md` created — initial 13 requirements (REQ-001..013) and a
  task backlog across 10 phases (Phase 0 — Foundation through Phase 9 —
  Documentation). Draft v0.1.0.
- `HISTORY.md` created (this file).
- `CHANGELOG.md` created.

### Component decisions
- **RTC** — DS3231SN (TCXO, I2C, ±2 ppm, backup cell, internal temperature
  sensor reused for on-demand temperature display).
- **Display** — four single-digit, **common-cathode**, green seven-segment
  indicators E11801-J-UG4-8-W (45.72 mm), **static** drive (no multiplexing).
- **Colon** — two discrete green LEDs between hours and minutes (the indicators
  expose a decimal point, not a colon); blinks at 1 Hz as a live "seconds" cue.
- **Ambient light** — BH1750FVI over I2C (replaces the originally considered
  photoresistor; more stable, digital, on the same bus).
- **Settings storage** — 24C01A I2C EEPROM (128 bytes), per-record **CRC** with
  fall-back to defaults on invalid CRC (lesson from a past silent
  bad-configuration bug).
- **Audible signal** — BMT-1203UX buzzer with built-in oscillator (GPIO +
  transistor switch + flyback; simple cadence, no melodies).
- **Input** — rotary encoder 3315Y-025-016L (no integral switch) read via a
  hardware timer in encoder mode, **plus two buttons** (Mode, Set/Enter).
- **MCU** — **STM32F103C8T6** (Cortex-M3, 72 MHz, LQFP48). Chosen over the
  earlier mis-pasted STM32F100R8T6B; AcroSched F103 port already exists.
- **Scheduler** — AcroSched 1.7 with the optional software-timer and IPC
  modules enabled.

### Architecture decisions
- **Static display, no software multiplex.** Single-digit indicators driven
  statically; display state changes only when a digit changes.
- **Self-made segment driver** from discrete logic: a chain of **74HC595**
  shift registers over **SPI**, followed by a **high-side source driver**
  (common-cathode) and per-segment resistors. Avoids the cost of integrated
  scan drivers (MAX7219 / HT16K33) and keeps the LQFP48 pin budget comfortable.
- **Brightness via `/OE` PWM** on the shift registers (global dimming) driven
  from BH1750 readings with averaging + hysteresis; not by varying segment
  current.
- **All four alarms in firmware** (DS3231 hardware alarms — only two — not
  relied upon).
- **24-hour format fixed** → no AM/PM indication needed.
- **Date format** — ISO 8601 (`YYYY-MM-DD`) in file headers, matching AcroSched.

### Decisions refined (same day)
- **Project name** — **AcroClock** confirmed (working title kept).
- **Leading-zero blanking** — the hours tens digit is blanked when zero
  (` 9:05`, not `09:05`).
- **Alarm repeat** — **7-bit weekday mask** per alarm (default daily); matching
  evaluated on minute change to avoid repeat triggers within the same minute.
- **Segment voltage — measured (2026-06-29).** Bench test: segment current
  limits to 20 mA at **~6 V** (three dies in series). 5 V is insufficient,
  12 V unnecessary → segment rail **~6.5–7 V**. Full static indication draws
  **~0.76 A** (0.64 A display + 2×20 mA colon + 4×20 mA alarm LEDs), cut by
  `/OE` PWM dimming. The earlier "9–12 V" figure was retracted as unfounded.

### Open hardware items
- **Static vs. dynamic display drive** — owner re-evaluating to reduce part
  count and average current; the higher segment rail is needed either way.
  This decision gates the driver topology.
- **Whole-device 18650 backup** — deferred; needs charging, protection, and
  mains/battery source switching. The DS3231 already retains time on its own
  cell during mains loss.

---

## 2026-08-15

### CMSIS solution added
- Added a CMSIS-based project solution to the repository to support
  standardized build and target configuration for STM32F103 (CMSIS device
  and toolchain integration).

## 2026-08-16

### AcroSched library update
- Added the AcroSched library version 2.2.0 to the workspace and updated
  integration notes to reference the newer scheduler release (replacing the
  prior 1.7 reference).

---
