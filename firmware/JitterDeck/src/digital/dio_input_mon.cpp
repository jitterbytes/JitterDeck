#include <Arduino.h>
#include <U8g2lib.h>

#include "dio_input_mon.h"
#include "button.h"
#include "fsm.h"
#include "config.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// ─────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────

static const uint16_t READ_INTERVAL_MS = 50;    // 20Hz refresh
static const uint16_t HISTORY_LEN      = 64;    // one pixel per sample = full width

// ─────────────────────────────────────────────
//  State
// ─────────────────────────────────────────────

static bool          _currentState  = false;
static bool          _history[HISTORY_LEN];    // ring buffer of past readings
static uint8_t       _histHead      = 0;       // write index into ring buffer
static uint32_t      _highCount     = 0;       // total HIGH readings since reset
static uint32_t      _totalCount    = 0;       // total readings since reset
static unsigned long _lastReadMs    = 0;

// ─────────────────────────────────────────────
//  Public: resetDIOInputMon()
// ─────────────────────────────────────────────

void resetDIOInputMon()
{
    pinMode(DIO_PIN, INPUT_PULLUP);

    _currentState = false;
    _histHead     = 0;
    _highCount    = 0;
    _totalCount   = 0;
    _lastReadMs   = 0;

    for (uint8_t i = 0; i < HISTORY_LEN; i++)
        _history[i] = false;
}

// ─────────────────────────────────────────────
//  drawDIOInputMon()
//
//  Layout:
//    y=10  "Input Monitor"         header
//    y=13  separator
//    y=22  "GPIO 5   [INPUT_PULLUP]"
//    y=34  " HIGH " or " LOW "    large state indicator (inverted box)
//    y=46  Duty: XX%  Count: NNNN  stats
//    y=54  separator + waveform history strip
//    y=63  "TAP: Back"
// ─────────────────────────────────────────────

void drawDIOInputMon()
{
    u8g2.clearBuffer();

    // Header
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, "Input Monitor");
    u8g2.drawHLine(0, 13, DISPLAY_W);

    // Pin info
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 22, "GPIO 5    INPUT_PULLUP");

    // Large state indicator — inverted box for HIGH, outline for LOW
    const char* stateStr = _currentState ? "  HIGH  " : "  LOW   ";
    u8g2.setFont(u8g2_font_7x13B_tr);

    if (_currentState)
    {
        // Filled box, inverted text
        uint8_t tw = u8g2.getStrWidth(stateStr);
        uint8_t bx = (DISPLAY_W - tw) / 2 - 2;
        u8g2.drawBox(bx, 25, tw + 4, 15);
        u8g2.setDrawColor(0);
        u8g2.drawStr((DISPLAY_W - tw) / 2, 37, stateStr);
        u8g2.setDrawColor(1);
    }
    else
    {
        // Outline box, normal text
        uint8_t tw = u8g2.getStrWidth(stateStr);
        uint8_t bx = (DISPLAY_W - tw) / 2 - 2;
        u8g2.drawFrame(bx, 25, tw + 4, 15);
        u8g2.drawStr((DISPLAY_W - tw) / 2, 37, stateStr);
    }

    // Stats: duty cycle
    u8g2.setFont(u8g2_font_5x7_tr);
    if (_totalCount > 0)
    {
        uint8_t duty = (uint8_t)((_highCount * 100UL) / _totalCount);
        char statBuf[24];
        snprintf(statBuf, sizeof(statBuf), "Duty: %d%%   Samples: %lu",
            duty, (unsigned long)_totalCount);
        u8g2.drawStr(4, 47, statBuf);
    }

    // Waveform history strip — 1px per sample, drawn at bottom
    // HIGH = pixel lit at y=51, LOW = pixel lit at y=53 (two-row strip)
    u8g2.drawHLine(0, 49, DISPLAY_W);
    for (uint8_t x = 0; x < HISTORY_LEN; x++)
    {
        // Read history in order from oldest to newest
        uint8_t idx = (_histHead + x) % HISTORY_LEN;
        if (_history[idx])
            u8g2.drawPixel(x + (DISPLAY_W - HISTORY_LEN) / 2, 51);
        else
            u8g2.drawPixel(x + (DISPLAY_W - HISTORY_LEN) / 2, 53);
    }
    u8g2.drawHLine(0, 54, DISPLAY_W);

    // Hint bar
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 63, "TAP: Back");

    u8g2.sendBuffer();
}

// ─────────────────────────────────────────────
//  handleDIOInputMon()
// ─────────────────────────────────────────────

void handleDIOInputMon()
{
    btnUp();
    btnDown();
    const ButtonEvent evSelect = btnSelect();

    if (evSelect == BTN_EVT_SHORT_PRESS)
    {
        setActiveTool(TOOL_NONE);
        setScreen(SCREEN_MENU);
        return;
    }

    // Poll GPIO at READ_INTERVAL_MS
    if (millis() - _lastReadMs >= READ_INTERVAL_MS)
    {
        _lastReadMs   = millis();
        _currentState = (digitalRead(DIO_PIN) == HIGH);

        // Store in ring buffer
        _history[_histHead] = _currentState;
        _histHead = (_histHead + 1) % HISTORY_LEN;

        // Update stats
        _totalCount++;
        if (_currentState) _highCount++;
    }
}