#include <Arduino.h>
#include <U8g2lib.h>

#include "dio_output_toggle.h"
#include "button.h"
#include "fsm.h"
#include "config.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// =============================================
//  State
// =============================================

static bool _outputState  = false;   // false = LOW, true = HIGH
static uint32_t _toggleCount = 0;    // how many times toggled this session

// =============================================
//  Public: resetDIOOutputToggle()
// =============================================

void resetDIOOutputToggle()
{
    _outputState  = false;
    _toggleCount  = 0;

    // Set pin as output, drive LOW initially
    pinMode(DIO_PIN, OUTPUT);
    digitalWrite(DIO_PIN, LOW);
}

// =============================================
//  drawDIOOutputToggle()
//
//  Layout:
//    y=10  "Output Toggle"         header
//    y=13  separator
//    y=22  "GPIO 5    OUTPUT"
//    y=38  " HIGH " or " LOW "    large state (inverted = HIGH)
//    y=48  "Toggles: NNN"
//    y=54  separator
//    y=63  "HOLD:Toggle   TAP:Back"
// =============================================

void drawDIOOutputToggle()
{
    u8g2.clearBuffer();

    // Header
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, "Output Toggle");
    u8g2.drawHLine(0, 13, DISPLAY_W);

    // Pin info
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 22, "GPIO 5    OUTPUT");

    // Large state — filled box for HIGH, outline for LOW
    const char* stateStr = _outputState ? "  HIGH  " : "  LOW   ";
    u8g2.setFont(u8g2_font_7x13B_tr);
    uint8_t tw = u8g2.getStrWidth(stateStr);
    uint8_t bx = (DISPLAY_W - tw) / 2 - 2;

    if (_outputState)
    {
        u8g2.drawBox(bx, 25, tw + 4, 15);
        u8g2.setDrawColor(0);
        u8g2.drawStr((DISPLAY_W - tw) / 2, 37, stateStr);
        u8g2.setDrawColor(1);
    }
    else
    {
        u8g2.drawFrame(bx, 25, tw + 4, 15);
        u8g2.drawStr((DISPLAY_W - tw) / 2, 37, stateStr);
    }

    // Toggle count
    u8g2.setFont(u8g2_font_5x7_tr);
    char cntBuf[20];
    snprintf(cntBuf, sizeof(cntBuf), "Toggles: %lu", (unsigned long)_toggleCount);
    u8g2.drawStr(4, 48, cntBuf);

    // Hint bar
    u8g2.drawHLine(0, 54, DISPLAY_W);
    u8g2.drawStr(2, 63, "HOLD: Toggle");
    uint8_t rw = u8g2.getStrWidth("TAP: Back");
    u8g2.drawStr(DISPLAY_W - rw - 2, 63, "TAP: Back");

    u8g2.sendBuffer();
}

// =============================================
//  handleDIOOutputToggle()
// =============================================

void handleDIOOutputToggle()
{
    btnUp();
    btnDown();
    const ButtonEvent evSelect = btnSelect();

    if (evSelect == BTN_EVT_SHORT_PRESS)
    {
        // Drive LOW before leaving — safe state
        digitalWrite(DIO_PIN, LOW);
        setActiveTool(TOOL_NONE);
        setScreen(SCREEN_MENU);
        return;
    }

    if (evSelect == BTN_EVT_LONG_PRESS)
    {
        _outputState = !_outputState;
        digitalWrite(DIO_PIN, _outputState ? HIGH : LOW);
        _toggleCount++;
    }
}