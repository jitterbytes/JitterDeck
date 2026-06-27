#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "i2c_scanner.h"
#include "button.h"
#include "fsm.h"
#include "config.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// ─────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────

static const uint8_t I2C_ADDR_START  = 0x01;
static const uint8_t I2C_ADDR_END    = 0x7F;
static const uint8_t MAX_DEVICES     = 16;   // max we'll store

// Dot animation timing
static const uint16_t DOT_INTERVAL_MS = 400;

// Progress bar geometry
static const uint8_t BAR_X  = 4;
static const uint8_t BAR_Y  = 40;
static const uint8_t BAR_W  = 120;
static const uint8_t BAR_H  = 6;

// List layout (post-scan)
//
// Zone map (64px display):
//   y=14–23  count string + separator    (baseline y=22, line y=23)
//   y=24–52  3 list rows × 9px each      (baselines y=32, y=41, y=50)
//   y=53–63  hint bar separator + text
//
static const uint8_t LIST_START_Y  = 32;  // baseline of first list row
static const uint8_t LIST_ITEM_H   = 9;   // pixels between row baselines
static const uint8_t LIST_VISIBLE  = 3;   // rows visible at once

// ─────────────────────────────────────────────
//  Known I2C address lookup table
//  Stored in flash (const) — zero RAM cost
// ─────────────────────────────────────────────

struct KnownDevice
{
    uint8_t     addr;
    const char* name;
};

static const KnownDevice KNOWN_DEVICES[] =
{
    { 0x20, "PCF8574"       },
    { 0x23, "BH1750"        },
    { 0x27, "LCD PCF8574"   },
    { 0x38, "AHT20"         },
    { 0x3C, "SSD1306 OLED"  },
    { 0x3D, "SSD1306 OLED"  },
    { 0x40, "INA219"        },
    { 0x44, "SHT31"         },
    { 0x45, "SHT31"         },
    { 0x48, "ADS1115"       },
    { 0x4A, "ADS1115"       },
    { 0x57, "AT24C EEPROM"  },
    { 0x5C, "AM2320"        },
    { 0x60, "MCP4725 DAC"   },
    { 0x68, "MPU6050/DS3231"},
    { 0x69, "MPU6050"       },
    { 0x76, "BME/BMP280"    },
    { 0x77, "BME/BMP280"    },
};

static const uint8_t KNOWN_COUNT =
    sizeof(KNOWN_DEVICES) / sizeof(KNOWN_DEVICES[0]);

// ─────────────────────────────────────────────
//  Scanner state
// ─────────────────────────────────────────────

enum ScanState
{
    SCAN_IDLE,      // waiting for user to initiate
    SCAN_RUNNING,   // actively scanning addresses
    SCAN_DONE       // results ready, showing list
};

static ScanState     _state           = SCAN_IDLE;
static uint8_t       _foundAddrs[MAX_DEVICES];
static uint8_t       _foundCount      = 0;
static uint8_t       _currentAddr     = I2C_ADDR_START;  // scan progress
static uint8_t       _listCursor      = 0;               // selected result
static uint8_t       _listScrollTop   = 0;               // first visible result
static unsigned long _dotTimer        = 0;
static uint8_t       _dotCount        = 0;               // 0,1,2 → " ." ".." "..."

// ─────────────────────────────────────────────
//  Public: resetI2CScanner()
//  Call this every time the tool is entered
// ─────────────────────────────────────────────

void resetI2CScanner()
{
    _state         = SCAN_IDLE;
    _foundCount    = 0;
    _currentAddr   = I2C_ADDR_START;
    _listCursor    = 0;
    _listScrollTop = 0;
    _dotTimer      = 0;
    _dotCount      = 0;
}

// ─────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────

// Look up name for a given address — returns nullptr if unknown
static const char* lookupDevice(uint8_t addr)
{
    for (uint8_t i = 0; i < KNOWN_COUNT; i++)
    {
        if (KNOWN_DEVICES[i].addr == addr)
            return KNOWN_DEVICES[i].name;
    }
    return nullptr;
}

