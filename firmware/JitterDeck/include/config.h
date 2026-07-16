#ifndef CONFIG_H
#define CONFIG_H

// =========================================
// Hardware - I2C
// =========================================
#define I2C_SDA   8
#define I2C_SCL   9

// =========================================
// Hardware - UART
// =========================================
#define UART_TX   20
#define UART_RX   21
 
// =========================================
// Hardware - Digital IO
// GPIO 5 - labelled "GPIO" on PCB silkscreen
// GPIO 4 - labelled "ADC"  on PCB silkscreen
// =========================================
#define DIO_PIN   5   // digital IO tools: input monitor, output toggle, pulse counter, PWM
#define ADC_PIN   4   // analog tools: ADC monitor, voltage meter

// ===========================================
// Hardware - Buttons & Timing Constants
// ===========================================
#define BTN_UP      0
#define BTN_DOWN    1
#define BTN_SELECT  3

#define BTN_DEBOUNCE_MS   30    // ignore transitions shorter than this
#define BTN_LONGPRESS_MS  700   // hold duration for long press

// =========================================
// Boot
// =========================================
#define BOOT_DURATION_MS  7000  // must match BOOT_TOTAL in boot_screen.cpp

// =========================================
// Navigation Stack
// =========================================
#define NAV_STACK_MAX_DEPTH  8

// =========================================
// Display Details
// =========================================
#define DISPLAY_W  128
#define DISPLAY_H  64

#endif