#include <Arduino.h>
#include <U8g2lib.h>

#include "dio_pwm_gen.h"
#include "button.h"
#include "fsm.h"
#include "config.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// =============================================
//  LEDC configuration
//  ESP32-C3 LEDC peripheral — dedicated PWM hardware
// =============================================

static const uint8_t  LEDC_CHANNEL    = 0;
static const uint8_t  LEDC_RESOLUTION = 8;    // 8-bit: 0–255 duty values
static const uint8_t  DUTY_MAX        = 255;

// =============================================
//  Frequency presets (Hz)
//  Selectable with UP/DOWN when freq is active
// =============================================

static const uint32_t FREQ_PRESETS[]  = { 100, 500, 1000, 2000, 5000, 10000, 20000, 50000 };
static const uint8_t  FREQ_COUNT      = 8;

// =============================================
//  Duty cycle presets (percent, 5% steps)
// =============================================

static const uint8_t DUTY_STEP_PCT    = 5;    // 5% increments
static const uint8_t DUTY_MIN_PCT     = 5;
static const uint8_t DUTY_MAX_PCT     = 95;

// =============================================
//  Which parameter is active for UP/DOWN
// =============================================

enum PWMParam { PARAM_FREQ, PARAM_DUTY };

// =============================================
//  State
// =============================================

static PWMParam  _activeParam  = PARAM_FREQ;
static uint8_t   _freqIdx      = 2;          // default: 1kHz (index 2)
static uint8_t   _dutyPct      = 50;         // default: 50%
static bool      _pwmRunning   = false;

// =============================================
//  Internal: apply current settings to LEDC
// =============================================

static void applyPWM()
{
    uint32_t freq    = FREQ_PRESETS[_freqIdx];
    uint32_t dutyRaw = (uint32_t)_dutyPct * DUTY_MAX / 100;

    if (!_pwmRunning)
    {
        ledcSetup(LEDC_CHANNEL, freq, LEDC_RESOLUTION);
        ledcAttachPin(DIO_PIN, LEDC_CHANNEL);
        _pwmRunning = true;
    }
    else
    {
        ledcSetup(LEDC_CHANNEL, freq, LEDC_RESOLUTION);
    }

    ledcWrite(LEDC_CHANNEL, dutyRaw);
}

static void stopPWM()
{
    if (_pwmRunning)
    {
        ledcWrite(LEDC_CHANNEL, 0);
        ledcDetachPin(DIO_PIN);
        pinMode(DIO_PIN, OUTPUT);
        digitalWrite(DIO_PIN, LOW);
        _pwmRunning = false;
    }
}

// =============================================
//  Public: resetDIOPWMGen()
// =============================================

void resetDIOPWMGen()
{
    stopPWM();
    _activeParam = PARAM_FREQ;
    _freqIdx     = 2;      // 1kHz
    _dutyPct     = 50;     // 50%
    applyPWM();            // start outputting immediately on enter
}

// =============================================
//  drawDIOPWMGen()
//
//  Layout:
//    y=10  "PWM Generator"          header
//    y=13  separator
//    y=22  "GPIO 5    LEDC Ch 0"
//
//    Frequency row (highlighted if active):
//    y=33  "> Freq:  1000 Hz"
//
//    Duty cycle row (highlighted if active):
//    y=44  "> Duty:    50 %"
//
//    Visual duty bar:
//    y=48–53  [████████░░░░░░░░░░░░]
//
//    y=54  separator
//    y=63  "HOLD:Switch   TAP:Back"
// =============================================