// Format address as "0xNN" into a caller-supplied buffer
static void fmtAddr(uint8_t addr, char* buf)
{
    const char hex[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = hex[(addr >> 4) & 0x0F];
    buf[3] = hex[addr & 0x0F];
    buf[4] = '\0';
}

// Advance dot animation — updates _dotCount on interval
static void tickDots()
{
    if (millis() - _dotTimer >= DOT_INTERVAL_MS)
    {
        _dotTimer = millis();
        _dotCount = (_dotCount + 1) % 3;
    }
}

// Draw the shared header bar (used in all states)
static void drawHeader()
{
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, "I2C Scanner");
    u8g2.drawHLine(0, 13, DISPLAY_W);
}

// Draw the hint bar at the bottom
static void drawHintBar(const char* left, const char* right)
{
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 63, left);

    // Right-align the right hint
    uint8_t rw = u8g2.getStrWidth(right);
    u8g2.drawStr(DISPLAY_W - rw - 2, 63, right);

    u8g2.drawHLine(0, 54, DISPLAY_W);
}

// ─────────────────────────────────────────────
//  Draw: SCAN_IDLE
// ─────────────────────────────────────────────

static void drawIdle()
{
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 30, "Ready to scan");

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 44, "0x01 - 0x7F");

    drawHintBar("HOLD: Scan", "TAP: Back");
}

// ─────────────────────────────────────────────
//  Draw: SCAN_RUNNING
// ─────────────────────────────────────────────

static void drawRunning()
{
    // "Scanning" + animated dots
    char label[14] = "Scanning";
    for (uint8_t d = 0; d <= _dotCount; d++)
        label[8 + d] = '.';
    label[9 + _dotCount] = '\0';

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 26, label);

    // Current address being probed
    char addrBuf[5];
    fmtAddr(_currentAddr, addrBuf);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 37, addrBuf);

    // Progress bar
    const uint8_t total    = I2C_ADDR_END - I2C_ADDR_START;
    const uint8_t scanned  = _currentAddr - I2C_ADDR_START;
    const uint8_t fillW    = (uint8_t)((uint16_t)scanned * BAR_W / total);

    u8g2.drawFrame(BAR_X, BAR_Y, BAR_W, BAR_H);
    if (fillW > 0)
        u8g2.drawBox(BAR_X, BAR_Y, fillW, BAR_H);

    // Found count so far
    char countBuf[16];
    snprintf(countBuf, sizeof(countBuf), "Found: %d", _foundCount);
    u8g2.drawStr(4, 52, countBuf);
}

// ─────────────────────────────────────────────
//  Draw: SCAN_DONE
// ─────────────────────────────────────────────

static void drawDone()
{
    // Result count header line
    char countBuf[20];
    if (_foundCount == 0)
        snprintf(countBuf, sizeof(countBuf), "No devices found");
    else if (_foundCount == 1)
        snprintf(countBuf, sizeof(countBuf), "Found 1 device");
    else
        snprintf(countBuf, sizeof(countBuf), "Found %d devices", _foundCount);

    // Count string — baseline y=22, sits cleanly in y=14..23 zone
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 22, countBuf);
    u8g2.drawHLine(0, 23, DISPLAY_W);

    if (_foundCount == 0)
    {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(4, 42, "Check wiring");
        drawHintBar("HOLD: Rescan", "TAP: Back");
        return;
    }

    // Scrollable device list — 3 visible rows
    // Row baselines: 32, 41, 50  (LIST_START_Y + row * LIST_ITEM_H)
    for (uint8_t row = 0; row < LIST_VISIBLE; row++)
    {
        uint8_t idx = _listScrollTop + row;
        if (idx >= _foundCount) break;

        const uint8_t addr    = _foundAddrs[idx];
        const bool    focused = (idx == _listCursor);
        const uint8_t y       = LIST_START_Y + (row * LIST_ITEM_H);

        // Highlight box: sits 7px above baseline, 8px tall — fits the 5x7 font
        if (focused)
        {
            u8g2.drawBox(0, y - 7, DISPLAY_W, 9);
            u8g2.setDrawColor(0);
        }

        // Address + device name — uniform 5x7 font for all rows
        // focused row gets the highlight box instead of a bigger font
        u8g2.setFont(u8g2_font_5x7_tr);

        char addrBuf[5];
        fmtAddr(addr, addrBuf);
        u8g2.drawStr(4, y, addrBuf);

        const char* name = lookupDevice(addr);
        if (name)
            u8g2.drawStr(30, y, name);

        if (focused)
            u8g2.setDrawColor(1);
    }

    drawHintBar("HOLD: Rescan", "TAP: Back");
}

