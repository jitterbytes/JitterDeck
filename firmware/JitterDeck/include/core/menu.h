#ifndef MENU_H
#define MENU_H

// ===========================================================
//  Generic carousel menu renderer + handler
//
//  Reads current state from navCurrent() and
//  dispatches button events via nav_stack API.
//  Call both every loop() tick while in SCREEN_MENU.
// ===========================================================

void drawMenu();
void handleMenu();

#endif