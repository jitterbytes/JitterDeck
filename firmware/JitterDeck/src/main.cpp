#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "config.h"
#include "fsm.h"
#include "nav_stack.h"
#include "button.h"
#include "boot_screen.h"
#include "menu.h"
#include "screens.h"

// =============================================
//  Display instance — shared across all modules
//  via extern declaration in each .cpp file
// =============================================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

static unsigned long _bootStart = 0;

void setup()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
    buttonInit();
    fsmInit();
    navInit();
    resetBootScreen();
    _bootStart = millis();
}

void loop()
{
    switch (getScreen())
    {
        case SCREEN_BOOT:
            drawBootScreen();
            if (millis() - _bootStart >= BOOT_DURATION_MS)
                setScreen(SCREEN_MENU);
            break;

        case SCREEN_MENU:
            handleMenu();
            drawMenu();
            break;

        case SCREEN_TOOL:
            handleToolScreen();
            drawToolScreen();
            break;
    }
}