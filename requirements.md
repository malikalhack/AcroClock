# AcroClock — Requirements & Task Backlog

**Project:** AcroClock
**Author:** Anton Chernov
**Date:** 2026-06-28
**Version:** 0.1.0 *(draft — inception)*

A digital desk clock built on the **AcroSched 2.7.0** cooperative scheduler.
Real-time accuracy from a DS3231 TCXO RTC, a four-digit multiplexed
seven-segment display, ambient-light brightness control, four software alarms,
and an audible buzzer.

> **Status legend:** ✅ decided · 🟡 open / TBD · ⏸ deferred to a later phase.

---

## 1. Requirements

### REQ-001 — Timekeeping ✅

The clock shall keep time using a **DS3231SN** TCXO real-time clock over I2C.

**Acceptance criteria:**
- Time is read from the DS3231 over I2C; the MCU does not free-run the clock.
- The DS3231 application driver accesses the device only through the BSP I2C
  API; MCU registers and I2C transaction sequencing remain in the BSP.
- During initial bring-up, the firmware reads the calendar registers once per
  second and reports the decoded time and read failures through the debug log.
- The RTC retains time across mains loss via its own backup cell (CR2032).
- Display format is **24-hour** (`00`–`23` hours). No 12h / AM-PM mode.
- The DS3231 `32K` output is not used.
- The DS3231 `INT/SQW` output is configured for a **1 Hz signal** and supplied
  to the MCU as an external interrupt. It is the hardware seconds event and
  is signalled to the AcroSched task via IPC; the MCU clock is not used as the
  authority for calendar values.
- `INT/SQW` is connected to `PB0/EXTI0`; the BSP detects the falling edge and
  the application task reads the RTC outside interrupt context.

---

### REQ-002 — Display ✅

The initial build shall display the current time on an **E40281-I-O-0-W**
four-digit, red, common-anode seven-segment display, driven dynamically
(multiplexed). Its character height is 7.1 mm, and the module includes a
two-LED colon.

**Acceptance criteria:**
- Normal mode shows `HH:MM` only.
- The integrated colon between hours and minutes blinks at 1 Hz (≈0.5 s on /
  0.5 s off) to convey passing seconds; blinking pauses while editing time.
- The hours **tens digit is blanked when zero** (e.g. ` 9:05`, not `09:05`).
- The display is multiplexed by firmware; the four digits are refreshed in
  turn.
- Each common anode is switched by a transistor; the MCU GPIOs directly
  control the segment cathodes through current-limiting resistors. No external
  display-driver IC is used.
- Brightness is adjusted by hardware PWM gating during every digit's scan slot;
  scan slots are not skipped for dimming.
- To blink a selected digit during editing, its digit-enable is suppressed on
  alternating blink phases while the normal scan cadence continues.
- The selected display is the initial implementation target. Scaling to other
  display sizes and the associated power design are deferred.

---

### REQ-003 — Display Driver 🟡

The display driver interface shall support the E40281-I-O-0-W common-anode
display and firmware-controlled multiplexing.

**Acceptance criteria:**
- PA0–PA7 drive the shared segment cathodes A–G and DP directly through
  current-limiting resistors. A segment is on when its GPIO is configured as
  output-low; high-impedance input mode turns it off.
- Each digit's common anode is switched individually by a transistor under
  MCU GPIO control. The firmware scans one digit at a time and blanks the
  segment outputs while changing digits to prevent ghosting.
- No display-driver IC or SPI interface is used in the initial design.
- GPIO and timer register access/configuration remains in the BSP; application
  display logic controls the hardware only through BSP interfaces.
- At 20 mA per LED chip, the datasheet specifies forward voltage of 2.0 V
  typical and 2.6 V maximum.
- Do not exceed the absolute maximum of 30 mA DC forward current per LED chip.
  The 90 mA peak rating applies only at 1/10 duty and 0.1 ms pulse width; it
  shall not be treated as a general multiplexing current limit.
- Select segment current and resistor values within both the LED ratings and
  the STM32 per-pin and aggregate GPIO current limits.
- The circuit shall use the module's integrated colon and follow the
  datasheet's internal circuit diagram and pinout.
- Global brightness control uses a dedicated hardware PWM output on
  `PB8/TIM4_CH3` to gate display current within each scan slot. It shall not
  dim by dropping whole scan slots. The transistor-level blanking connection
  is finalized with the circuit design.
- Transistor selection, resistor values, supply rails, current budget, and
  scaling remain to be determined from the electrical design and measurements.

---

### REQ-004 — Ambient Brightness Control ✅

Display brightness shall adapt automatically to ambient light.

**Acceptance criteria:**
- Ambient light is measured with a **BH1750FVI** sensor over I2C (shared bus).
- Display brightness is globally adjustable using hardware PWM on
  `PB8/TIM4_CH3`, gated within each digit's scan slot.
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
- During bring-up, the DS3231 temperature is read and reported in the debug
  log once per minute; this diagnostic log does not change display behavior.
- It is shown only on user request (via the Mode button); the normal view stays
  `HH:MM`.

---

### REQ-010 — Scheduler Core ✅

The firmware shall be built on **AcroSched 2.7.0** (cooperative dispatcher,
static memory, no RTOS).

**Acceptance criteria:**
- Functionality is split into cooperative tasks (timekeeping, brightness, UI,
  alarms) plus a hardware-timer tick as the AcroSched time source.
- Timed behaviour (colon blink, buzzer cadence, menu timeout) uses the
  **software-timer module** (`ACROSCHED_USE_TIMERS`).
- ISR-to-task signalling (e.g. button / encoder events) uses AcroSched
  **event flags / mailbox** (`ACROSCHED_USE_IPC`).
