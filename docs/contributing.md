# Contributing

Thank you for contributing to JitterDeck. This document covers everything you need to know before opening a pull request.

---

## Before You Start

Read the following documents first - they explain the decisions behind the codebase and prevent the most common contribution mistakes:

- [Codebase Guide](codebase_guide.md) - how the firmware is structured and the reading order
- [Adding Sensors](adding_sensors.md) - the exact five-step process for adding any sensor or tool
- [Design Philosophy](design_philosophy.md) - why the firmware is built the way it is

---

## Types of Contributions

### Sensor profiles
Adding a driver for a supported sensor in any protocol category. This is the most common and most welcome contribution. Follow the five-step process in [Adding Sensors](adding_sensors.md) exactly.

### Protocol tools
Adding a new utility tool (e.g. a SPI bus analyser, a UART baud rate detector). Same five-step process, but the `MenuItem` goes directly into the protocol menu rather than into a sensor profiles sub-menu.

### Bug fixes
Open an issue first describing the bug and the conditions that trigger it. Include what hardware you tested on.

### New features
Open an issue before writing code to discuss whether the feature fits the project's scope. JitterDeck is a hardware validation tool - features should serve that purpose directly.

---

## Code Rules

These are non-negotiable. PRs that violate them will be asked to revise before merging.

### No `delay()`
All timing is elapsed-based using `millis()`. The loop must never block. If you need to wait for something, check elapsed time on the next tick.

```cpp
// Wrong
delay(100);

// Correct
if (millis() - _lastMs >= 100) { _lastMs = millis(); doThing(); }
```

### No Arduino `String` objects
Use `char buf[]` on the stack and `snprintf()`. No heap allocation anywhere.

```cpp
// Wrong
String s = "Address: 0x" + String(addr, HEX);

// Correct
char buf[16];
snprintf(buf, sizeof(buf), "Address: 0x%02X", addr);
```

### No floating point in sensor math
Scale raw values to fixed-point integers. Use tenths, hundredths, or thousandths of the physical unit as integers. Format for display with `snprintf`.

```cpp
// Wrong
float temp = raw * 0.0625f;

// Correct - tenths of a degree
int32_t tempTenths = ((int32_t)raw * 625) / 1000;
```

### Always check WHO_AM_I
Every I²C and SPI sensor driver must read the chip identification register on init and compare it against the expected datasheet value. If it does not match, transition to an error state and display both the expected and received values.

### Always poll all three buttons
Even if your tool only uses SELECT, call `btnUp()` and `btnDown()` every tick. Their internal state machines must run continuously.

### Define children before parents in `menu_defs.cpp`
Arrays must be defined before the arrays that reference them. Always define the deepest level first and work up toward `MENU_MAIN`.

### Mark sub-arrays `static` in `menu_defs.cpp`
Every array in `menu_defs.cpp` is `static` except `MENU_MAIN`. This keeps them local to the file. `MENU_MAIN` is the only public entry point.

### File placement
- Headers → `include/<category>/`
- Implementations → `src/<category>/`
- New category subfolders → add a matching `-I include/<category>` line to `build_flags` in `platformio.ini`

### Naming conventions
| Thing | Convention | Example |
|-------|-----------|---------|
| ToolID enum values | `TOOL_PROTOCOL_SENSORNAME` | `TOOL_I2C_XYZ123` |
| Public functions | `camelCase` with module prefix | `drawXYZ123()`, `handleXYZ123()`, `resetXYZ123()` |
| Static variables | `_camelCase` with leading underscore | `_rawTemp`, `_lastReadMs` |
| Constants | `UPPER_SNAKE_CASE` | `REG_WHO_AM_I`, `READ_INTERVAL_MS` |
| Header guards | `FILENAME_H` | `#ifndef XYZ123_H` |

---

## Checklist Before Opening a PR

Go through this list before submitting:

- [ ] Tested on real hardware (not just compiled)
- [ ] WHO_AM_I check implemented and shows expected vs received on failure
- [ ] No `delay()` anywhere in new code
- [ ] No `String` objects anywhere in new code
- [ ] All three buttons polled every tick in `handleXYZ()` 
- [ ] `resetXYZ()` zeroes all static state variables
- [ ] Children arrays defined before parent arrays in `menu_defs.cpp`
- [ ] Sub-arrays marked `static` in `menu_defs.cpp`
- [ ] `childCount` updated in parent branch after adding to array
- [ ] Header in `include/<category>/`, implementation in `src/<category>/`
- [ ] Sensor I²C address added to `KNOWN_DEVICES[]` in `i2c_scanner.cpp` (I²C sensors only)
- [ ] `#include "xyz123.h"` added to `screens.cpp`
- [ ] All four switch statements in `screens.cpp` updated: `toolName`, `onToolEnter`, `drawToolScreen`, `handleToolScreen`
- [ ] New `include/` subfolder added to `build_flags` in `platformio.ini` if created

---

## Getting Help

Open an issue with the **question** label if anything is unclear. Include which sensor you are trying to add and what step you are on.