# JitterDeck

A portable embedded hardware validation tool for engineers who want to test sensors and communication interfaces fast — without writing dedicated firmware every time.

> Connect a sensor. Select its profile. Instantly see live data, run diagnostics, verify your wiring.

![JitterDeck](img/JitterDeck_PCB.png)

---

## What is JitterDeck?

JitterDeck is my first open source handheld tool built on the ESP32-C3 that lets you validate sensors and modules in the field. It exposes I²C, SPI, UART, 1-Wire, analog, and digital IO through a menu-driven interface on a 1.3" OLED display, navigated with three buttons.

It is a practical companion for embedded development, debugging, prototyping, and hardware bring-up. The kind of tool you keep in your kit alongside your multimeter.

---

## Hardware at a Glance

| Component       | Details                                  |
|-----------------|------------------------------------------|
| Microcontroller | ESP32-C3 SuperMini                       |
| Display         | 1.3" SH1106 OLED 128×64 (I²C)           |
| Buttons         | 3× tactile push buttons                  |

---

## Documentation

| Document | Description |
|----------|-------------|
| [Getting Started](docs/getting_started.md) | Hardware wiring, building, and flashing |
| [Codebase Guide](docs/codebase_guide.md) | How the firmware is structured and how to read it |
| [Adding Sensors](docs/adding_sensors.md) | How to add a new sensor or tool to any protocol |
| [Design Philosophy](docs/design_philosophy.md) | Why the firmware is built the way it is |
| [Contributing](docs/contributing.md) | Rules and checklist before opening a pull request |

---

## License

MIT License — see `LICENSE` file.
Free to use, modify, and distribute. Attribution appreciated.