- The STM32 independent watchdog is enabled and refreshed by the AcroSched
  watchdog hook (`ACROSCHED_USE_WATCHDOG`); `acroWatchdogRefresh()` kicks
  `bspWatchdogKick()`.
- The normal idle hook is enabled (`ACROSCHED_USE_IDLE_HOOK`) to run Cortex-M
  `WFI` when no task ran. It is shallow CPU sleep only; it does not select
  tickless, STOP, or STANDBY modes.
- Deep/tickless sleep is not enabled (`ACROSCHED_USE_TICKLESS = 0`);
  the 1 kHz hardware-timer tick remains the AcroSched time source.
- Display multiplexing is performed by firmware as required by REQ-002/003.

---

### REQ-011 — Target MCU ✅

The firmware shall target the **STM32F103C8T6** (Cortex-M3, 72 MHz, LQFP48,
64 KB flash, 20 KB RAM).

**Acceptance criteria:**
- USART1 provides the debug UART on PA9/PA10; I2C1 connects the RTC on PB6/PB7
  with external bus pull-ups.
- The confirmed MCU pin allocation is:

  | MCU pin | Function | Notes |
  |---|---|---|
  | `NRST` | Shared reset | Connected to DS3231 `RST` pin 4. |
  | `PA9` | USART1_TX | Debug UART, 115200 8N1. |
  | `PA10` | USART1_RX | Debug UART, 115200 8N1. |
  | `PA0`–`PA7` | Segment cathodes | Shared `A`–`G` and `DP`, via current-limiting resistors. |
  | `PB0` | EXTI0 | DS3231 `INT/SQW`, 1 Hz falling edge; BSP pull-up to 3.3 V. |
  | `PB6` | I2C1_SCL | Shared DS3231/BH1750/24C01A bus. |
  | `PB7` | I2C1_SDA | Shared DS3231/BH1750/24C01A bus. |
  | `PB8` | TIM4_CH3 | Dedicated hardware PWM for global display brightness. |
  | `PB12` | SPI2_NSS | Reserved SPI2 bus. |
  | `PB13` | SPI2_SCK | Reserved SPI2 bus. |
  | `PB14` | SPI2_MISO | Reserved SPI2 bus. |
  | `PB15` | SPI2_MOSI | Reserved SPI2 bus. |
  | `PC13` | On-board LED | Temporary 500 ms on / 500 ms off debug blink. |
  | `PA13` | SWDIO | Reserved for ST-Link debugging. |
  | `PA14` | SWCLK | Reserved for ST-Link debugging. |

- The remaining assignments are deferred until the display and other board
  circuitry are finalized: encoder channels, Mode/Set buttons,
  transistor-control GPIOs for digit anodes, buzzer, and alarm LEDs.
  SPI2 is reserved but has no assigned peripheral in the initial display
  implementation.
- All peripherals fit the device: I2C (RTC + light + EEPROM), timer resources
  for display scanning and encoder mode, and GPIO for segment lines, digit
  transistors, buttons, alarm LEDs, buzzer, and DS3231 INT/SQW.
- The existing AcroSched Cortex-M (F103) port is used unchanged.

---

### REQ-012 — Power ⏸

The overall power architecture is deferred until after the initial display
implementation.

**Acceptance criteria:**
- The supply rails, input voltage, display current budget, and regulator/driver
  design are not fixed by the initial display requirements.
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

### REQ-014 — Debug Log ✅

The firmware shall provide a UART debug log for bring-up and diagnostics.

**Acceptance criteria:**
- The initial console uses USART1 at 115200 baud, 8 data bits, no parity, and
  one stop bit.
- Application log messages use a platform-independent logging interface;
  USART registers and UART transmission remain in the BSP.
- Startup, decoded DS3231 time, and RTC communication failures are reported.

---

## 2. Task Backlog

### Phase 0 — Foundation
- **T-001** — Finalise component list (BOM) and confirm part availability.
- **T-002** ⏸ — Determine power rails and display current budget after the
  initial display implementation.
- **T-003** — Configure the BSP UART and add the application debug-log
  interface.
- **T-004** — Confirm AcroSched F103 port + project skeleton.
- **T-005** — Implement the BSP I2C1 master interface.
- **T-006** — Add the DS3231 application driver and log the time from the
  1 Hz `INT/SQW` event through the BSP-backed debug log.
- **T-007** — Draft the remaining MCU pin map after the initial RTC bring-up.
- **T-008** — Read the DS3231 temperature and report it in the debug log once
  per minute.

### Phase 1 — Display subsystem
- **T-010** — Define the direct-GPIO segment, transistor-switched digit-anode,
  and global PWM blanking interfaces for the E40281-I-O-0-W.
- **T-011** — Bring up one digit and verify segment mapping and digit selection.
- **T-012** — Extend to all 4 digits and the colon LEDs.
- **T-013** — Segment/font lookup table and frame-buffer update routine.
- **T-014** — Implement global brightness control with hardware PWM on
  `PB8/TIM4_CH3`, without skipping scan slots.

### Phase 2 — Timekeeping
- **T-020** — Integrate DS3231 readings and configure `PB0/EXTI0` for 1 Hz IPC
  events from `INT/SQW` as the hardware seconds signal.
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
2. ⏸ **Display supply voltage and current budget** — determine during the
   deferred power-design phase for the selected display and transistor circuit.
3. ✅ ~~Leading-zero blanking~~ — **yes**, blank the hours tens zero.
4. ✅ ~~Alarm repeat model~~ — **7-bit weekday mask** (default daily).
5. 🟡 **Digit transistors, current-limiting resistors, supply, current budget,
   and scaling** — determine during circuit design and bench testing.
6. 🟡 **Snooze / auto-stop** timings for the buzzer.
7. ⏸ **Whole-device 18650 backup** — deferred until the power architecture is
   defined.

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
