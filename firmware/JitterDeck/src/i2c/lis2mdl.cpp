#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "lis2mdl.h"
#include "button.h"
#include "fsm.h"
#include "config.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// =============================================
//  LIS2MDL Register Map (from datasheet)
// =============================================

static const uint8_t LIS2MDL_ADDR        = 0x1E;

// Identification
static const uint8_t REG_WHO_AM_I        = 0x4F;
static const uint8_t WHO_AM_I_VALUE      = 0x40;  // expected chip ID

// Configuration
static const uint8_t REG_CFG_REG_A      = 0x60;  // ODR, mode
static const uint8_t REG_CFG_REG_B      = 0x61;  // low-pass filter
static const uint8_t REG_CFG_REG_C      = 0x62;  // BDU, data ready

// Status
static const uint8_t REG_STATUS          = 0x67;  // data ready flag

// Output — 16-bit little-endian, two's complement
static const uint8_t REG_OUTX_L         = 0x68;
static const uint8_t REG_OUTX_H         = 0x69;
static const uint8_t REG_OUTY_L         = 0x6A;
static const uint8_t REG_OUTY_H         = 0x6B;
static const uint8_t REG_OUTZ_L         = 0x6C;
static const uint8_t REG_OUTZ_H         = 0x6D;

// CFG_REG_A values
//   ODR: 00=10Hz 01=20Hz 10=50Hz 11=100Hz
//   MD:  00=continuous  01=single  10/11=idle
static const uint8_t CFG_A_ODR_100HZ_CONT = 0b00001100; // ODR=100Hz, MD=continuous

// CFG_REG_C values — enable Block Data Update
//   BDU prevents reading MSB/LSB from different samples
static const uint8_t CFG_C_BDU          = 0b00010000;

// Sensitivity: 1.5 mGauss per LSB (fixed, no range selection on LIS2MDL)
// Stored as integer + fractional parts to avoid float division loops
// raw * 1.5 = raw + raw/2
// We'll scale to tenths of mGauss: raw * 15 / 10

// =============================================
//  Update interval
// =============================================

static const uint16_t READ_INTERVAL_MS = 100;  // 10Hz display refresh

// =============================================
//  Tool state
// =============================================

enum LIS2MDLState
{
    MAG_INIT,       // first entry — check WHO_AM_I, configure
    MAG_ERROR,      // WHO_AM_I mismatch or I2C failure
    MAG_RUNNING     // reading and displaying live data
};

static LIS2MDLState  _state        = MAG_INIT;
static int16_t       _rawX         = 0;
static int16_t       _rawY         = 0;
static int16_t       _rawZ         = 0;
static unsigned long _lastReadMs   = 0;
static uint8_t       _errorCode    = 0;   // stores I2C error for display

// =============================================
//  Low-level I2C helpers
// =============================================

// Write one byte to a register
// Returns Wire error code: 0 = success
static uint8_t regWrite(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(LIS2MDL_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission();
}

// Read one byte from a register
// Returns true on success, stores result in *out
static bool regRead(uint8_t reg, uint8_t* out)
{
    Wire.beginTransmission(LIS2MDL_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)   // repeated start
        return false;

    uint8_t received = Wire.requestFrom((uint8_t)LIS2MDL_ADDR, (uint8_t)1);
    if (received != 1)
        return false;

    *out = Wire.read();
    return true;
}

// Burst-read `len` bytes starting at `reg` into `buf`
// LIS2MDL auto-increments the register address
static bool regReadBurst(uint8_t reg, uint8_t* buf, uint8_t len)
{
    Wire.beginTransmission(LIS2MDL_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
        return false;

    uint8_t received = Wire.requestFrom((uint8_t)LIS2MDL_ADDR, len);
    if (received != len)
        return false;

    for (uint8_t i = 0; i < len; i++)
        buf[i] = Wire.read();

    return true;
}

// =============================================
//  Sensor init sequence
// =============================================

static bool initSensor()
{
    // 1. Confirm chip identity
    uint8_t whoAmI = 0;
    if (!regRead(REG_WHO_AM_I, &whoAmI))
    {
        _errorCode = 0xFF;  // I2C failure
        return false;
    }

    if (whoAmI != WHO_AM_I_VALUE)
    {
        _errorCode = whoAmI;  // show what we got vs what we expected
        return false;
    }

    // 2. Set ODR = 100Hz, continuous measurement mode
    if (regWrite(REG_CFG_REG_A, CFG_A_ODR_100HZ_CONT) != 0)
    {
        _errorCode = 0xFE;  // config write failed
        return false;
    }

    // 3. Enable Block Data Update (prevents split reads)
    if (regWrite(REG_CFG_REG_C, CFG_C_BDU) != 0)
    {
        _errorCode = 0xFD;
        return false;
    }

    return true;
}

// =============================================
//  Read output registers
// =============================================

static bool readMag()
{
    // Check data ready bit in STATUS register (bit 3 = ZYXDA)
    uint8_t status = 0;
    if (!regRead(REG_STATUS, &status))
        return false;

    if (!(status & 0x08))
        return false;   // data not ready yet — skip this tick

    // Burst read 6 bytes: XL XH YL YH ZL ZH
    uint8_t buf[6];
    if (!regReadBurst(REG_OUTX_L, buf, 6))
        return false;

    // Reconstruct signed 16-bit values (little-endian)
    _rawX = (int16_t)((buf[1] << 8) | buf[0]);
    _rawY = (int16_t)((buf[3] << 8) | buf[2]);
    _rawZ = (int16_t)((buf[5] << 8) | buf[4]);

    return true;
}

// =============================================
//  Scale raw to mGauss × 10 (tenths of mGauss)
//  Sensitivity = 1.5 mGauss/LSB
//  × 10 → 15/10 per LSB, integer only
// =============================================

static int32_t toMgaussTenths(int16_t raw)
{
    return ((int32_t)raw * 15) / 10;
}

// =============================================
//  Format mGauss tenths into "NNNN.N" string
//  e.g. -1234 tenths → "-123.4"
// =============================================

static void fmtMgauss(int32_t tenths, char* buf, uint8_t bufLen)
{
    const char* sign = (tenths < 0) ? "-" : " ";
    int32_t abs = (tenths < 0) ? -tenths : tenths;
    int32_t whole = abs / 10;
    int32_t frac  = abs % 10;
    snprintf(buf, bufLen, "%s%ld.%ld", sign, (long)whole, (long)frac);
}

// =============================================
//  Draw helpers
// =============================================

static void drawHeader(const char* subtitle)
{
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, "LIS2MDL");
    u8g2.drawHLine(0, 13, DISPLAY_W);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 22, subtitle);
}

