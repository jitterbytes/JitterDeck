#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

// ==========================
//  Button event type
// ==========================

enum ButtonEvent
{
    BTN_EVT_NONE,
    BTN_EVT_SHORT_PRESS,   // Mainly used for btnSelect() Tap -> Back / Cancel
    BTN_EVT_LONG_PRESS     // Mainly used for btnSelect() Hold -> Enter / Confirm
};

// ==============================
//  Per-button state tracker 
// ==============================

struct ButtonState
{
    bool lastRaw;                   // raw pin read last tick
    bool pressed;                   // debounced pressed state
    unsigned long pressStartMs;     // when debounced press began
    bool longFired;                 // long press event already emitted
};

// =======================
//  Public API
// =======================

void buttonInit();
ButtonEvent buttonUpdate(uint8_t pin, ButtonState& state);

// Convenience wrappers - one per physical button
ButtonEvent btnUp();
ButtonEvent btnDown();
ButtonEvent btnSelect();

#endif