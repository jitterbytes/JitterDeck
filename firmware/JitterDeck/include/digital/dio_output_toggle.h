#ifndef DIO_OUTPUT_TOGGLE_H
#define DIO_OUTPUT_TOGGLE_H

// =============================================
//  Digital Output Toggle
//  Drives GPIO 5 (DIO_PIN) HIGH or LOW.
//  Long press SELECT toggles state.
//  Short press SELECT goes back.
// =============================================

void drawDIOOutputToggle();
void handleDIOOutputToggle();
void resetDIOOutputToggle();

#endif