static void drawHintBar()
{
    u8g2.drawHLine(0, 54, DISPLAY_W);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 63, "TAP: Back");
}

// =============================================
//  Draw: MAG_ERROR
// =============================================

static void drawError()
{
    drawHeader("Init failed");

    u8g2.setFont(u8g2_font_5x7_tr);

    if (_errorCode == 0xFF)
    {
        u8g2.drawStr(4, 34, "I2C no response");
        u8g2.drawStr(4, 43, "Check wiring");
        u8g2.drawStr(4, 52, "Addr: 0x1E");
    }
    else
    {
        // WHO_AM_I mismatch — show expected vs got
        char buf[28];
        snprintf(buf, sizeof(buf), "WHO_AM_I: got 0x%02X", _errorCode);
        u8g2.drawStr(4, 34, buf);
        u8g2.drawStr(4, 43, "Expected: 0x40");
        u8g2.drawStr(4, 52, "Wrong device?");
    }

    drawHintBar();
}

// =============================================
//  Draw: MAG_RUNNING
//
//  Layout:
//    y=10  "LIS2MDL"          header
//    y=13  separator
//    y=22  "0x1E | 100Hz"     subtitle
//    y=33  "X:  -123.4 mG"
//    y=42  "Y:   456.7 mG"
//    y=51  "Z:    89.0 mG"
//    y=54  separator
//    y=63  "TAP: Back"
// =============================================

static void drawRunning()
{
    // Header
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, "LIS2MDL");
    u8g2.drawHLine(0, 13, DISPLAY_W);

    // Subtitle: address + ODR confirmation
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 22, "0x1E  100Hz  mGauss");

    // X axis
    char xBuf[12], yBuf[12], zBuf[12];
    fmtMgauss(toMgaussTenths(_rawX), xBuf, sizeof(xBuf));
    fmtMgauss(toMgaussTenths(_rawY), yBuf, sizeof(yBuf));
    fmtMgauss(toMgaussTenths(_rawZ), zBuf, sizeof(zBuf));

    char lineBuf[22];

    snprintf(lineBuf, sizeof(lineBuf), "X: %s", xBuf);
    u8g2.drawStr(4, 33, lineBuf);

    snprintf(lineBuf, sizeof(lineBuf), "Y: %s", yBuf);
    u8g2.drawStr(4, 42, lineBuf);

    snprintf(lineBuf, sizeof(lineBuf), "Z: %s", zBuf);
    u8g2.drawStr(4, 51, lineBuf);

    drawHintBar();
}

// =============================================
//  Public: resetLIS2MDL()
//  Called by onToolEnter() before first tick
// =============================================

void resetLIS2MDL()
{
    _state      = MAG_INIT;
    _rawX       = 0;
    _rawY       = 0;
    _rawZ       = 0;
    _lastReadMs = 0;
    _errorCode  = 0;
}

// =============================================
//  Public: drawLIS2MDL()
//  Called every loop() tick while tool is active
// =============================================

void drawLIS2MDL()
{
    u8g2.clearBuffer();

    switch (_state)
    {
        case MAG_INIT:
            drawHeader("Initialising...");
            break;

        case MAG_ERROR:
            drawError();
            break;

        case MAG_RUNNING:
            drawRunning();
            break;
    }

    u8g2.sendBuffer();
}

// =============================================
//  Public: handleLIS2MDL()
//  Called every loop() tick while tool is active
// =============================================

void handleLIS2MDL()
{
    // TAP back — works in all states
    if (btnSelect() == BTN_EVT_SHORT_PRESS)
    {
        setActiveTool(TOOL_NONE);
        setScreen(SCREEN_MENU);
        return;
    }

    // Must still poll up/down even if unused — keeps button state machines running
    btnUp();
    btnDown();

    switch (_state)
    {
        // == Run init sequence once on entry ==========================
        case MAG_INIT:
            if (initSensor())
            {
                _lastReadMs = millis();
                _state      = MAG_RUNNING;
            }
            else
            {
                _state = MAG_ERROR;
            }
            break;

        // == Error: nothing to do, user taps back =====================
        case MAG_ERROR:
            break;

        // == Running: poll for new data every READ_INTERVAL_MS ========
        case MAG_RUNNING:
            if (millis() - _lastReadMs >= READ_INTERVAL_MS)
            {
                _lastReadMs = millis();
                readMag();   // updates _rawX/Y/Z if data is ready
                             // silently skips if DRDY not set yet
            }
            break;
    }
}