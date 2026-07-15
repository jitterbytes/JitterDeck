# Adding Sensors and Tools

How to add a new sensor profile or tool to JitterDeck for any protocol.

---

## Overview

Adding any sensor always follows the same five steps regardless of protocol. The architecture is designed so that navigation, rendering, back button, and depth tracking never need touching. You only add data and implementation.

**Total files touched: 3 existing files + 2 new files.**

---

## The Five Steps

This example adds a `XYZ123` temperature sensor to the I²C sensor profiles. Substitute your sensor name and protocol throughout.

---

### Step 1 - Add a ToolID in `include/core/menu_defs.h`

Find the correct protocol group in the `ToolID` enum and add one line:

```cpp
// I²C
TOOL_I2C_SHT31,
TOOL_I2C_XYZ123,    // < add this line
TOOL_I2C_SSD1306,
```

The value is assigned automatically by the compiler. `TOOL_COUNT` at the end of the enum updates itself. This ID is the sensor's permanent identifier throughout the entire firmware.

---

### Step 2 - Add a MenuItem in `src/core/menu_defs.cpp`

Two changes in this file:

**Add the sensor entry to the correct profiles array:**
```cpp
// In I2C_SENSOR_PROFILES[]:
{ "BME280",   TOOL_I2C_BME280,   nullptr, 0 },
{ "XYZ123",   TOOL_I2C_XYZ123,   nullptr, 0 },  // < add
{ "Custom I2C", TOOL_I2C_CUSTOM, nullptr, 0 },
```

**Update the parent branch's childCount:**
```cpp
// In I2C_MENU[] - was 8, now 9:
{ "Sensor Profiles", TOOL_NONE, I2C_SENSOR_PROFILES, 9 },
```

If you forget to update the count, the last item in the array will never be reachable by the cursor.

---

### Step 3 - Wire into `src/core/screens.cpp`

Four additions - one line in each of the four switch statements:

```cpp
// In toolName():
case TOOL_I2C_XYZ123: return "XYZ123";

// In onToolEnter():
case TOOL_I2C_XYZ123: resetXYZ123(); break;

// In drawToolScreen():
case TOOL_I2C_XYZ123: drawXYZ123(); break;

// In handleToolScreen():
case TOOL_I2C_XYZ123: handleXYZ123(); break;
```

Also add the include at the top of `screens.cpp`:
```cpp
#include "xyz123.h"
```

---

### Step 4 - Create `include/i2c/xyz123.h`

Three functions, always the same pattern:

```cpp
#ifndef XYZ123_H
#define XYZ123_H

// XYZ123 - brief description, I²C address, datasheet reference
void drawXYZ123();
void handleXYZ123();
void resetXYZ123();

#endif
```

---

### Step 5 - Create `src/i2c/xyz123.cpp`

This is the only file where sensor-specific logic lives. Structure your implementation in this order:

```cpp
// 1. Includes
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "xyz123.h"
#include "button.h"
#include "fsm.h"
#include "config.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// 2. Register map - from datasheet
static const uint8_t XYZ123_ADDR     = 0x48;
static const uint8_t REG_WHO_AM_I    = 0x0F;
static const uint8_t WHO_AM_I_VALUE  = 0xBC;
// ... all other registers

// 3. Tool state enum
enum XYZ123State { XYZ_INIT, XYZ_ERROR, XYZ_RUNNING };

// 4. Static state variables
static XYZ123State   _state      = XYZ_INIT;
static int16_t       _rawTemp    = 0;
static uint8_t       _errorCode  = 0;
static unsigned long _lastReadMs = 0;

// 5. Low-level I²C helpers
static uint8_t regWrite(uint8_t reg, uint8_t val) { ... }
static bool    regRead(uint8_t reg, uint8_t* out)  { ... }
static bool    regReadBurst(uint8_t reg, uint8_t* buf, uint8_t len) { ... }

// 6. initSensor() - WHO_AM_I check first, then config registers
static bool initSensor() { ... }

// 7. readSensor() - check data ready, burst read, scale to physical units
static bool readSensor() { ... }

// 8. Draw helpers
static void drawHeader() { ... }
static void drawHintBar() { ... }
static void drawError()   { ... }
static void drawRunning() { ... }

// 9. Public functions
void resetXYZ123()   { _state = XYZ_INIT; _rawTemp = 0; ... }
void drawXYZ123()    { u8g2.clearBuffer(); /* draw by state */ u8g2.sendBuffer(); }
void handleXYZ123()  { /* button events + state transitions */ }
```

---

## Driver Rules

These apply to every sensor driver in JitterDeck without exception.

**Always check WHO_AM_I first.**
The first thing `initSensor()` does is read the chip identification register and compare it to the expected value from the datasheet. If it does not match, set an error state and show both the expected and received values on screen. This is the single most useful thing a hardware validation tool can do - it immediately tells the user whether the right chip is on the bus.

