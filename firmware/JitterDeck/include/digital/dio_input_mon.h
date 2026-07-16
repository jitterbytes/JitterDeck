#ifndef DIO_INPUT_MON_H
#define DIO_INPUT_MON_H

// =============================================
//  Digital Input Monitor
//  Reads GPIO 5 (DIO_PIN) and displays live
//  HIGH/LOW state, updated every 50ms.
// =============================================

void drawDIOInputMon();
void handleDIOInputMon();
void resetDIOInputMon();

#endif