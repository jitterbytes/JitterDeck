# Design Philosophy

Why JitterDeck is built the way it is.

These are not arbitrary choices. Each decision was made for a specific reason that directly affects the reliability and usefulness of a hardware validation tool.

---

## Register-Level Drivers, Not Library Wrappers

JitterDeck does not use Adafruit, SparkFun, or similar high-level sensor libraries. Every sensor driver reads and writes registers directly from the datasheet.

**Why:** JitterDeck is a validation tool. When you are bringing up a new piece of hardware, you need to see raw register values, confirm the chip ID yourself, and understand exactly what I²C transaction happened. High-level abstractions work against this. `bme.readTemperature()` tells you a number - it does not tell you whether the WHO_AM_I register matched, what the raw ADC counts were, or whether the sensor was in the right measurement mode.

A validation tool that hides the hardware is not a validation tool.

**In practice:** every sensor driver in JitterDeck starts by reading the WHO_AM_I register and comparing it to the expected value from the datasheet. If it does not match, it shows the user exactly what was received. This is the most useful thing this tool can do - it immediately distinguishes "sensor not present", "wrong device on this address", and "sensor present and correctly identified" without ambiguity.

---

## No Dynamic Memory Allocation

No `malloc`, no `new`, no Arduino `String` objects. No heap involvement anywhere.

**Why:** embedded devices that run continuously accumulate heap fragmentation over time. A tool you leave running on a bench overnight should behave identically to one that just powered on. Heap fragmentation produces rare, timing-dependent bugs that are extremely difficult to reproduce or diagnose - exactly the kind of bugs you do not want in a diagnostic tool.

**In practice:**
- The navigation stack is a fixed array of 8 frames allocated at compile time
- All menu data is `const` in flash - zero RAM cost for the entire tree
- All sensor buffers are `char buf[N]` on the stack, freed when the function returns
- `snprintf` instead of `String` concatenation everywhere

The entire navigation system for 60+ menu items across 3 levels of depth costs **80 bytes of RAM**.

---

## Non-Blocking Everything

No `delay()` anywhere in the firmware. All timing uses elapsed `millis()`.

**Why:** `delay()` freezes the CPU. During that freeze the display does not update, buttons do not register, and any background operation stalls. On a simple menu with three items this is barely noticeable. On a live sensor screen that polls data at 10Hz while also handling buttons and refreshing the display, a blocking `delay()` anywhere in the chain breaks the entire interaction model.

**In practice:** every operation that needs timing - button debounce, sensor poll intervals, boot animation phases, dot animation - uses the pattern:
```cpp
if (millis() - _lastActionMs >= INTERVAL_MS)
{
    _lastActionMs = millis();
    doAction();
}
```
The loop runs as fast as possible and each subsystem checks its own elapsed time independently. Nothing waits for anything else.

---

## One Concern Per File

`menu_defs.cpp` is pure data. `nav_stack.cpp` is pure position tracking. `menu.cpp` is pure rendering. `button.cpp` is pure input. Sensor files are pure sensor logic. Nothing bleeds across these boundaries.

**Why:** when you need to change how the carousel looks, you open `menu.cpp` and only `menu.cpp`. When you add a sensor, you create one new file and touch three specific lines in three specific existing files. When something breaks in navigation, you look in `nav_stack.cpp`. The scope of every change and every bug is predictable.

**In practice:** this is enforced by the dependency structure. `menu_defs.cpp` includes nothing from the rest of the firmware - it cannot accidentally call a display function. `nav_stack.cpp` knows nothing about buttons. The compiler enforces these boundaries at build time.

---

## The FSM Never Drives Itself

`fsm.cpp` only stores state. It never decides to change state on its own.

**Why:** if the FSM contained transition logic, you would have to look in two places to understand any state change - the file that triggered it and the FSM itself. Keeping the FSM as pure storage means the transition logic is always in the file that owns the decision:

- Boot timeout - `main.cpp` decides and calls `setScreen()`
- Entering a tool - `menu.cpp` decides after a long press
- Exiting a tool - `screens.cpp` decides after a short press

Trace any state change and you find it in exactly one place.

---

## Data Separate From Logic

The entire menu tree - all 60+ items across 11 top-level categories - is `const` data in flash. The navigation engine holds only pointers and indices into this data. The renderer reads from the navigation engine. None of these three layers knows about the others' internals.

**Why:** adding a new sensor is a data change, not a logic change. You add entries to arrays in `menu_defs.cpp` and `menu_defs.h`. The navigation engine does not need to know a new sensor exists. The renderer does not need to know. Only the dispatcher (`screens.cpp`) needs a new case, because dispatching is explicitly about routing by ID.

This separation is what makes the five-step sensor addition process work without touching navigation or rendering code.

---

## Embedded-Safe Primitives

The LCG PRNG in `boot_screen.cpp` instead of `rand()`. Integer arithmetic instead of float. Fixed-size arrays instead of `std::vector`. Stack buffers instead of heap strings.

**Why:** JitterDeck is designed to eventually run on targets smaller than the ESP32-C3. The ESP32-C3 has 400KB RAM and a hardware FPU - it can handle most of these comfortably. But the habits that make code portable to a 32KB ATmega or a 256KB STM32 are the same habits that make code predictable and deterministic on any target. The cost of writing `snprintf` instead of `String +=` is zero. The benefit is a codebase that ports cleanly.