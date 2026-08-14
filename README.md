# AcroClock


AcroClock

AcroClock is a compact desktop digital clock based on the AcroSched 1.7
cooperative scheduler. The project uses a DS3231 RTC over I2C, four large
green seven-segment displays driven statically, a BH1750 ambient light
sensor for adaptive brightness, an external EEPROM for persistent settings,
and a simple buzzer for alarms.

Key features:

- Precise timekeeping using the DS3231 RTC (I2C).
- Display: 4× single-digit seven-segment indicators, 24-hour format, blinking
	colon at 1 Hz, leading-zero blanking for the hours tens digit.
- Automatic brightness control: BH1750 with PWM on the `/OE` pin for global
	dimming with averaging and hysteresis.
- Alarms: four firmware-managed alarms with settings stored in 24C01A EEPROM
	(CRC-protected records).
- User interface: rotary encoder + two buttons (Mode, Set) for setting time
	and alarms; temperature display via the DS3231.
- Target MCU: STM32F103C8T6 (Cortex-M3, 72 MHz).

See `requirements.md` and `HISTORY.md` for detailed requirements, task backlog
and project history.  

## Project structure:  

```
.
├── CHANGELOG.md        changelog of releases and notable changes.
├── HISTORY.md          project history and milestone notes.
├── LICENSE             project license.
├── README.md           this file.
├── requirements.md     requirements, acceptance criteria and task backlog.
├── docs/
│   ├── Doxyfile        Doxygen configuration (project documentation settings).
│   └── mainpage.md     main documentation page for Doxygen.
├── inc/                public header files and interfaces.
├── lib/                external libraries, drivers and third-party code.
└── src/                firmware source code.
```

