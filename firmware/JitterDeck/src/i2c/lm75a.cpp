#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "lm75a.h"
#include "button.h"
#include "fsm.h"
#include "config.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// =============================================
//  LM75A Register Map
// =============================================

static const uint8_t LM75A_ADDR       = 0x48;  // default: all addr pins GND

static const uint8_t REG_TEMP         = 0x00;  // temperature (16-bit, big-endian)
static const uint8_t REG_CONF         = 0x01;  // configuration
static const uint8_t REG_THYST        = 0x02;  // hysteresis threshold
static const uint8_t REG_TOS          = 0x03;  // overtemperature shutdown

// REG_CONF bits
static const uint8_t CONF_NORMAL_MODE = 0x00;  // bit1=0 -> normal (not shutdown)

// =============================================
//  LM75A has no WHO_AM_I register.
//  Init strategy:
//    1. Write config register -> normal mode
//    2. Read temperature register
//    3. Validate raw value is in plausible range
//       LM75A range: -55°C to +125°C
//       Raw 9-bit range: -110 to 250 (in 0.5°C units)
//       We accept -120 to 260 to allow for noise
// =============================================

static const int16_t PLAUSIBLE_MIN    = -120;  // tenths: -12.0°C lower guard
static const int16_t PLAUSIBLE_MAX    =  260;  // tenths:  26.0°C as 0.5 units

// =============================================
//  Update interval
// =============================================

static const uint16_t READ_INTERVAL_MS = 500;  // LM75A updates every ~100ms
                                                // 500ms is comfortable for display

// =============================================
//  Tool state
// =============================================

enum LM75AState
{
    LM75A_INIT,      // first entry - configure and validate
    LM75A_ERROR,     // I²C failure or implausible reading
    LM75A_RUNNING    // reading and displaying live temperature
};

static LM75AState    _state       = LM75A_INIT;
static int16_t       _rawTemp     = 0;    // raw 9-bit signed value from sensor
static int16_t       _tempHalves  = 0;    // temperature in 0.5°C units (signed)
static uint8_t       _errorCode   = 0;    // I²C error code for display
static unsigned long _lastReadMs  = 0;

// =============================================
//  Low-level I²C helpers
// =============================================

static uint8_t regWrite(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(LM75A_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission();
}

// LM75A temperature register is 16-bit big-endian (MSB first)
// Returns true on success
static bool readTempReg(int16_t* out)
{
    Wire.beginTransmission(LM75A_ADDR);
    Wire.write(REG_TEMP);
    if (Wire.endTransmission(false) != 0)
        return false;

    uint8_t received = Wire.requestFrom((uint8_t)LM75A_ADDR, (uint8_t)2);
    if (received != 2)
        return false;

    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    // 16-bit big-endian -> signed 16-bit
    // Upper 9 bits are valid, lower 7 bits unused
    // Shift right 7 to get 9-bit signed value in 0.5°C units
    int16_t raw = (int16_t)((msb << 8) | lsb);
    *out = raw >> 7;

    return true;
}

// =============================================
//  Init sequence
//  LM75A has no WHO_AM_I so we:
//    1. Put device in normal mode
//    2. Read temperature
//    3. Validate plausibility
// =============================================

static bool initSensor()
{
    // 1. Write normal mode to config register
    uint8_t err = regWrite(REG_CONF, CONF_NORMAL_MODE);
    if (err != 0)
    {
        _errorCode = err;
        return false;
    }

    // 2. Small settle wait - LM75A needs one conversion cycle
    //    We don't use delay() - just attempt read and validate
    int16_t raw = 0;
    if (!readTempReg(&raw))
    {
        _errorCode = 0xFF;  // I²C read failure
        return false;
    }

    // 3. Plausibility check - raw is in 0.5°C units
    //    Sensor range is -110 to +250 (= -55°C to +125°C)
    //    We use a slightly wider window to guard against power-on noise
    if (raw < -120 || raw > 260)
    {
        _errorCode = 0xFE;  // implausible value
        return false;
    }

    _tempHalves = raw;
    return true;
}

// =============================================
//  Format temperature for display
//
//  _tempHalves is signed, in 0.5°C units
//  Examples:
//    _tempHalves =  50 ->  25.0°C
//    _tempHalves =  51 ->  25.5°C
//    _tempHalves = -3  ->  -1.5°C
//
//  Output: "-125.5" or " 25.0" (space-padded for alignment)
// =============================================

static void fmtTemp(char* buf, uint8_t bufLen)
{
    int16_t  halves   = _tempHalves;
    bool     negative = (halves < 0);
    if (negative) halves = -halves;

    int16_t whole = halves / 2;
    uint8_t frac  = (halves % 2) ? 5 : 0;  // 0 or 5 (tenths digit)

    snprintf(buf, bufLen, "%s%d.%d",
        negative ? "-" : " ",
        whole,
        frac);
}

// =============================================
//  Draw helpers
// =============================================

static void drawHeader()
{
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, "LM75A  Temp");
    u8g2.drawHLine(0, 13, DISPLAY_W);
}

