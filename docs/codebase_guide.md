# Codebase Guide

How the JitterDeck firmware is structured and how to read it from scratch.

---

## Project Structure

```
JitterDeck/
│
├── platformio.ini          # Board, framework, library dependencies
│
├── include/                # All header files (.h)
│   ├── config.h            # Pin definitions and global constants (read first)
│   ├── core/               # Firmware infrastructure
│   │   ├── fsm.h           # Finite state machine
│   │   ├── nav_stack.h     # Navigation stack
│   │   ├── button.h        # Button input driver
│   │   ├── menu_defs.h     # Menu tree data structures
│   │   ├── menu.h          # Carousel menu renderer
│   │   └── screens.h       # Tool screen dispatcher
│   ├── ui/
│   │   └── boot_screen.h   # Boot animation
│   ├── i2c/                # I²C tool headers
│   ├── spi/                # SPI tool headers
│   ├── uart/               # UART tool headers
│   ├── analog/             # Analog tool headers
│   ├── digital/            # Digital IO tool headers
│   ├── wireless/           # WiFi + BLE tool headers
│   └── system/             # System tool headers
│
└── src/                    # All implementation files (.cpp)
    ├── main.cpp            # Entry point - setup() and loop() only
    ├── core/               # Mirrors include/core/
    ├── ui/                 # Mirrors include/ui/
    ├── i2c/                # Mirrors include/i2c/
    └── ...
```

**The single rule:** every `.h` lives in `include/<category>/`, every `.cpp` lives in `src/<category>/`. They always mirror each other. Never put a header next to its `.cpp`.

---

## Reading Order

Read the files in this exact order. Each one builds on the previous. Jumping around makes things harder than they need to be.

---

### 1. `include/config.h`

**Start here.** Every pin number and timing constant in the entire firmware lives in this one file. Nothing is hardcoded anywhere else. If you want to know what GPIO a button is on, or how long the boot animation runs, this is the only file you check.

---

### 2. `include/core/menu_defs.h`

Two things to understand here:

**`ToolID` enum** - a numbered list where every tool in the firmware has a permanent unique ID stored as a single byte (`uint8_t`). Two special values:
- `TOOL_NONE = 0` - this is a branch node (has children, navigates deeper)
- `TOOL_COUNT` - sits at the end, its value equals the total number of tools

**`MenuItem` struct** - four fields that double as both a branch and a leaf using the same struct:

```cpp
struct MenuItem {
    const char*      label;       // text shown on screen
    ToolID           toolID;      // TOOL_NONE = branch, anything else = leaf
    const MenuItem*  children;    // child array (branch only), else nullptr
    uint8_t          childCount;  // length of children array
};
```

When `menu.cpp` picks up a `MenuItem` it checks `toolID` first. `TOOL_NONE` - push children onto the nav stack. Anything else - launch a tool screen.

---

### 3. `src/core/menu_defs.cpp`

The entire menu tree as `const` data stored in flash - zero RAM cost. Three things to notice:

**Bottom-up definition order.** Children are always defined before the parents that reference them. If you wrote `I2C_MENU` before `I2C_SENSOR_PROFILES`, the compiler would throw an unknown identifier error. Always define the child array first.

**`static` on everything except `MENU_MAIN`.** This keeps every sub-array local to this file - invisible to other files. Only `MENU_MAIN` is the public entry point the rest of the firmware needs.

**Two node patterns:**
```cpp
// Leaf node - launches a tool screen
{ "BME280", TOOL_I2C_BME280, nullptr, 0 }

// Branch node - pushes child menu onto the nav stack
{ "Sensor Profiles", TOOL_NONE, I2C_SENSOR_PROFILES, 8 }
```

---

### 4. `include/core/fsm.h` + `src/core/fsm.cpp`

The finite state machine. Holds exactly two variables: the current top-level screen state and the active tool ID.

**Three states only:**
```
SCREEN_BOOT   -  boot animation is running
SCREEN_MENU   -  user is navigating menus (nav_stack handles depth)
SCREEN_TOOL   -  a leaf tool is active and running
```

**The FSM never drives itself.** It only stores where you are. Other files push state into it:

```
main.cpp    - reads  getScreen()           every loop tick
main.cpp    - calls  setScreen(SCREEN_MENU) after boot timeout
menu.cpp    - calls  setActiveTool()       when user long-presses a leaf
menu.cpp    - calls  setScreen(SCREEN_TOOL) immediately after
screens.cpp - reads  getActiveTool()       to know which tool to draw
screens.cpp - calls  setScreen(SCREEN_MENU) when user taps back
```

---

### 5. `include/core/nav_stack.h` + `src/core/nav_stack.cpp`

The navigation engine. A fixed 8-frame stack with zero heap allocation. Each frame stores:

```cpp
struct NavFrame {
    const MenuItem* items;   // pointer into menu_defs data (no copy)
    uint8_t         count;   // how many items in this level
    uint8_t         cursor;  // which item is currently selected
    const char*     title;   // header string shown at top of screen
};
```