// ─────────────────────────────────────────────
//  drawI2CScanner()
//  Call every loop() tick while tool is active
// ─────────────────────────────────────────────

void drawI2CScanner()
{
    u8g2.clearBuffer();
    drawHeader();

    switch (_state)
    {
        case SCAN_IDLE:    drawIdle();    break;
        case SCAN_RUNNING: drawRunning(); break;
        case SCAN_DONE:    drawDone();    break;
    }

    u8g2.sendBuffer();
}

// ─────────────────────────────────────────────
//  Scan one address per tick — non-blocking
//
//  Wire.endTransmission() return codes:
//    0 = success (device ACKed)
//    1 = data too long
//    2 = NACK on address → no device
//    3 = NACK on data
//    4 = other error
// ─────────────────────────────────────────────

static void scanTick()
{
    if (_currentAddr > I2C_ADDR_END)
    {
        _state = SCAN_DONE;
        return;
    }

    Wire.beginTransmission(_currentAddr);
    uint8_t err = Wire.endTransmission();

    if (err == 0 && _foundCount < MAX_DEVICES)
        _foundAddrs[_foundCount++] = _currentAddr;

    _currentAddr++;
    tickDots();
}

// ─────────────────────────────────────────────
//  handleI2CScanner()
//  Call every loop() tick while tool is active
// ─────────────────────────────────────────────

void handleI2CScanner()
{
    const ButtonEvent evUp     = btnUp();
    const ButtonEvent evDown   = btnDown();
    const ButtonEvent evSelect = btnSelect();

    // TAP SELECT → always goes back regardless of state
    if (evSelect == BTN_EVT_SHORT_PRESS)
    {
        setActiveTool(TOOL_NONE);
        setScreen(SCREEN_MENU);
        return;
    }

    switch (_state)
    {
        // ── IDLE: hold SELECT to begin scan ─────────────────────────
        case SCAN_IDLE:
            if (evSelect == BTN_EVT_LONG_PRESS)
            {
                _foundCount  = 0;
                _currentAddr = I2C_ADDR_START;
                _dotCount    = 0;
                _dotTimer    = millis();
                _state       = SCAN_RUNNING;
            }
            break;

        // ── RUNNING: scan one address this tick ──────────────────────
        case SCAN_RUNNING:
            scanTick();
            // No button input accepted mid-scan except TAP back (handled above)
            break;

        // ── DONE: scroll list, hold to rescan ────────────────────────
        case SCAN_DONE:
            if (evUp == BTN_EVT_SHORT_PRESS || evUp == BTN_EVT_LONG_PRESS)
            {
                if (_listCursor > 0)
                {
                    _listCursor--;
                    // Scroll up if cursor moves above visible window
                    if (_listCursor < _listScrollTop)
                        _listScrollTop = _listCursor;
                }
            }

            if (evDown == BTN_EVT_SHORT_PRESS || evDown == BTN_EVT_LONG_PRESS)
            {
                if (_listCursor < _foundCount - 1)
                {
                    _listCursor++;
                    // Scroll down if cursor moves below visible window
                    if (_listCursor >= _listScrollTop + LIST_VISIBLE)
                        _listScrollTop = _listCursor - LIST_VISIBLE + 1;
                }
            }

            if (evSelect == BTN_EVT_LONG_PRESS)
            {
                // Rescan
                _foundCount    = 0;
                _currentAddr   = I2C_ADDR_START;
                _listCursor    = 0;
                _listScrollTop = 0;
                _dotCount      = 0;
                _dotTimer      = millis();
                _state         = SCAN_RUNNING;
            }
            break;
    }
}