void drawDIOPWMGen()
{
    u8g2.clearBuffer();

    // Header
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, "PWM Generator");
    u8g2.drawHLine(0, 13, DISPLAY_W);

    // Pin info
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 22, "GPIO 5    LEDC Ch 0");

    // == Frequency row =================================================
    char freqBuf[20];
    uint32_t f = FREQ_PRESETS[_freqIdx];
    if (f >= 1000)
        snprintf(freqBuf, sizeof(freqBuf), "%lu kHz", (unsigned long)(f / 1000));
    else
        snprintf(freqBuf, sizeof(freqBuf), "%lu Hz", (unsigned long)f);

    if (_activeParam == PARAM_FREQ)
    {
        u8g2.drawBox(0, 24, DISPLAY_W, 12);
        u8g2.setDrawColor(0);
    }
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 33, "Freq:");
    u8g2.drawStr(40, 33, freqBuf);
    if (_activeParam == PARAM_FREQ)
    {
        u8g2.drawStr(110, 33, "\x11\x10");  // left-right arrows showing adjustable
        u8g2.setDrawColor(1);
    }

    // == Duty cycle row ================================================
    char dutyBuf[8];
    snprintf(dutyBuf, sizeof(dutyBuf), "%d%%", _dutyPct);

    if (_activeParam == PARAM_DUTY)
    {
        u8g2.drawBox(0, 35, DISPLAY_W, 12);
        u8g2.setDrawColor(0);
    }
    u8g2.drawStr(4, 44, "Duty:");
    u8g2.drawStr(40, 44, dutyBuf);
    if (_activeParam == PARAM_DUTY)
    {
        u8g2.drawStr(110, 44, "\x11\x10");
        u8g2.setDrawColor(1);
    }

    // == Duty bar ======================================================
    const uint8_t barX = 4, barY = 47, barW = 120, barH = 5;
    uint8_t fillW = (uint8_t)((uint16_t)_dutyPct * barW / 100);
    u8g2.drawFrame(barX, barY, barW, barH);
    if (fillW > 0)
        u8g2.drawBox(barX, barY, fillW, barH);

    // Hint bar
    u8g2.drawHLine(0, 54, DISPLAY_W);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 63, "HOLD: Switch param");
    uint8_t rw = u8g2.getStrWidth("TAP: Back");
    u8g2.drawStr(DISPLAY_W - rw - 2, 63, "TAP: Back");

    u8g2.sendBuffer();
}

// =============================================
//  handleDIOPWMGen()
// =============================================

void handleDIOPWMGen()
{
    const ButtonEvent evUp     = btnUp();
    const ButtonEvent evDown   = btnDown();
    const ButtonEvent evSelect = btnSelect();

    // TAP back — stop PWM cleanly before leaving
    if (evSelect == BTN_EVT_SHORT_PRESS)
    {
        stopPWM();
        setActiveTool(TOOL_NONE);
        setScreen(SCREEN_MENU);
        return;
    }

    // LONG SELECT — switch active parameter
    if (evSelect == BTN_EVT_LONG_PRESS)
    {
        _activeParam = (_activeParam == PARAM_FREQ) ? PARAM_DUTY : PARAM_FREQ;
        return;
    }

    bool changed = false;

    if (_activeParam == PARAM_FREQ)
    {
        if (evUp == BTN_EVT_SHORT_PRESS || evUp == BTN_EVT_LONG_PRESS)
        {
            if (_freqIdx < FREQ_COUNT - 1) { _freqIdx++; changed = true; }
        }
        if (evDown == BTN_EVT_SHORT_PRESS || evDown == BTN_EVT_LONG_PRESS)
        {
            if (_freqIdx > 0) { _freqIdx--; changed = true; }
        }
    }
    else // PARAM_DUTY
    {
        if (evUp == BTN_EVT_SHORT_PRESS || evUp == BTN_EVT_LONG_PRESS)
        {
            if (_dutyPct + DUTY_STEP_PCT <= DUTY_MAX_PCT)
                { _dutyPct += DUTY_STEP_PCT; changed = true; }
        }
        if (evDown == BTN_EVT_SHORT_PRESS || evDown == BTN_EVT_LONG_PRESS)
        {
            if (_dutyPct - DUTY_STEP_PCT >= DUTY_MIN_PCT)
                { _dutyPct -= DUTY_STEP_PCT; changed = true; }
        }
    }

    if (changed)
        applyPWM();
}