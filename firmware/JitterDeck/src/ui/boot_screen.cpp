#include <Arduino.h>
#include <U8g2lib.h>

#include "boot_screen.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// =============
// Constants
// =============

static const char*   TITLE          = "JITTERDECK";
static const char*   TAGLINE        = "HW VALIDATION TOOL";
static const uint8_t TITLE_LEN      = 10;

// Timing windows (ms)
static const uint16_t GLITCH_END    = 3000;  // pure scramble phase
static const uint16_t RESOLVE_END   = 4000;  // letter-by-letter resolve
static const uint16_t BOOT_TOTAL    = 10000;  // must match deck_v1.ino transition

// Progress bar geometry
static const uint8_t BAR_X         = 4;
static const uint8_t BAR_Y         = 50;
static const uint8_t BAR_W         = 120;
static const uint8_t BAR_H         = 8;
static const uint8_t SEG_GAP       = 2;
static const uint8_t NUM_SEGMENTS  = 10;

// Glitch character pool - printable ASCII noise
static const char    GLITCH_CHARS[]    = "!@#$%^&*<>?/\\|~+=[]{}";
static const uint8_t GLITCH_POOL_SIZE  = sizeof(GLITCH_CHARS) - 1;

// =============
// State
// =============

static bool          _initialized  = false;
static unsigned long _startTime    = 0;
static char          _glitchBuf[TITLE_LEN + 1];

// Lightweight LCG PRNG — deterministic, no heap, embedded-safe
static uint16_t _lcgState = 0xACE1;

static uint8_t lcgRand()
{
    _lcgState = (uint16_t)((_lcgState * 214013u + 2531011u) >> 8);
    return (uint8_t)(_lcgState & 0xFF);
}

// ================
// Public API
// ================

void resetBootScreen()
{
    _initialized = false;
    _startTime   = 0;
    _lcgState    = 0xACE1;
}

// =============================================
//  Internal helpers
// =============================================

// Build display buffer: locked letters up to resolvedCount, noise beyond
static void buildGlitchBuffer(uint8_t resolvedCount)
{
    for (uint8_t i = 0; i < TITLE_LEN; i++)
    {
        _glitchBuf[i] = (i < resolvedCount)
            ? TITLE[i]
            : GLITCH_CHARS[lcgRand() % GLITCH_POOL_SIZE];
    }
    _glitchBuf[TITLE_LEN] = '\0';
}

// Chunky segmented progress bar — filledSegments in [0, NUM_SEGMENTS]
static void drawProgressBar(uint8_t filledSegments)
{
    const uint8_t segW = (BAR_W - (NUM_SEGMENTS - 1) * SEG_GAP) / NUM_SEGMENTS;

    // Outer border frame
    u8g2.drawFrame(BAR_X - 2, BAR_Y - 2, BAR_W + 4, BAR_H + 4);

    for (uint8_t s = 0; s < NUM_SEGMENTS; s++)
    {
        uint8_t sx = BAR_X + s * (segW + SEG_GAP);

        if (s < filledSegments)
            u8g2.drawBox(sx, BAR_Y, segW, BAR_H);   // filled block
        else
            u8g2.drawFrame(sx, BAR_Y, segW, BAR_H); // empty block outline
    }
}

// ======================
// drawBootScreen()
// ======================

void drawBootScreen()
{
    if (!_initialized)
    {
        _initialized = true;
        _startTime   = millis();
    }

    const unsigned long elapsed = millis() - _startTime;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tr);

    // == Phase 1: Pure glitch scramble (0 – GLITCH_END) ============
    if (elapsed < GLITCH_END)
    {
        buildGlitchBuffer(0);
        u8g2.drawStr(24, 22, _glitchBuf);
    }

    // == Phase 2: Letter-by-letter resolve (GLITCH_END – RESOLVE_END)
    else if (elapsed < RESOLVE_END)
    {
        const uint32_t window   = RESOLVE_END - GLITCH_END;
        const uint32_t progress = elapsed - GLITCH_END;
        const uint8_t  resolved = (uint8_t)((progress * TITLE_LEN) / window);

        buildGlitchBuffer(resolved < TITLE_LEN ? resolved : TITLE_LEN);
        u8g2.drawStr(24, 22, _glitchBuf);
    }

    // == Phase 3: Resolved title + tagline + progress bar ==========
    else
    {
        u8g2.drawStr(24, 22, TITLE);
        

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(14, 34, TAGLINE);

        const uint32_t window   = BOOT_TOTAL - RESOLVE_END;
        const uint32_t progress = elapsed - RESOLVE_END;
        const uint8_t  filled   = (uint8_t)((progress * NUM_SEGMENTS) / window);

        drawProgressBar(filled < NUM_SEGMENTS ? filled : NUM_SEGMENTS);
    }

    u8g2.sendBuffer();
}