#ifndef LM75A_H
#define LM75A_H

// =============================================
//  LM75A - Digital Temperature Sensor (NXP/TI)
//  Breakout: CJMCU-75
//  I²C address: 0x4B (A1=HIGH, A0=HIGH, A2=LOW)
//  Resolution: 0.5°C (9-bit), no WHO_AM_I register
//  Register: 0x00, 16-bit big-endian, upper 9 bits valid
//
//  Wire from screens.cpp:
//    case TOOL_I2C_LM75A:
//      handleLM75A();
//      drawLM75A();
//      break;
// =============================================

void drawLM75A();
void handleLM75A();
void resetLM75A();

#endif