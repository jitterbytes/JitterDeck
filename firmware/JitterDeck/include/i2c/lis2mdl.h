#ifndef LIS2MDL_H
#define LIS2MDL_H

// =============================================
//  LIS2MDL — 3-axis magnetometer (ST)
//  I2C address: 0x1E
//  Register-level driver, no external library
//
//  Wire from screens.cpp:
//    case TOOL_I2C_LIS2MDL:
//      handleLIS2MDL();
//      drawLIS2MDL();
//      break;
// =============================================

void drawLIS2MDL();
void handleLIS2MDL();
void resetLIS2MDL();

#endif