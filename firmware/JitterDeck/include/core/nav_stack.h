#ifndef NAV_STACK_H
#define NAV_STACK_H

#include <stdint.h>
#include "menu_defs.h"
#include "config.h"

// ===============================================================
//  NavFrame
//  One level of the navigation stack. One level of where I am
//  Stores the menu array being displayed and
//  which item the cursor is on.
// ===============================================================

struct NavFrame
{
    const MenuItem* items;       // pointer to menu item array at this level
    uint8_t         count;       // number of items in array
    uint8_t         cursor;      // which item is selected right now
    const char*     title;       // header label shown at top of screen
};

// ========================
//  Public API
// ========================

void navInit();

// Push a new menu level onto the stack
bool navPush(const MenuItem* items, uint8_t count, const char* title);

// Pop back to the previous level — returns false if already at root
bool navPop();

// Peek at the current frame (top of stack)
const NavFrame*  navCurrent();

// Move cursor up/down within current frame (wraps)
void navCursorUp();
void navCursorDown();

// Returns the item currently under the cursor
const MenuItem*  navSelectedItem();

// Stack depth (1 = root main menu)
uint8_t          navDepth();

#endif