static void drawHintBar()
{
    u8g2.drawHLine(0, 54, DISPLAY_W);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 63, "TAP: Back");
}

// =============================================
//  Draw: LM75A_ERROR
// =============================================

static void drawError()
{
    drawHeader();
    u8g2.setFont(u8g2_font_5x7_tr);

    if (_errorCode == 0xFF)
    {
        u8g2.drawStr(4, 26, "I2C read failed");
        u8g2.drawStr(4, 35, "Check wiring");
        u8g2.drawStr(4, 44, "Addr: 0x48");
    }
    else if (_errorCode == 0xFE)
    {
        u8g2.drawStr(4, 26, "Implausible reading");
        u8g2.drawStr(4, 35, "Wrong device at");
        u8g2.drawStr(4, 44, "0x48?");
    }
    else
    {
        char buf[24];
        snprintf(buf, sizeof(buf), "I2C err code: %d", _errorCode);
        u8g2.drawStr(4, 26, buf);
        u8g2.drawStr(4, 35, "Config write failed");
    }

    drawHintBar();
}

// =============================================
//  Draw: LM75A_RUNNING
//
//  Layout:
//    y=10  "LM75A  Temp"         header
//    y=13  separator
//    y=22  "0x48  0.5C res"      subtitle
//    y=38  "  25.5 C"            large temperature - centred
//    y=48  "Raw: 51"             raw register value for validation
//    y=54  separator
//    y=63  "TAP: Back"
// =============================================

static void drawRunning()
{
    drawHeader();

    // Subtitle
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 22, "0x48    Res: 0.5\xb0C");

    // Large temperature value - centred on screen
    char tempBuf[10];
    fmtTemp(tempBuf, sizeof(tempBuf));

    // Append degree symbol and C
    char display[16];
    snprintf(display, sizeof(display), "%s\xb0C", tempBuf);

    u8g2.setFont(u8g2_font_7x13B_tr);
    uint8_t tw = u8g2.getStrWidth(display);
    u8g2.drawStr((DISPLAY_W - tw) / 2, 40, display);

    // Raw register value - useful for validation
    char rawBuf[20];
    snprintf(rawBuf, sizeof(rawBuf), "Raw: %d  (%d halves)",
        _rawTemp, _tempHalves);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 50, rawBuf);

    drawHintBar();
}

// =============================================
//  Public: resetLM75A()
// =============================================

void resetLM75A()
{
    _state      = LM75A_INIT;
    _rawTemp    = 0;
    _tempHalves = 0;
    _errorCode  = 0;
    _lastReadMs = 0;
}

// =============================================
//  Public: drawLM75A()
// =============================================

void drawLM75A()
{
    u8g2.clearBuffer();

    switch (_state)
    {
        case LM75A_INIT:
            drawHeader();
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(4, 30, "Initialising...");
            break;

        case LM75A_ERROR:
            drawError();
            break;

        case LM75A_RUNNING:
            drawRunning();
            break;
    }

    u8g2.sendBuffer();
}

// =============================================
//  Public: handleLM75A()
// =============================================

void handleLM75A()
{
    // Must poll all buttons every tick
    const ButtonEvent evUp     = btnUp();
    const ButtonEvent evDown   = btnDown();
    const ButtonEvent evSelect = btnSelect();

    (void)evUp;    // unused in this tool
    (void)evDown;  // unused in this tool

    // TAP back - works in all states
    if (evSelect == BTN_EVT_SHORT_PRESS)
    {
        setActiveTool(TOOL_NONE);
        setScreen(SCREEN_MENU);
        return;
    }

    switch (_state)
    {
        // == Run init once on entry ====================================
        case LM75A_INIT:
            if (initSensor())
            {
                _lastReadMs = millis();
                _state      = LM75A_RUNNING;
            }
            else
            {
                _state = LM75A_ERROR;
            }
            break;

        // == Error: wait for user to tap back =========================
        case LM75A_ERROR:
            break;

        // == Running: poll temperature every READ_INTERVAL_MS =========
        case LM75A_RUNNING:
            if (millis() - _lastReadMs >= READ_INTERVAL_MS)
            {
                _lastReadMs = millis();

                int16_t raw = 0;
                if (readTempReg(&raw))
                {
                    _rawTemp    = (int16_t)((int16_t)(raw << 7) >> 7); // keep 16-bit form
                    _tempHalves = raw;
                }
                // If read fails mid-session, keep last known value
                // silently - transient I²C noise should not crash display
            }
            break;
    }
}