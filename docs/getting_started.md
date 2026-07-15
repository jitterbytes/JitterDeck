# Getting Started

Everything you need to go from zero to a running JitterDeck.

---

## Hardware Required

| Component       | Details                                       |
|-----------------|-----------------------------------------------|
| Microcontroller | ESP32-C3 SuperMini                            |
| Display         | 1.3" SH1106 OLED 128×64, I²C interface       |
| Buttons         | 3× tactile push buttons (momentary, NO)       |

---

## Wiring

### OLED Display

| OLED Pin | ESP32-C3 Pin |
|----------|--------------|
| VCC      | 3.3V         |
| GND      | GND          |
| SDA      | GPIO 8       |
| SCL      | GPIO 9       |

### Buttons

All buttons use internal `INPUT_PULLUP`. Wire each button between its GPIO pin and GND. No external resistors needed.

| Button   | ESP32-C3 Pin |
|----------|--------------|
| UP       | GPIO 0       |
| DOWN     | GPIO 1       |
| SELECT   | GPIO 3       |

---

### Button Behaviour

| Action                    | Result               |
|---------------------------|----------------------|
| Short press SELECT (tap)  | Go back / cancel     |
| Long press SELECT (700ms) | Enter / confirm      |
| UP                        | Navigate up          |
| DOWN                      | Navigate down        |

---

## Software Setup

### Prerequisites

- [VS Code](https://code.visualstudio.com/)
- [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)

Install PlatformIO via VS Code's Extensions panel (`Ctrl+Shift+X`), search **PlatformIO IDE**, install, then restart VS Code.

### Clone and Build

```bash
# Clone the repository
git clone https://github.com/yourusername/JitterDeck.git
cd JitterDeck/firmware/JitterDeck   

# open THIS folder in VS Code
code .
```

PlatformIO detects `platformio.ini` automatically. On first build it downloads the ESP32-C3 toolchain and the U8g2 display library. No manual library installation needed.

```
Build   -  Ctrl+Alt+B
Upload  -  Ctrl+Alt+U  (with ESP32-C3 connected via USB-C)
Monitor -  Ctrl+Alt+S  (115200 baud)
```

## First Boot

On successful flash, the device shows the boot animation - glitch effect resolving into **JITTERDECK**, then a chunky progress bar. After 10 seconds it transitions to the main menu. Navigate with UP/DOWN, long press SELECT to enter, short press SELECT to go back.

---

## Power Notes

**USB powerbanks with auto-shutoff:** Some powerbanks cut power to devices drawing under ~100mA. The ESP32-C3 + OLED at idle draws ~40mA, which can trigger this. If your device shuts off after ~20 seconds on a powerbank, your powerbank has auto-shutoff. Use one with an always-on mode, or power from a raw LiPo directly.

**For field use:** A 3.7V LiPo connected via a TP4056 module (with protection) gives clean regulated power with no auto-shutoff concerns. This is the recommended approach for v2.

---

## Pin Configuration

All pin assignments live in one place: `include/config.h`. If you need to change any GPIO for your specific wiring, that is the only file to edit.

```cpp
#define I2C_SDA  8
#define I2C_SCL  9
#define BTN_UP   0
#define BTN_DOWN 1
#define BTN_SELECT 3
```