#ifndef FSM_H
#define FSM_H

#include "menu_defs.h"

// =======================================================
// Top Level Screen States
// Navigation depth is managed by nav_stack
// =======================================================

enum Screen
{
    SCREEN_BOOT,    // boot animation is running
    SCREEN_MENU,    // user is navigating menus -> nav_stack drives all menu depth
    SCREEN_TOOL     // a leaf tool is active
};

// ===============================
// Public APIs
// ===============================

void    fsmInit();

void    setScreen(Screen screen);
Screen  getScreen();

// Active tool - set before transitioning to SCREEN_TOOL

void    setActiveTool(ToolID tool);
ToolID  getActiveTool();

#endif