#include <Arduino.h>
#include "button.h"
#include "config.h"

// ===========================================
//  Static state for each physical button
// ===========================================

static ButtonState _stateUp;
static ButtonState _stateDown;
static ButtonState _stateSelect;

// =============================
//  buttonInit()
//  Call once from setup() after pinMode is set.
// =============================

void buttonInit()
{
    pinMode(BTN_UP,     INPUT_PULLUP);
    pinMode(BTN_DOWN,   INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    // Zero-init all state structs
    _stateUp     = { true, false, 0, false };
    _stateDown   = { true, false, 0, false };
    _stateSelect = { true, false, 0, false };
}

// =============================
//  buttonUpdate()
//  Call every loop() tick per button.
//
//  State machine:
//    Raw LOW  → debounce → pressed
//    While pressed & time > LONGPRESS_MS → emit LONG_PRESS (once)
//    Raw HIGH after pressed & no long fired → emit SHORT_PRESS
// =============================

ButtonEvent buttonUpdate(uint8_t pin, ButtonState& s)
{
    const bool raw = (digitalRead(pin) == LOW);  // active LOW

    // Debounce: only act on stable transitions
    if (raw != s.lastRaw)
    {
        s.lastRaw = raw;

        if (raw)
        {
            // Falling edge - button just went down
            if (!s.pressed)
            {
                s.pressed      = true;
                s.pressStartMs = millis();
                s.longFired    = false;
            }
        }
        else
        {
            // Rising edge - button released
            if (s.pressed)
            {
                s.pressed = false;

                if (!s.longFired)
                {
                    // Released before long press threshold → short press
                    unsigned long held = millis() - s.pressStartMs;
                    if (held >= BTN_DEBOUNCE_MS)
                        return BTN_EVT_SHORT_PRESS;
                }
            }
        }
    }

    // Long press detection (fires once while held) 
    if (s.pressed && !s.longFired) 
    {
        if ((millis() - s.pressStartMs) >= BTN_LONGPRESS_MS)
        {
            s.longFired = true;
            return BTN_EVT_LONG_PRESS;
        }
    }

    return BTN_EVT_NONE;
}

// =============================
//  Convenience wrappers
// =============================

ButtonEvent btnUp()     { return buttonUpdate(BTN_UP,     _stateUp);     }
ButtonEvent btnDown()   { return buttonUpdate(BTN_DOWN,   _stateDown);   }
ButtonEvent btnSelect() { return buttonUpdate(BTN_SELECT, _stateSelect); }