`navPush()` goes deeper. `navPop()` goes back. Cursor positions are preserved at every level - going back always lands exactly where you left off.

The stack never copies menu data. It only holds pointers and indices. The actual menu arrays stay untouched in flash the entire time.

**Memory cost:** 8 frames × 10 bytes = **80 bytes of RAM** for the entire navigation system across 60+ menu items.

---

### 6. `include/core/button.h` + `src/core/button.cpp`

Non-blocking button manager. No `delay()` anywhere.

Every call to `btnUp()`, `btnDown()`, or `btnSelect()` returns one of:
```
BTN_EVT_NONE         -  nothing happened this tick
BTN_EVT_SHORT_PRESS  -  tapped and released under 700ms  -  go back
BTN_EVT_LONG_PRESS   -  held for 700ms, fires once       -  enter/confirm
```

**All three must be called every loop tick** regardless of what you expect. The internal state machines for each button track timing continuously. Skipping a button breaks its timing.

---

### 7. `include/core/menu.h` + `src/core/menu.cpp`

The carousel renderer and input handler. This is where everything comes together visually.

**Layout (128×64 display):**
```
y= 0  ┌──────────────────────────────┐
      │  Header: parent menu title   │
y=14  ├──────────────────────────────┤
      │  prev item   (small font)    │  baseline y=24
      │> FOCUSED ITEM (bold font)    │  baseline y=38  < always here
      │  next item   (small font)    │  baseline y=52
y=54  ├──────────────────────────────┤
      │  ● ○ ○  depth dots           │  centred at y=60
y=64  └──────────────────────────────┘
```

Depth dots at the bottom show how deep you are in the tree. Hollow dots = parent levels. Filled dot = current level.

Branch items show a `>` suffix after the label. All visual changes to menu appearance happen only in this file.

---

### 8. `include/ui/boot_screen.h` + `src/ui/boot_screen.cpp`

Self-contained boot animation. Three phases across 10 seconds:

```
0–1500ms    Pure glitch scramble (LCG PRNG, no heap)
1500–4500ms Letter-by-letter resolve
4500–5300ms Title hold
5300–7000ms Full title + tagline + chunky block progress bar
```

Completely isolated from the rest of the firmware. No other file needs to know how it works internally.

---

### 9. `include/core/screens.h` + `src/core/screens.cpp`

The tool dispatcher. When the FSM is in `SCREEN_TOOL`, this file routes to the correct implementation based on `getActiveTool()`.

Three switch statements - one each in `onToolEnter()`, `drawToolScreen()`, and `handleToolScreen()`. Unimplemented tools fall through to a stub that shows the tool name and "Not implemented / TAP: Back".

`onToolEnter()` is called exactly once when entering a tool, before the first draw tick. Use it to reset all tool-local state to a known clean starting point.

---

### 10. `src/main.cpp`

Read this last. It is 35 lines. By this point you will understand every single one of them.

```
setup()  -  init hardware - init display - init buttons
          - init FSM - init nav stack - start boot animation

loop()   -  read FSM state
          - SCREEN_BOOT:  draw boot animation, check timeout
          - SCREEN_MENU:  handle input, draw carousel
          - SCREEN_TOOL:  handle input, draw tool screen
```

---

## The Full Data Flow

A complete trace of what happens when a user navigates to BME280 and back:

```
navInit() - _stack[0] = { MENU_MAIN, 11, cursor=0, "JITTERDECK" }

User presses DOWN - navCursorDown() - cursor = 1 ... cursor = 0 (I2C Tools)

User long presses SELECT on "I2C Tools":
  navSelectedItem() - &MENU_MAIN[0]
  toolID == TOOL_NONE - branch
  navPush(I2C_MENU, 2, "I2C Tools")
  _stack[1] = { I2C_MENU, 2, cursor=0, "I2C Tools" }
  _depth = 2

User long presses SELECT on "Sensor Profiles":
  navPush(I2C_SENSOR_PROFILES, 8, "Sensor Profiles")
  _stack[2] = { I2C_SENSOR_PROFILES, 8, cursor=0, "Sensor Profiles" }
  _depth = 3

User long presses SELECT on "BME280":
  toolID == TOOL_I2C_BME280 - leaf
  setActiveTool(TOOL_I2C_BME280)
  onToolEnter(TOOL_I2C_BME280)  ← resets BME280 tool state
  setScreen(SCREEN_TOOL)
  ← nav_stack untouched, still at depth 3

User short presses SELECT (back from tool):
  setActiveTool(TOOL_NONE)
  setScreen(SCREEN_MENU)
  ← nav_stack still at depth 3, cursor still on BME280

User short presses SELECT (back from Sensor Profiles):
  navPop() - _depth = 2
  ← I2C_MENU visible, cursor still on "Sensor Profiles"

User short presses SELECT (back from I2C Tools):
  navPop() - _depth = 1
  ← MENU_MAIN visible, cursor still on "I2C Tools"

User short presses SELECT at root:
  navPop() returns false - nothing happens
```