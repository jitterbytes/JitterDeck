#ifndef DIO_PWM_GEN_H
#define DIO_PWM_GEN_H

// =============================================
//  PWM Generator
//  Outputs PWM on GPIO 5 (DIO_PIN) using
//  ESP32-C3 LEDC peripheral.
//
//  UP/DOWN   → adjust active parameter
//  LONG SEL  → switch between Frequency / Duty
//  SHORT SEL → stop PWM and go back
// =============================================

void drawDIOPWMGen();
void handleDIOPWMGen();
void resetDIOPWMGen();

#endif