# Changelog

All notable changes to AcroClock will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
AcroClock uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added
- `requirements.md` — initial requirements specification (REQ-001..013) and a
  task backlog across 10 phases. Draft v0.1.0.
- `HISTORY.md` — project milestone log; inception entry with component,
  architecture, and open-hardware decisions.
- `CHANGELOG.md` — this file.

### Decided
- Project name: **AcroClock** (working title).
- Target MCU: STM32F103C8T6 on the existing AcroSched 2.2.0 (F103) port.
- Display: four single-digit common-cathode green seven-segment indicators
  (E11801-J-UG4-8-W), driven via a 74HC595 SPI chain + high-side source driver;
  colon from two discrete LEDs, blinking at 1 Hz; leading-zero blanking on the
  hours tens digit.
- Segment supply: bench-measured Vf ≈ 6 V @20 mA (3 dies in series) → ~6.5–7 V
  segment rail (5 V too low, 12 V not needed); full static draw ≈ 0.76 A.
- Sensors / peripherals: DS3231SN (time + temperature), BH1750FVI (ambient
  light), 24C01A EEPROM (settings with per-record CRC), BMT-1203UX buzzer.
- Input: rotary encoder via hardware timer + two buttons (Mode, Set/Enter).
- Four firmware-managed alarms with per-alarm LED indication and a 7-bit
  weekday mask (default daily).

### Open
- Segment rail voltage — depends on the 3-die internal wiring (series vs.
  parallel); datasheet table is per-chip only.
- Display drive: **static vs. dynamic (multiplex)** — affects driver topology
  and average current.
- Buzzer snooze / auto-stop timings.
- Whole-device 18650 backup (deferred, depends on the drive scheme).
