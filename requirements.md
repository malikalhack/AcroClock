# AcroClock — Requirements & Task Backlog

**Project:** AcroClock
**Author:** Anton Chernov
**Date:** 2026-06-28
**Version:** 0.1.0 *(draft — inception)*

A digital desk clock built on the **AcroSched 2.2.0** cooperative scheduler.
Real-time accuracy from a DS3231 TCXO RTC, four large green seven-segment
displays driven statically, ambient-light brightness control, four
software alarms, and an audible buzzer.

> **Status legend:** ✅ decided · 🟡 open / TBD · ⏸ deferred to a later phase.

---

## 1. Requirements

### REQ-001 — Timekeeping ✅

The clock shall keep time using a **DS3231SN** TCXO real-time clock over I2C.

**Acceptance criteria:**
- Time is read from the DS3231 over I2C; the MCU does not free-run the clock.
- The RTC retains time across mains loss via its own backup cell (CR2032).
- Display format is **24-hour** (`00`–`23` hours). No 12h / AM-PM mode.
- The DS3231 1 Hz / SQW output is available to the MCU (drives the colon blink
  and/or a 1-second tick), 🟡 exact use confirmed in Phase 2.

---

### REQ-002 — Display ✅

The clock shall display the current time on **four single-digit, common-cathode,
green seven-segment indicators** (E11801-J-UG4-8-W, 45.72 mm character height),
driven **statically** (no multiplexing).

**Acceptance criteria:**
- Normal mode shows `HH:MM` only.
- A **colon** between hours and minutes is implemented with **two discrete
  green LEDs** (the indicators provide a decimal point, not a colon).
- The colon **blinks at 1 Hz** (≈0.5 s on / 0.5 s off) to convey passing
  seconds; blinking pauses while editing time.
- The hours **tens digit is blanked when zero** (e.g. ` 9:05`, not `09:05`).

---

### REQ-003 — Display Driver 🟡

The segment driver shall be a **self-made** design (no integrated scan driver
such as MAX7219 / HT16K33), built from discrete logic to keep cost down.

**Acceptance criteria:**
- Static drive of ~32 segment lines is achieved through a chain of **74HC595**
  shift registers fed over **SPI** (clock, data, latch), keeping the MCU pin
  count low.
- Per-segment current is supplied by a **high-side source driver**
  (common-cathode geometry; e.g. TBD62783 / UDN2981 class) plus a
  current-limiting resistor per segment.
- **Segment supply voltage — measured:** segment current limits to 20 mA at
  **~6 V** (three dies in series), so 5 V is **insufficient** and 12 V is
  unnecessary; a **~6.5–7 V** rail covers Vf + source-driver + resistor drop.
  Full static draw ≈ **0.76 A** (0.64 A display + 2×20 mA colon + 4×20 mA alarm
  LEDs), reduced by `/OE` PWM dimming.
- 🟡 **Static vs. dynamic (multiplexed) drive** is being reconsidered to cut
  total current and part count; the chosen scheme drives the whole driver
  design. *(Owner deciding.)*
- 🟡 The 4 alarm LEDs and the colon LEDs **may** be folded into the same
  shift-register chain to save GPIO — decided during schematic design.

---

### REQ-004 — Ambient Brightness Control ✅

Display brightness shall adapt automatically to ambient light.

**Acceptance criteria:**
- Ambient light is measured with a **BH1750FVI** sensor over I2C (shared bus).
- Brightness is set by **PWM on the shift-register `/OE`** line (global dimming),
  not by changing segment currents.
- The brightness mapping uses **averaging and hysteresis** so the display does
  not flicker or hunt under changing/shadowed light.
- A sensible **minimum brightness** keeps the clock readable in darkness.

---

### REQ-005 — Alarms ✅