```cpp
uint8_t whoAmI = 0;
if (!regRead(REG_WHO_AM_I, &whoAmI) || whoAmI != WHO_AM_I_VALUE)
{
    _errorCode = whoAmI;  // show what was actually received
    return false;
}
```

**Use repeated start for register reads.**
`Wire.endTransmission(false)` sends a repeated start instead of a stop condition. This is the correct I²C protocol for reading a register - write the register address, repeated start, then read. Some sensors require this and all sensors tolerate it.

```cpp
Wire.beginTransmission(ADDR);
Wire.write(reg);
Wire.endTransmission(false);  // repeated start, not stop
Wire.requestFrom(ADDR, len);
```

**Enable Block Data Update (BDU) if the sensor supports it.**
BDU holds output registers frozen from the moment you start reading until you finish. Without it you can read the low byte of one sample and the high byte of the next. Check your datasheet for this feature - it is usually a single bit in a configuration register.

**Use integer arithmetic, not float.**
Scale raw values to a fixed-point integer representation. For example, tenths of a unit: `raw * 15 / 10` gives tenths of mGauss for a 1.5 mGauss/LSB sensor. Format for display using `snprintf` with manual integer and fractional parts.

```cpp
// Good - integer only
int32_t tenths = ((int32_t)raw * 15) / 10;
snprintf(buf, sizeof(buf), "%ld.%ld", tenths/10, tenths%10);

// Avoid - floating point
float val = raw * 1.5f;
```

**No `delay()` anywhere.**
All timing is elapsed-based using `millis()`. The display must refresh every loop tick regardless of what the sensor is doing. Poll sensors on an interval:

```cpp
if (millis() - _lastReadMs >= READ_INTERVAL_MS)
{
    _lastReadMs = millis();
    readSensor();
}
```

**No Arduino `String` objects.**
Use `char buf[]` on the stack and `snprintf()`. No heap allocation.

**Always poll all three buttons every tick.**
Even if your tool only uses SELECT, still call `btnUp()` and `btnDown()` every tick. Their internal state machines must run continuously or their timing breaks.

```cpp
void handleXYZ123()
{
    const ButtonEvent evUp     = btnUp();      // must call even if unused
    const ButtonEvent evDown   = btnDown();    // must call even if unused
    const ButtonEvent evSelect = btnSelect();

    if (evSelect == BTN_EVT_SHORT_PRESS)
    {
        setActiveTool(TOOL_NONE);
        setScreen(SCREEN_MENU);
        return;
    }
    // ... rest of your logic
}
```

---

## Adding to the I²C Scanner Known Devices Table

When you add a new I²C sensor, also add its address to the known devices table in `src/i2c/i2c_scanner.cpp`. This lets the scanner identify the device by name during a bus scan.

```cpp
static const KnownDevice KNOWN_DEVICES[] =
{
    // ... existing entries
    { 0x48, "XYZ123" },   // < add your sensor
};
```

If the sensor has multiple possible addresses (e.g. address pin selectable), add one entry per address.

---

## Protocol Reference

The same five steps apply for every protocol. Only the subfolder and target array change:

| Protocol   | src subfolder    | include subfolder    | Profiles array in menu_defs.cpp  |
|------------|------------------|----------------------|----------------------------------|
| I²C        | `src/i2c/`       | `include/i2c/`       | `I2C_SENSOR_PROFILES[]`          |
| SPI        | `src/spi/`       | `include/spi/`       | `SPI_SENSOR_PROFILES[]`          |
| UART       | `src/uart/`      | `include/uart/`      | `UART_SENSOR_PROFILES[]`         |
| 1-Wire     | `src/onewire/`   | `include/onewire/`   | `ONEWIRE_SENSOR_PROFILES[]`      |
| Analog     | `src/analog/`    | `include/analog/`    | `ANALOG_SENSOR_PROFILES[]`       |
| Digital IO | `src/digital/`   | `include/digital/`   | `DIO_SENSOR_PROFILES[]`          |

When adding a new protocol subfolder, add a matching `-I include/<subfolder>` line to `build_flags` in `platformio.ini`.

---

## Adding a New Tool (not a sensor profile)

Tools like I²C Scanner, Serial Monitor, or ADC Graph are not sensor profiles - they are standalone utilities. They still follow the same five steps but go in the appropriate branch of the menu rather than a sensor profiles sub-menu.

For example, adding a new I²C tool:

```cpp
// Step 1 - menu_defs.h: add ToolID
TOOL_I2C_PULLUP_TESTER,

// Step 2 - menu_defs.cpp: add to I2C_MENU[] directly (not sensor profiles)
{ "Pullup Tester", TOOL_I2C_PULLUP_TESTER, nullptr, 0 },
// Update I2C_MENU childCount

// Steps 3–5: same as sensor addition
```