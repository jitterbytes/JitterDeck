#ifndef SCREENS_H
#define SCREENS_H

#include "menu_defs.h"

// =============================================
//  Tool screen dispatcher
// =============================================

void drawToolScreen();
void handleToolScreen();

// Called once when entering a tool — resets tool state
void onToolEnter(ToolID id);

#endif