The clock shall provide **four independent alarms**, fully managed in MCU
firmware (the DS3231's two hardware alarms are not relied upon).

**Acceptance criteria:**
- Each alarm stores at least: enabled flag and time (`HH:MM`).
- Each alarm has a dedicated **LED indicator** showing its enabled/selected state.
- When an alarm fires it triggers the audible buzzer (REQ-006).
- Alarm settings persist across power loss (REQ-008).
- Each alarm carries a **7-bit weekday mask** selecting the days it is active;
  default is **daily** (all days). The user does **not** edit individual bits —
  the mask is chosen from a small set of **presets** (see §4).
- An optional **one-shot** flag lets an alarm fire once and then auto-disable.
- Matching is evaluated **on minute change** (or via a last-fired-minute guard)
  so an alarm does not re-trigger throughout the matching minute.

---

### REQ-006 — Audible Signal ✅

The clock shall produce an audible alarm using a **BMT-1203UX** buzzer with a
built-in oscillator.

**Acceptance criteria:**
- The buzzer is driven by a single GPIO through a transistor switch with a
  flyback diode (no PWM tone generation needed — the buzzer self-oscillates).
- A simple beep **pattern** (on/off cadence) is produced via the scheduler's
  software timers; elaborate melodies are out of scope.
- 🟡 Snooze and auto-stop timeout behaviour — TBD in Phase 5.

---

### REQ-007 — User Input ✅

The user shall set the time and alarms using a **rotary encoder plus two
push-buttons**.

**Acceptance criteria:**
- The encoder (3315Y-025-016L, no integral switch) is read through a hardware
  **timer in encoder-interface mode**; the UI task reads the timer counter.
- Two buttons: **Mode** (select field / cycle views, incl. temperature) and
  **Set/Enter** (enter / confirm). Short vs. long press may be used.
- A UI **state machine** governs normal display, time-set, and alarm-set modes.
- Alarm editing is a per-field walk (enable → hours → minutes → days), the edited
  field blinking; the **days** field cycles **day presets** shown as a
  2-character code (§4), not bit-by-bit editing — a deliberate fit for the
  4-digit display.
- The UI **times out** back to normal display after inactivity.

---

### REQ-008 — Settings Persistence ✅

User settings shall survive a full power loss.

**Acceptance criteria:**
- Settings (4 alarms + configuration) are stored in an external **24C01A**
  I2C EEPROM (128 bytes).
- Each stored record is protected by a **CRC**; an invalid CRC on load causes
  that record to fall back to safe defaults rather than using corrupt data.
- Writes occur only on user change (EEPROM endurance respected).

---

### REQ-009 — Temperature Display ✅

The clock shall be able to show ambient temperature on demand.

**Acceptance criteria:**
- Temperature is read from the **DS3231's internal sensor** (0.25 °C resolution).
- It is shown only on user request (via the Mode button); the normal view stays
  `HH:MM`.

---

### REQ-010 — Scheduler Core ✅

The firmware shall be built on **AcroSched 2.2.0** (cooperative dispatcher,
static memory, no RTOS).

**Acceptance criteria:**
- Functionality is split into cooperative tasks (timekeeping, brightness, UI,
  alarms) plus a hardware-timer tick as the AcroSched time source.
- Timed behaviour (colon blink, buzzer cadence, menu timeout) uses the
  **software-timer module** (`ACROSCHED_USE_TIMERS`).
- ISR-to-task signalling (e.g. button / encoder events) uses AcroSched
  **event flags / mailbox** (`ACROSCHED_USE_IPC`).
- No display multiplexing in software (static drive — see REQ-002/003).

---

### REQ-011 — Target MCU ✅

The firmware shall target the **STM32F103C8T6** (Cortex-M3, 72 MHz, LQFP48,
64 KB flash, 20 KB RAM).

**Acceptance criteria:**
- All peripherals fit the device: SPI (display), I2C (RTC + light + EEPROM),
  one timer in encoder mode, a PWM channel for `/OE` brightness, and GPIO for
  buttons, alarm LEDs, buzzer, and DS3231 INT/SQW.
- The existing AcroSched Cortex-M (F103) port is used unchanged.

---

### REQ-012 — Power ⏸

The clock is powered from a **5 V** board input.

**Acceptance criteria:**
- Nominal operation from the 5 V rail (logic).
- The display segments need a **~6.5–7 V** rail (segment Vf ≈ 6 V @20 mA,
  measured); board input voltage is decided with the display-drive scheme.
- The DS3231 keeps time on its own backup cell during mains loss.
- ⏸ **Optional whole-device battery backup (18650)** — charging (e.g. TP4056),
  protection, and source switching (mains/battery) are **deferred to a later
  phase**; if added, the display must remain operational on battery, with
  aggressive auto-dimming to limit current.

---

### REQ-013 — Code Style, Documentation, Versioning ✅

The project shall follow the conventions established by AcroSched.

**Acceptance criteria:**
- C source follows the project style guide (single entry/exit with `ret_val`,
  declarations at block start, `__attribute__((weak))`, etc.).
- Public interfaces documented with Doxygen tags.
- **Semantic Versioning** for the firmware; `HISTORY.md` and `CHANGELOG.md`
  maintained in the AcroSched style.
- File header carries two `@date` fields (creation date — never changed — and
  the Doxygen build date).

---

## 2. Task Backlog

### Phase 0 — Foundation
- **T-001** — Finalise component list (BOM) and confirm part availability.
- **T-002** ✅ — Segment wiring & Vf measured on the bench: 20 mA limit at ~6 V
  (3 dies in series) → segment rail ~6.5–7 V; full static draw ≈ 0.76 A.
- **T-003** — Draft MCU pin map (SPI / I2C / encoder timer / PWM / GPIO).
- **T-004** — Confirm AcroSched F103 port + project skeleton.

### Phase 1 — Display subsystem
- **T-010** — Design self-made static driver schematic (74HC595 chain +
  high-side source driver + segment resistors).
- **T-011** — Bring up a single digit (static), verify brightness & current.
- **T-012** — Extend to 4 digits + colon LEDs over SPI.
- **T-013** — Segment/font lookup table and frame-buffer update routine.
- **T-014** — `/OE` PWM channel for global brightness.

### Phase 2 — Timekeeping
- **T-020** — DS3231 I2C driver (time read/write, BCD handling).
- **T-021** — Periodic time read → frame buffer; 1 Hz colon blink.
- **T-022** — Time-set UI integration (hours/minutes, optional seconds reset).

### Phase 3 — Brightness
- **T-030** — BH1750FVI I2C driver (lux read).
- **T-031** — Brightness mapping with averaging + hysteresis + night floor.

### Phase 4 — Input / UI
- **T-040** — Encoder via hardware timer (encoder mode); counter read.
- **T-041** — Two-button input with debounce (short/long press).
- **T-042** — UI state machine (normal / set-time / set-alarm / show-temp).
- **T-043** — Inactivity timeout back to normal view.

### Phase 5 — Alarms & sound
- **T-050** — Alarm data model (4 alarms) and matching logic (per minute).
- **T-051** — 4 alarm LED indicators.
- **T-052** — Buzzer driver (GPIO + transistor + flyback) and beep pattern.
- **T-053** — Snooze / auto-stop behaviour.

### Phase 6 — Persistence
- **T-060** — 24C01A I2C EEPROM driver (paged writes, busy poll).
- **T-061** — Settings records with CRC; defaults on invalid CRC.

### Phase 7 — Integration on AcroSched
- **T-070** — Define task set, periods, priorities; wire software timers + IPC.
- **T-071** — System integration test on hardware.

### Phase 8 — Power & enclosure ⏸
- **T-080** — 18650 charging / protection / source switching *(deferred)*.
- **T-081** — Enclosure and indicator mounting.

### Phase 9 — Documentation
- **T-090** — Doxygen comments across public interfaces.
- **T-091** — Schematic, pin map, and user notes in `README.md`.

---

## 3. Open Questions

1. ✅ ~~Project name~~ — **AcroClock** confirmed (working title kept).
2. ✅ ~~Segment supply voltage~~ — **measured:** Vf ≈ 6 V @20 mA → ~6.5–7 V
   rail (5 V too low, 12 V not needed); full static draw ≈ 0.76 A.
3. ✅ ~~Leading-zero blanking~~ — **yes**, blank the hours tens zero.
4. ✅ ~~Alarm repeat model~~ — **7-bit weekday mask** (default daily).
5. 🟡 **Display drive: static vs. dynamic (multiplex)** — owner evaluating;
   affects driver topology and average current.
6. 🟡 **Snooze / auto-stop** timings for the buzzer.
7. ⏸ **Whole-device 18650 backup** — deferred (depends on the drive scheme).

---

## 4. Alarm Weekday Mask

Each alarm stores a **7-bit `dayMask`**, one bit per day of week; bit set = the
alarm is active that day. The day index is derived from the DS3231 day-of-week
register (1–7), mapped to `0 = Mon … 6 = Sun`.

| Constant | Value | Days |
|---|---|---|
| `DOW_DAILY` | `0x7F` | all 7 days |
| `DOW_WEEKDAYS` | `0x1F` | Mon–Fri |
| `DOW_WEEKEND` | `0x60` | Sat, Sun |

An alarm fires when `enabled` **and** `hour:minute` match the current time
**and** the bit for today is set — evaluated on minute change. `dayMask` is part
of the CRC-protected EEPROM record (REQ-008).

### 4.1 Day selection UI — presets (decided)

Because the 4-digit display cannot show per-day toggles legibly, the user does
**not** edit the 7 bits directly. In the alarm "days" field the encoder cycles a
fixed list of presets, each rendered as a 2-character code on the display:

| Display code | Preset | Resulting `dayMask` / flag |
|---|---|---|
| `7d` | daily | `DOW_DAILY` (`0x7F`) |
| `5d` | weekdays (Mon–Fri) | `DOW_WEEKDAYS` (`0x1F`) |
| `2d` | weekend (Sat–Sun) | `DOW_WEEKEND` (`0x60`) |
| `1d` | once | `oneShot` flag set (auto-disable after firing) |

All code glyphs (`1 2 5 7 d`) render on a seven-segment digit. `Set` confirms
the selection. A future "advanced" mode (full per-day editing via the 7 segments
of one digit, with an enclosure legend) may be added later but is **out of scope
for the initial UI**.
